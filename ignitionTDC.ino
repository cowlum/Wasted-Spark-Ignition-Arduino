//W 45 DEGREES OFFSET
// Saturday 24th.
// * Retard the timing by a few degrees.
// Possibly code a starting sequence
// try changing to 160MHz to stabalize timing

//Hall Effect Sensor Settings
#define INTERRUPT_PIN D2
void ICACHE_RAM_ATTR hallChanged();
long int latestPulseMicros;
long int prevPulseMicros;
unsigned long int revMicros;
unsigned long int prerevMicros;
unsigned long int ignDelay;
unsigned long int ignAdjust = 1;
float calcAdvance;
bool newPulse = false;
volatile bool magnet;
bool previousMagnet;
bool inRange;

unsigned long int currentMicros;
unsigned long int dwellMicros;

//BANK OUTPUTS
//int led1 = 15;
//int led2 = 12;
#define bank1 15
#define bank2 12
#define bank1bit (1<<15)
#define bank2bit (1<<12)
long int rpm = 200;
long int rpmDebug = 98;
unsigned long int dwell = 3000;

//Advance
int advanceKey = 2;
float advancePercentage[] = {
  0.983,0.983,0.983,0.972,0.961,0.956,0.950,0.944,0.939,0.933,0.933,0.928,0.922,0.917,0.911,0.906,0.900,0.894,0.889,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.883,0.900,0.917,0.944,0.972,0.972
  }; 
// 0                                                     10                             15                            20                            25                            30                           35                            40                            45        
float dwellPercentage[] = {
  1.300,1.300,1.300,1.300,1.300,1.250,1.250,1.250,1.200,1.100,1.050,1.000,1.000,1.000,1.000,0.990,0.980,0.970,0.960,0.950,0.940,0.930,0.920,0.910,0.900,0.890,0.880,0.870,0.860,0.850,0.840,0.830,0.820,0.810,0.800,0.790,0.780,0.770,0.760,0.750,0.740,0.730,0.720,0.710,0.700,0.700
  }; 

void setup()
{
  pinMode(bank1, OUTPUT); 
  pinMode(bank2, OUTPUT);
  Serial.begin(115200);
  Serial.print("The interupt is about to begin /n");
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), hallChanged, CHANGE);
}

void hallChanged()
{
  WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + 8, bank1bit );
  WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + 8, bank2bit );
  latestPulseMicros = micros();
  newPulse = true;  
  if(digitalRead(INTERRUPT_PIN) == 0){
    magnet = true;
   }
  else{
    magnet = false;
   }
}

void delayfunc()
{
   currentMicros = micros();
     while(currentMicros - latestPulseMicros <= (ignDelay - dwell)) {
     currentMicros = micros();
     }
}

void delaydwellfunc()
{ 
  currentMicros = micros();
  dwellMicros = currentMicros;
    while (currentMicros - dwellMicros <= dwell) {
    currentMicros = micros();
    }
}

void magnetfunction()  // Function to fire the banks
{
  if (magnet != previousMagnet){
    rpmDebug++;
    if (magnet == true){ 
      previousMagnet = magnet;
      if ((rpm>=100) && (rpm<=2100) && (inRange == true))
        {
      delayfunc();
      //delayMicroseconds(dwell);
      delaydwellfunc();
      WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + 8, bank2bit );
        }
    }
    else {
      previousMagnet = magnet;
      if ((rpm>=200) && (rpm<=2100) && (inRange == true))
        {
      delayfunc();
      WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + 4, bank1bit );
      //delayMicroseconds(dwell); 
      delaydwellfunc();
      WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + 8, bank1bit );
        }
        }
  }
  else {
  }
}

void pulseFunction()
{
      if (newPulse == true) {

      prerevMicros=revMicros; // In range function last revolution timing is recorded for the in range function
      
      revMicros = latestPulseMicros - prevPulseMicros; // revMicros equals the new pulse just taken minus the old revmicros
      prevPulseMicros = latestPulseMicros; //
      newPulse = false;
      
      //Check for wild variation
      inRangefunc();
      
      //Reset dwell
      dwell = 3000;

      // calculate RPM based on time between pulses
      ignDelay = (revMicros * (advancePercentage[advanceKey]));
      ignDelay = ignDelay + ignAdjust;
      dwell = (dwell * dwellPercentage[advanceKey]);
      rpm = (revMicros*2)/1000;
      rpm = (1000/rpm)*60;
      advanceKey = round(rpm/100);
      magnetfunction();
      //advanceKey = advanceKey + ignAdjust;
  }
}

void inRangefunc()
{
  if ((revMicros>(prerevMicros * 0.10)) && revMicros<(prerevMicros * 1.10)) // UPDATE NEEDED: if rpm above 400 then variable width can shrink
{
  inRange = true;
  //Serial.print(" \n Within Range");
}
else
{
  inRange = false;
  Serial.print(" \n Range Exceeded NO FIRE");
}
}
void loop()
{

    pulseFunction();  
      

    // DEBUG
      if (rpmDebug == 100){
        rpmDebug=1;
        calcAdvance = ((float)ignDelay/(float)revMicros);
        Serial.print("\n");
        Serial.print("\n RPM =  ");
        Serial.print(rpm);
        //Serial.print("\n percentage ignDelay/revMicros");
        //Serial.print((float)ignDelay/(float)revMicros)*100;
        Serial.print("\n degrees advance past TDC (minus 45 sensor) ");
        Serial.print(135-(180*(float)advancePercentage[advanceKey])); //125 degress because sensor is 55 (45-10)  degrees ATDC.
        Serial.print("\n revmicros  ");
        Serial.print(revMicros);
        Serial.print("\n igndelay  ");
        Serial.print(ignDelay);
        Serial.print("\n Dwell  ");
        Serial.print(dwell);
        Serial.print(" \n prerevMircros ");
        Serial.print(prerevMicros);
        Serial.print(" \n Calculated advance ");
        Serial.print(calcAdvance);
      }    
  }
