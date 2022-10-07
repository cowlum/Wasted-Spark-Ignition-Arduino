// Dual Core
// Connects to a access point and sends an NMEA IIRPM string
//
#include "RemoteDebug.h"  //https://github.com/JoaoLopesF/RemoteDebug
#include <WiFi.h>
#include <stdio.h>
#include <string.h>

// Core Tasks
TaskHandle_t Task1;
TaskHandle_t Task2;

/* 
Wifi Variables 
*/
const char* ssid = "midnitesun";
const char* password =  "ericson27";
const char* server = "10.0.0.1";
int port = 50015;


int connectAttempts = 0;
int WiFiconnectAttempts = 0;
unsigned long time_since_failure;

WiFiClient client;

RemoteDebug Debug;

/// IGNITION SETUP

//Hall Effect Sensor Settings
#define INTERRUPT_PIN 21

//Put hall changed function into memory
void ICACHE_RAM_ATTR hallChanged();

//Timing calc variables
volatile bool newPulse = false;
volatile float revMicros = 150000;
int advancetach = 0;
volatile unsigned long int latestPulseMicros; 
unsigned long int prevPulseMicros;
unsigned long int prerevMicros = 150000;
unsigned long int ignDelay = 2500000;
unsigned long int dwellMicros;  

long int ignAdjust = 1000; //timing adjestment. Should be zero when hall effect in correct position.
int rev_limit = 2200; // max rpm before spark cut

bool inRange = true;
int missfire = 0;
unsigned long int rpmtach = 0;
unsigned long int rpm = 200;
unsigned long int dwell = 3000;
int advanceKey = 2;

volatile bool magnet;
bool previousMagnet;

//BANK OUTPUTS
#define bank1 02
#define bank2 04
#define tach 19
#define relay 18

//Advance curve
float advancePercentage[] = {
  0.990,0.980,0.960,0.950,0.940,0.940,0.940,0.940,0.939,0.933,0.933,0.928,0.922,0.917,0.911,0.906,0.900,0.894,0.889,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.900,0.917,0.944,0.950,0.950,0.950,0.950
  }; 
//0                                                     10                             15                            20                            25                            30                           35                            40                            45        
float dwellPercentage[] = {
  1.300,1.300,1.300,1.300,1.300,1.250,1.250,1.250,1.200,1.100,1.050,1.000,1.000,1.000,1.000,0.990,0.980,0.970,0.960,0.950,0.940,0.930,0.920,0.910,0.900,0.890,0.880,0.870,0.860,0.850,0.840,0.830,0.820,0.810,0.800,0.790,0.780,0.770,0.760,0.750,0.740,0.730,0.720,0.710,0.700,0.700
  }; 


void setup() {
  disableCore0WDT();
  disableCore1WDT();
  Serial.begin(115200);
  
  pinMode(bank1, OUTPUT); 
  pinMode(bank2, OUTPUT);
  pinMode(tach, OUTPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(10); 

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    10,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
  delay(10);

}


void Task1code( void * pvParameters ){
  wifi_connect();
  for(;;){
    nmea_sender();
    delay(1000);
  }
}

void Task2code( void * pvParameters ){
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), hallChanged, CHANGE);
  for(;;){
    pulseFunction();
  }
}

/*
   NMEA LOGIC
*/

void wifi_connect(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
    connectAttempts++;
    if (connectAttempts == 120){
      WiFi.mode(WIFI_OFF);
      break;
    }
  }
  connectAttempts = 0;
  client.connect(server, port);
  // Initialize the server (telnet or web socket) of RemoteDebug
  String Host_Name =  String(WiFi.localIP());
  Debug.begin(Host_Name);
  Debug.setResetCmdEnabled(true); // Enable the reset command
  }

void nmea_sender(){ // Compile san send the nmea sentence.
 //rpmtach = random(1200, 3300); //used for debugging, fake RPM.
     String nmea_rpm_str = "$IIXDR,T," + String(rpmtach) + ".0,R,ENGINE#0";
     int nmea_len = nmea_rpm_str.length() + 1;
     char nmea_array[nmea_len]; 
     nmea_rpm_str.toCharArray(nmea_array, nmea_len);
     client.printf("%s%s%X\n", nmea_rpm_str.c_str(),"*", nmea0183_checksum(nmea_array));
     //Serial.print(nmea_rpm_str);
     //Serial.print("*");
     //Serial.println(nmea0183_checksum(nmea_array), HEX);
     //Serial.println(client.connected());
     if ((client.connected() != 1) && (connectAttempts < 60)){
        connectAttempts++;
        client.connect(server, port);
        time_since_failure = micros();
      } else if ((micros() - time_since_failure) > 100000000  ){ 
        connectAttempts = 0;
      } else if (connectAttempts == 60){
        WiFi.mode(WIFI_OFF);
        connectAttempts++;
        Serial.println("wifi mode off");
      } 
      #ifndef DEBUG_DISABLED
      if (Debug.isActive(Debug.VERBOSE)) {
          debugI("Rpm   = %d", rpmtach);
          debugI("Dwell = %d", dwell);
          debugV("IgnDelay = %d", ignDelay);
          debugV("missfire = %d", missfire);
          debugV("IgnAdjust = %d", ignAdjust);
          debugV("revMicros = %d", revMicros);
          debugV("prerevMicros = %d", prerevMicros);
          debugV("rev_limit = %d", rev_limit);
          debugV("--------------------------");
      }
      #endif
      Debug.handle();
}

