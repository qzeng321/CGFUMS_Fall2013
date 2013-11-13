// Citations
//ADXL362_MotionActivatedSleep
//by Anne Mahaffey - hosted on http://annem.github.com/ADXL362
//LowPower library from http://github.com/rocketscream/Low-Power
//SdFat library: https://code.google.com/p/sdfatlib/
//SdFat was chosen because sleep mode current draw is lower than arduino SD library
//
// Libraries 
//
//// For ADXL362 and Motion Activated Sleep of Arduino
#include <LowPower.h> 
#include <SPI.h>
#include <ADXL362.h>

//// For SD card
// Circuit attachment of SD card
// * SD card attached to SPI bus as follows:
// ** MOSI - pin 11
// ** MISO - pin 12
// ** CLK - pin 13
// ** CS - pin 4
#include <SdFat.h>
//
ADXL362 xl; // ADSXL362
SdFat sd;  // SD card
SdFile file; // SD file
//
//  Setup interrupt on Arduino
//  See interrupt example at http://arduino.cc/en/Reference/AttachInterrupt
int interruptPin = 2;          //Setup ADXL362 interrupt output to Interrupt 0 (digital pin 2)
int interruptStatus = 0;
//
int XData, YData, ZData, Temp; // We sense these values
//
// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const uint8_t sdChipSelect = 8; // CGFUMS pin 8 - JP, SD pin
#define ADXLPin 10

void setup(){

    // Startup
    Serial.begin(9600); // Begin serial communication
    //
    //// INITIALIZING ADXL362
    xl.begin();  //begin/reset ADXL362
    delay(1000); //I don't really know why we need this delay in here yet
    //  
    //// INITIALIZING SD CARD
    if (!sd.begin(sdChipSelect)
    || !file.open("DATALOG4.CSV", O_CREAT | O_WRITE | O_APPEND)) {
    // Replace this with somthing for your app.
    Serial.println(F("SD problem"));
    while(1);
    }
    //
    //// SETUP MOTION ACTIVATED SLEEP ON ARDUINO MINI
    //  Setup Activity and Inactivity thresholds
    //     tweaking these values will effect the "responsiveness" and "delay" of the interrupt function
    //     my settings result in a very rapid, sensitive, on-off switch, with a 2 second delay to sleep when motion stops
    xl.setupDCActivityInterrupt(300, 10);		// 300 code activity threshold.  With default ODR = 100Hz, time threshold of 10 results in 0.1 second time threshold
    xl.setupDCInactivityInterrupt(80, 200);		// 80 code inactivity threshold.  With default ODR = 100Hz, time threshold of 30 results in 2 second time threshold
    Serial.println();
    //
    // SETUP ADXL362 FOR PROPER AUTOSLEEP MODE
	
    // Map Awake status to Interrupt 1
    xl.SPIwriteOneRegister(0x2A, 0x40);   
	
    // Setup Activity/Inactivity register
    xl.SPIwriteOneRegister(0x27, 0x3F); // Referenced Activity, Referenced Inactivity, Loop Mode  
        
    // turn on Autosleep bit
    byte POWER_CTL_reg = xl.SPIreadOneRegister(0x2D);
    POWER_CTL_reg = POWER_CTL_reg | (0x04);				// turn on POWER_CTL[2] - Autosleep bit
    xl.SPIwriteOneRegister(0x2D, POWER_CTL_reg);
 
    // turn on Measure mode
    //
    xl.beginMeasure();                      // DO LAST! enable measurement mode   
    xl.checkAllControlRegs();               // check some setup conditions    
    delay(100);
    //
    // SETUP INTERRUPT FUNCTION ON ARDUINO MINI
    //    IMPORTANT - Do this last in the setup, after you have fully configured ADXL.  
    //    You don't want the Arduino to go to sleep before you're done with setup
    pinMode(2, INPUT);    
    attachInterrupt(0, interruptFunction, RISING);  // A high on output of ADXL interrupt means ADXL is awake, and wake up Arduino 
    //
}

    void measureADXL(){
      digitalWrite(ADXLPin, LOW);
      SPI.transfer(0x0B);  // read instruction
      SPI.transfer(0x0E);  // Start at XData Reg
      XData = SPI.transfer(0x00);
      XData = XData + (SPI.transfer(0x00) << 8);
      YData = SPI.transfer(0x00);
      YData = YData + (SPI.transfer(0x00) << 8);
      ZData = SPI.transfer(0x00);
      ZData = ZData + (SPI.transfer(0x00) << 8);
      Temp = SPI.transfer(0x00);
      Temp = Temp + (SPI.transfer(0x00) << 8);
      digitalWrite(ADXLPin, HIGH);
    }

void loop(){
  //
  //  Check ADXL362 interrupt status to determine if it's asleep
  interruptStatus = digitalRead(interruptPin);
  
  // if ADXL362 is asleep, call LowPower.powerdown  
  if(interruptStatus == 0) { 
    Serial.print("\nADXL went to sleep - Put Arduino to sleep now \n");
    delay(100);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);     
  }
  
  // if ADXL362 is awake, report XYZT data to Serial Monitor
  else{
    delay(10); //why is this delay needed?
    
    measureADXL();   
    //// WRITE DATA TO SD CARD
    String dataString = String(XData)+ ' ' + String(YData) + ' ' + String(ZData); //For ON duty cycle 
//                      // Can store varying amounts of data
//                      // String(XValue)+ ' ' + String(YValue) + ' ' + String(ZValue);	
//    
//    // datalog.csv has already been opened in setup
//    // if the file is available, write to it:
//    //if (file) {
//// SCHEMA1: STORING RAW DATA
      file.print(XData);
      file.print(",");
      file.print(YData);
      file.print(",");
      file.println(ZData);

      file.sync();
//      // print to the serial port too:
      Serial.println(dataString);
    //}  
    // if the file isn't open, pop up an error:
    //else {
     // Serial.println("error opening datalog.csv");
    //}     
  }
}


// Function called if Arduino detects interrupt activity
//    when rising edge detected on Arduino interrupt
// Interrupt service routine (takes no parameters and returns nothing)
void interruptFunction(){
  Serial.println("\nArduino is Awake! \n");
}

