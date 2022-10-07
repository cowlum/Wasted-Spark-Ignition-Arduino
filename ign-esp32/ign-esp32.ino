

//Hall Effect Sensor Settings
#define INTERRUPT_PIN 21

//Put hall changed function into memory
void ICACHE_RAM_ATTR hallChanged();

//Timing calc variables
volatile bool newPulse = false;
volatile unsigned long int latestPulseMicros; 
unsigned long int prevPulseMicros;
unsigned long int revMicros = 150000;
unsigned long int prerevMicros = 150000;
unsigned long int ignDelay = 2500000;
unsigned long int dwellMicros;

long int ignAdjust = 500; //timing adjestment. Should be zero when hall effect in correct position.

bool inRange = true;
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
  0.950,0.940,0.940,0.940,0.940,0.940,0.940,0.940,0.939,0.933,0.933,0.928,0.922,0.917,0.911,0.906,0.900,0.894,0.889,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.900,0.917,0.944,0.950,0.950,0.950,0.950
  }; 
//0                                                     10                             15                            20                            25                            30                           35                            40                            45        
float dwellPercentage[] = {
  1.300,1.300,1.300,1.300,1.300,1.250,1.250,1.250,1.200,1.100,1.050,1.000,1.000,1.000,1.000,0.990,0.980,0.970,0.960,0.950,0.940,0.930,0.920,0.910,0.900,0.890,0.880,0.870,0.860,0.850,0.840,0.830,0.820,0.810,0.800,0.790,0.780,0.770,0.760,0.750,0.740,0.730,0.720,0.710,0.700,0.700
  }; 

void setup()
{
  pinMode(bank1, OUTPUT); 
  pinMode(bank2, OUTPUT);
  pinMode(tach, OUTPUT);
  pinMode(relay, OUTPUT);
  Serial.begin(115200);
  Serial.print("The interupt is about to begin /n");
  digitalWrite(relay, HIGH);
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), hallChanged, CHANGE);
}

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
      GPIO.out_w1tc = (1 << tach);
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
  inRange = false;
  Serial.print(" \n Range Exceeded NO FIRE");  // debugging
  Serial.print(", prerevMircros = ");
  Serial.print(prerevMicros);
  Serial.print(", igndelay = ");
  Serial.print(ignDelay);
  Serial.print(", revMicros = ");
  Serial.print(revMicros);
  Serial.print(", as a percent = ");
  Serial.print((float)revMicros/(float)prerevMicros);

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


void loop()
{
pulseFunction();  
    /*
      if (rpmDebug == 100){
        rpmDebug=1;
        Serial.print("\n");
        Serial.print("\n RPM =  ");
        Serial.print(rpm);
        Serial.print("\n percentage ignDelay/revMicros  ");
        Serial.print((float)ignDelay/(float)revMicros)*100;
        Serial.print("\n degrees advance ");
        Serial.print(180-(180*(float)advancePercentage[advanceKey])); //125 degress because sensor is 55 (45-10)  degrees ATDC.
        Serial.print("\n revmicros  ");
        Serial.print(revMicros);
        Serial.print("\n igndelay  ");
        Serial.print(ignDelay);
        //Serial.print("\n Dwell  ");
        //Serial.print(dwell);
        //Serial.print(" \n prerevMircros ");
        //Serial.print(prerevMicros);
        //Serial.print(" \n  latestPulseMicros  ");
        //Serial.print(latestPulseMicros);
        //Serial.print(" \n AdvanceKey  ");
        //Serial.print(advanceKey);
        //Serial.print(" \n PrepulsMicros  ");
        //Serial.print(prevPulseMicros);
        //Serial.print("\n as a percent = ");
        //Serial.print((float)revMicros/(float)prerevMicros);
        //Serial.print("\n ledbuiltint ");
        //Serial.print(ledbit);
      }
      //*/    
  }

  