int nmea0183_checksum(char *nmea_array){  // https://forum.arduino.cc/t/nmea0183-checksum/559531/2
    int crc = 0;
    int i;
    // ignore the first $ sign,  no checksum in sentence
    for (i = 1; i < strlen(nmea_array); i ++) { 
        crc ^= nmea_array[i];
    }
    return crc;
}


/*
   IGNITION LOGIC
*/

// interrupt triggers, turn off going high on coil pack(may be redundant if the bank(s) already fired),ser time of interrupt, newpulse and magnet polarity.
void hallChanged(){
  GPIO.out_w1tc = (1 << bank1);
  GPIO.out_w1tc = (1 << bank2);
  GPIO.out_w1tc = (1 << tach);
 
  latestPulseMicros = micros();
  newPulse = true;  
  if(digitalRead(INTERRUPT_PIN) == 0){
    magnet = true;
   }
  else{
    magnet = false;
   }
}

// Delay for 1/2 revolution before dwell starts
void delayfunc(){     
     while (micros() - latestPulseMicros <= (ignDelay - dwell) && (newPulse == false) ) {
    }
}

// Duration of dwell
void delaydwellfunc(){ 
    dwellMicros = micros();
    while ((micros() - dwellMicros <= dwell) && (newPulse == false)) {
      }
    }

// If magnet polarity change, wait 1/2 rpm duration, pin up for dwell duration, then pin down. 
void magnetfunction(){  //function that fires the banks
  if (magnet != previousMagnet){
    if (magnet == true){ 
      previousMagnet = magnet;
      if ((rpm>=130) && (rpm<=rev_limit) && (inRange == true))
        {
      delayfunc();
      GPIO.out_w1ts = (1 << bank1);
      GPIO.out_w1ts = (1 << tach);
      delaydwellfunc();
      GPIO.out_w1tc = (1 << bank1);
      GPIO.out_w1tc = (1 << tach);
        }
    }
    else {
      previousMagnet = magnet;
      if ((rpm>=130) && (rpm<=rev_limit) && (inRange == true))
       {
      delayfunc();
      GPIO.out_w1ts = (1 << bank2);
      GPIO.out_w1ts = (1 << tach);
      delaydwellfunc();
      GPIO.out_w1tc = (1 << bank2);
      GPIO.out_w1tc = (1 << tach);
      }
      else{
      Serial.print("revlimiter");
        }
    }      
  }
  else {
  }
}

// Check the last 1/2 rpm is not too much faster or slower than the latest.
void inRangefunc(){
  if ((revMicros>(prerevMicros * 0.60)) && revMicros<(prerevMicros * 1.10)) // Left is Acceleration limit. Right is deAccelration
{
  inRange = true;
  //Serial.print(" \n Within Range");
} else {
  missfire++;
  inRange = false;
  /*
    // Print details if for debugging if there is a missfire.
    Serial.print(" \n Range Exceeded NO FIRE");  // debugging
    Serial.print(", prerevMircros = ");
    Serial.print(prerevMicros);
    Serial.print(", igndelay = ");
    Serial.print(ignDelay);
    Serial.print(", revMicros = ");
    Serial.print(revMicros);
    Serial.print(", as a percent = ");
    Serial.print((float)revMicros/(float)prerevMicros);
  */}
}

/*
Upon newPulse from interrupt.
Previous 1/2 rpm time is set to prerevMicros for the inRangefunc function to utilise.
Latest 1/2 rpm time is recorded and checked for duration limits.
*/
void pulseFunction(){
      if (newPulse == true) {
      newPulse = false;  
      prerevMicros=revMicros; // In range function
     
      // revMicros equals the new pulse just taken minus the old revmicros
      revMicros = latestPulseMicros - prevPulseMicros;   
      if ((revMicros < 7000) || (revMicros > 210000)){
        Serial.print(" \n REVMICROS redacted  ");
        Serial.print(revMicros);
        revMicros = 300000;
      }
      
      // Latest pulse time is recrded for next
      prevPulseMicros = latestPulseMicros;  time.

      // Check for wild variation
      inRangefunc();
      
      // Reset dwell
      dwell = 3000;

      // Calculate RPM based on time between pulses
      rpm = (revMicros*2)/1000;
      rpm = (1000/rpm)*60;
      rpmtach = (revMicros*2)/1000;
      rpmtach = (1000/rpmtach)*60;
      if (rpmtach < 101){
        rpmtach = 0;
      }
                
      advancetach = 180-(180*(float)advancePercentage[advanceKey]);

      // Using rpm select the advance key for use with advance array.
      advanceKey = round(rpm/100);
      if ((rpm < 500) && (advanceKey > 10))  { // Probably not needed due to the revMicros Reset.
       Serial.print(" \n ADVANCE KEY redacted");
        advanceKey = 2;
      }

      // Adjust the dwell time to suit RPM via advanceKey.
      dwell = (dwell * dwellPercentage[advanceKey]);

      // Calculate the wait time before pin up. 
      ignDelay = (revMicros * (advancePercentage[advanceKey]));
      ignDelay = ignDelay + ignAdjust; 

      //With timing calculated run the magnet polarity check and pinup/down appropriately.
      magnetfunction();
  }
}

void loop() {
}
