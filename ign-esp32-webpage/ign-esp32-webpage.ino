// Dual Core
// Creates a wifi access point and hosts a simple webpage diplaying basic engine data.
// Youtube example https://youtu.be/GZBaOKuiqLw
//

// Core Tasks
TaskHandle_t Task1;
TaskHandle_t Task2;


#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();


// IGNITION SETUP

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

long int ignAdjust = 500; //timing adjestment. Should be zero when hall effect in correct position.

bool inRange = true;
int missfire = 0;
float rpmtach = 0;
float cputemp = 0;
char* revlimit = "false";
unsigned long int rpm = 200;
unsigned long int rpmDebug = 0;
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


/// WIFI SETUP

#include <WiFi.h>
#include <WebServer.h>

IPAddress         apIP(10, 10, 10, 1);    // Private network for server
WebServer         webServer(80);          // HTTP server

void setup() {
  disableCore0WDT();
  disableCore1WDT();
  Serial.begin(115200);
  Serial.print((temprature_sens_read() - 32) / 1.8);
  Serial.print("\n");
  
  Serial.print("\n");
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
  wifisetup();
  for(;;){
    webServer.handleClient();
    delay(1);
  } 
}
void Task2code( void * pvParameters ){
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), hallChanged, CHANGE);
  for(;;){
    pulseFunction();
  }
}


//#### LOGIC FOR WIFI ######

void wifisetup(){
  // configure access point
  btStop();
  WiFi.mode(WIFI_AP);
  delay(10);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(10);
  WiFi.softAP("atomicspark"); // WiFi name
  delay(10);
 
 webServer.on("/", [](){
  String ptr = "<!DOCTYPE html>";
  ptr +="<html>";
  ptr +="<head>";
  ptr +="<title>Atomic4</title>";
  ptr +="<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr +="<style>";
  ptr +="html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}";
  ptr +="body{margin: 50px;} ";
  ptr +="h1 {margin: 50px auto 30px;} ";
  ptr +=".side-by-side{display: table-cell;vertical-align: middle;position: relative;}";
  ptr +=".text{font-weight: 600;font-size: 19px;width: 200px;}";
  ptr +=".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}";
  ptr +=".rpm .reading{color: #F29C1F;}";
  ptr +=".dwell .reading{color: #3B97D3;}";
  ptr +=".advancetach .reading{color: #26B99A;}";
  ptr +=".superscript{font-size: 17px;font-weight: 600;position: absolute;top: 10px;}";
  ptr +=".rpmdata{padding: 10px;}";
  ptr +=".container{display: table;margin: 0 auto;}";
  ptr +="</style>";
  ptr +="<script>\n";
  ptr +="setInterval(loadDoc,100);\n";
  ptr +="function loadDoc() {\n";
  ptr +="var xhttp = new XMLHttpRequest();\n";
  ptr +="xhttp.onreadystatechange = function() {\n";
  ptr +="if (this.readyState == 4 && this.status == 200) {\n";
  ptr +="document.body.innerHTML =this.responseText}\n";
  ptr +="};\n";
  ptr +="xhttp.open(\"GET\", \"/\", true);\n";
  ptr +="xhttp.send();\n";
  ptr +="}\n";
  ptr +="</script>\n";
  ptr +="</head>";
  ptr +="<body>";
  ptr +="<h1>Atomic RPM</h1>";
  ptr +="<div class='container'>";
  ptr +="<div class='rpmdata rpm'>";
  ptr +="<div class='side-by-side icon'>";;
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>rpm</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)rpmtach;
  ptr +="</div>";
  ptr +="<div class='container'>";
  ptr +="<div class='advancetach'>";
  ptr +="<div class='side-by-side icon'>";;
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>advance</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)advancetach;
  ptr +="</div>";
  ptr +="<div class='container'>";
  ptr +="<div class='dwell'>";
  ptr +="<div class='side-by-side text'>dwell</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)dwell;
  ptr +="</div>";
  ptr +="<div class='container'>";
  ptr +="<div class='dwell'>";
  ptr +="<div class='side-by-side text'>CPU temp</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)cputemp;
  ptr +="</div>";
  ptr +="<div class='container'>";
  ptr +="<div class='revlimit'>";
  ptr +="<div class='side-by-side text'>RPM limiter</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=revlimit;
  ptr +="</div>";
  ptr +="<div class='container'>";
  ptr +="<div class='missfire'>";
  ptr +="<div class='side-by-side text'>Missfire</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=missfire;
  ptr +="</div>";
  ptr +="</body>";
  ptr +="</html>";
  webServer.send(200, "text/html", ptr);
  });
 
 webServer.begin();
 Serial.println("Web server started!");
}

//#### LOGIC FOR IGNITION ######

// interrupt triggers, turn off going high on coil pack(may be redundant),ser time of interrupt, newpulse and magnet polarity.
void hallChanged() 
{
      GPIO.out_w1tc = (1 << bank1);
      GPIO.out_w1tc = (1 << bank2);
      GPIO.out_w1tc = (1 << tach);
  
  latestPulseMicros = micros();
  newPulse = true;  
  //Serial.print(" \n interrupt ");
  if(digitalRead(INTERRUPT_PIN) == 0){
    magnet = true;
   }
  else{
    magnet = false;
   }
}

// Delay for 1/2 revolution before dwell starts
void delayfunc() 
{     
     while (micros() - latestPulseMicros <= (ignDelay - dwell) && (newPulse == false) ) {
    }
}

//duration of dwell
void delaydwellfunc() 
{ 
    dwellMicros = micros();
    while ((micros() - dwellMicros <= dwell) && (newPulse == false)) {
      }
    }


//If magnet polarity change, wait 1/2 rpm duration, pin up for dwell duration, then pin down. 
void magnetfunction()  // Function to fire the banks
{
  
  if (magnet != previousMagnet){
    rpmDebug++; // for debugging
    if (magnet == true){ 
      previousMagnet = magnet;
      if ((rpm>=200) && (rpm<=2400) && (inRange == true))
        {
      delayfunc();
      revlimit = "false";
      GPIO.out_w1ts = (1 << bank1);
      GPIO.out_w1ts = (1 << tach);
      delaydwellfunc();
      GPIO.out_w1tc = (1 << bank1);
      GPIO.out_w1tc = (1 << tach);
        }
    }
    else {
      previousMagnet = magnet;
      if ((rpm>=200) && (rpm<=2400) && (inRange == true))
        {
      delayfunc();
      GPIO.out_w1ts = (1 << bank2);
      GPIO.out_w1ts = (1 << tach);
      delaydwellfunc();
      GPIO.out_w1tc = (1 << bank2);
      GPIO.out_w1tc = (1 << tach);}
      else{
               revlimit = "true";
      Serial.print("revlimiter");
        }
    }      
  }
  else {
  }
}

// Check the last 1/2 rpm is not too much faster or slower than the latest.
void inRangefunc()
{
  if ((revMicros>(prerevMicros * 0.60)) && revMicros<(prerevMicros * 1.10)) // Left is Acceleration limit. Right is deAccelration
{
  inRange = true;
  //Serial.print(" \n Within Range");
}
else
{
  missfire++;
  inRange = false;
  /* for debugging
  Serial.print(" \n Range Exceeded NO FIRE");  // debugging
  Serial.print(", prerevMircros = ");
  Serial.print(prerevMicros);
  Serial.print(", igndelay = ");
  Serial.print(ignDelay);
  Serial.print(", revMicros = ");
  Serial.print(revMicros);
  Serial.print(", as a percent = ");
  Serial.print((float)revMicros/(float)prerevMicros);
  */
}
}

/*
Upon newPulse from interrupt.
last 1/2 rpm time is set to prerevMicros for inrange func to utilise.
latest 1/2 rpm time is recorded and checked for duration limits.

*/
void pulseFunction()
{
      if (newPulse == true) {
      newPulse = false;
       
      prerevMicros=revMicros; // In range function
      
      revMicros = latestPulseMicros - prevPulseMicros;   // revMicros equals the new pulse just taken minus the old revmicros
      if ((revMicros < 7000) || (revMicros > 210000)){
        Serial.print(" \n REVMICROS redacted  ");
        Serial.print(revMicros);
        //Serial.print("\n RPM =  ");
        //Serial.print(rpm);
        revMicros = 300000;
      }
      
      prevPulseMicros = latestPulseMicros; //latest pulse time is recrded for next time.

      //Check for wild variation
      inRangefunc();
      
      //Reset dwell
      dwell = 3000;

      // calculate RPM based on time between pulses
      rpm = (revMicros*2)/1000;
      rpm = (1000/rpm)*60;
      rpmtach = (revMicros*2)/1000;
      rpmtach = (1000/rpmtach)*60;
      if (rpmtach < 101){
        rpmtach = 0;
      }
          
      
      cputemp = ((temprature_sens_read() - 32) / 1.8); 
      
      advancetach = 180-(180*(float)advancePercentage[advanceKey]);

      //Using rpm select the advance key for use with advance array.
      advanceKey = round(rpm/100);
      if ((rpm < 500) && (advanceKey > 10))  { // Probably not needed due to the revMicros Reset.
       Serial.print(" \n ADVANCE KEY redacted");
        advanceKey = 2;
      }

      //Adjust length of dwell.
      dwell = (dwell * dwellPercentage[advanceKey]);

      //Calculate thewait time before pin up. 
      ignDelay = (revMicros * (advancePercentage[advanceKey]));
      ignDelay = ignDelay + ignAdjust; 

      //With timing calculated run the magnet polarity check and pinup/down appropriately.
      magnetfunction();
  }
}

void loop() {
      
}
