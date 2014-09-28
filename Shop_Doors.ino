/*
Author: Shannon Moore
Based on: UberSensor.ino by Eric Tsai
Date: 9-28-2014
File: Shop_Doors.ino
This sketch is for a wired Arduino w/ RFM69 wireless transceiver
Sends sensor data: Contact switch open and close on front and back shop doors, immediatly
                   PIR motion sensor movement detected, immediatly - then resets after about 15 seconds
                   Temperature from the sensor onboard the RFM69HW, every 6 minutes of so
                   RSSI of the node, every 6 minutes of so (when the temperature is sent)
*/








/* sensor

MQTT topic format:   node# + device ID + 1=millis, 2=sensor data, 3=sensor data or 4=RSSI

deviceID     MQTT topic    Sensor variable description          Pin Setup


4            xx42          PIR                                                    D7
6            xx62          temperature                                            no pin, sensor onboard RMF69
9            xx92          1,2 = switch one on, off; 3,4 = switch 2 on,off        D3,D5

*/











// ------------------------------------------------  Includes  ----------------------------------------------------------
#include <RFM69.h>                         // RMF69 tranceiver livrary
#include <SPI.h>                           // SPI library



// ----------------------------------------  Variables and Constants  --------------------------------------------------
// RFM69  Tranciever  --------------------------------------------------------------------------------------------------
#define NODEID        15                   // unique for each node on same network
#define NETWORKID     101                  // the same on all nodes that talk to each other
#define GATEWAYID     1

// Match frequency to the hardware version of the radio on your Moteino (uncomment one):
   #define FREQUENCY   RF69_433MHZ
// #define FREQUENCY   RF69_868MHZ
// #define FREQUENCY   RF69_915MHZ

#define ENCRYPTKEY    "1111111111111111"  // exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW                        // uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ACK_TIME      30                  // max # of ms to wait for an ack
#define LED           9                   // Moteinos have LEDs on D9
#define SERIAL_BAUD   9600                // must be 9600 for GPS, use whatever if no GPS
const int ledPin = 13;                    // the pin that the LED is attached to


//struct for wireless data transmission
typedef struct {		
  int       nodeID; 		          // node ID (1xx, 2xx, 3xx)
  int       deviceID;		          // sensor ID (2, 3, 4, 5)
  unsigned long   var1_usl; 		  // uptime in ms
  float     var2_float;   	          // sensor data?
  float     var3_float;		          // battery condition?
} Payload;
Payload theData;

char buff[20];
byte sendSize=0;
boolean requestACK = false;
RFM69 radio;

// device 4 - PIR sensor  =============================================================================
int PirInput             = 7;                             // digital pin we're connected to
int PIR_status           = 1;
int PIR_reading          = 1;
int PIR_reading_previous = 1;
unsigned long pir_time;                                   // stores the current millis for the PIR sensor

// device 6 - RFM69HW onboard temperature =============================================================
unsigned long temperature_time;                           // stores the current millis for the temperature send timer

// device 9 - magnetic contact switches  ==============================================================
int sensorPin1       = 3;                                 // connect door 1 switch to this pin and ground
int magnetSensor1    = 1;                                 //  
int switchCounter1   = 0;                                 // counter for the number of button presses
int switchState1     = 1;                                 // current state of the button
int lastSwitchState1 = 1;                                 // previous state of the button

int ledpin1          = 4;                                 // LED turns on when door is closed

int sensorPin2       = 5;                                 // connect door 1 switch to this pin and ground
int magnetSensor2    = 1;  
int switchCounter2   = 0;                                 // counter for the number of button presses
int switchState2     = 1;                                 // current state of the button
int lastSwitchState2 = 1;                                 // previous state of the button

int ledpin2          = 6;                                 // LED turns on when door is closed
int readSensor1      = 1;



void setup(){

  Serial.begin(9600);

  // PIR sensor
  pinMode(PirInput, INPUT);
  pir_time = millis();

  // set the temperature time to current millis 
  temperature_time = millis(); 
 
  // Initialize switch pins
  pinMode(sensorPin1, INPUT);
  digitalWrite(sensorPin1, HIGH);

  pinMode(sensorPin2, INPUT);
  digitalWrite(sensorPin2, HIGH);
  
  // Initialize LED pins
  pinMode(ledpin1, OUTPUT);                            // door 1 open / closed indicator
  pinMode(ledpin2, OUTPUT);                            // door 2 open / closed indicator


  // Initialize RFM69-------------------------------------------
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HW
    radio.setHighPower();                              //uncomment only for RFM69HW!
  #endif
  radio.encrypt(ENCRYPTKEY);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  theData.nodeID = NODEID;
  //end RFM--------------------------------------------
  
  delay(200);                        // delay a bit to make sure the RFM68 setup is done before continuing
}

void loop() {

// PIR sensor  -------------------------------------------------------------------------------------------------

  PIR_reading = digitalRead(PirInput);

 
    if (PIR_reading == 1)
     Serial.println("PIR = 1");
    else
     Serial.println("PIR = 0");
  delay (1000);
  
   //send PIR sensor value only if presence is detected and the last time
  //presence was detected is over x miniutes ago.  Avoid excessive RFM sends

      if ((PIR_reading == 1) && ( ((millis() - pir_time)>60000)||( (millis() - pir_time)< 0)) ) 
	{
		
                theData.deviceID = 4;
		theData.var1_usl = millis();
		theData.var2_float = PIR_reading;
		theData.var3_float;		//null value;
		radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
                
                Serial.println("Device 4, PIR detected RFM");
                
                delay(2000);
                
      		pir_time = millis();  //update pir_time with when sensor value last transmitted
          }     //end PIR transmit routine
   
   
   
    
// Read the RFM69 onboard temperature sensor  ------------------------------------------------------------------------

  unsigned long time_passed = 0;      // variable that stores millis - xxx_time, related to the timing of when the sensors run

  time_passed = millis() - temperature_time;
  if (time_passed < 0)
  {
	temperature_time = millis();
  }
  
  if (time_passed > 360000)                              // sets the interval when the read and send temperature block runs
  {
      byte temperature =  radio.readTemperature(+5); // +5 = user cal factor, adjust for correct ambient
      byte fTemp = 1.8 * temperature + 32; // 9/5=1.8

     // Transmit the data
      theData.deviceID = 6;
      theData.var1_usl = millis();
      theData.var2_float = temperature;                    // temperature in centigrade
      theData.var3_float = fTemp;                          // temperature in farenheight
      radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));

     delay(500);                        // delay a bit to make sure the transmission is done before continuing

      Serial.print( "RFM69HW Radio Temp is ");
      Serial.print(temperature);
      Serial.print("C, ");
      Serial.print(fTemp); //converting to F loses some resolution, obvious when C is on edge between 2 values (ie 26C=78F, 27C=80F)
      Serial.println('F');
    temperature_time = millis();

  }
else{}                                                   // not time for temp sensor read yet, so move on


// Check magnetic switch #1  ---------------------------------------------------------------------------------

if (readSensor1 == 1)                                    // this is changed to 0 after this switch is read, so the program reads switch 2 instead next time through the loop
  {
  switchState1 = digitalRead(sensorPin1);                // read the magnetic switch input pin
  if (switchState1 != lastSwitchState1)                  // compare the switchState1 to its previous state, if changed, then transmit new state
    { 
    
      if (switchState1 == HIGH)                          // reading is HIGH when the magnet is close to the reed switch
        {      
          switchCounter1++;                              // if the state has changed, increment the counter
    
            // Transmit the data
              theData.deviceID = 9;
              theData.var1_usl = millis();
              theData.var2_float = 1;                    // 1 = magnet far from switch (door open)
              theData.var3_float = switchCounter1;       // switch toggle counter
              radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
    
              delay(500);                                // delay a bit to make sure the transmission is done before continuing
             
              digitalWrite(ledpin1, LOW);                // turn off LED when doors are open

              Serial.print("Device 9, RFM, Door 1 Open - ");
              Serial.print("number of times:  ");
              Serial.println(switchCounter1);
        
      } 
    else {

          // Transmit the data
              theData.deviceID = 9;
              theData.var1_usl = millis();
              theData.var2_float = 2;                     // 2 = magnet touching sensor (door closed)
              theData.var3_float = switchCounter1;	  // switch toggle counter
              radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
    
              delay(500);                                // delay a bit to make sure the transmission is done before continuing
      
              digitalWrite(ledpin1, HIGH);                // LED turns on when door is closed   
          
              Serial.println("Device 9, RFM, Door 1 Closed");   // if the current state is HIGH then the magnet is close to the switch
        }
  }
  
  lastSwitchState1 = switchState1;                        // save the current state as the last state, for next time through the loop

  readSensor1 = 0;                                        // sets the readsensor variable to 0 so switch 1 is skipped on the next loop and switch 2 is read instead
  }                                                       // end switch 1 


else                                                      // if switch 1 is not read, read switch 2
{

// Check magnetic switch #2  ---------------------------------------------------------------------------------
switchState2 = digitalRead(sensorPin2);                // read the magnetic switch input pin

  if (switchState2 != lastSwitchState2)                  // compare the switchState2 to its previous state
    { 
    
      if (switchState2 == HIGH)                          // reading is HIGH when the magnet is close to the reed switch
        {      
          switchCounter2++;                              // if the door is opened, increment the counter
    
            // Transmit the data
              theData.deviceID = 9;
              theData.var1_usl = millis();
              theData.var2_float = 3;                    // 3 = magnet far from switch (door open)
              theData.var3_float = switchCounter2;       // switch toggle counter
              radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
    
              delay(500);                                // delay a bit to make sure the transmission is done before continuing
             
              digitalWrite(ledpin2, LOW);                // turn off led when door is open

              Serial.print("Device 9, RFM, Door 2 Open - ");
              Serial.print("number of times:  ");
              Serial.println(switchCounter2);
        
      } 
    else {

          // Transmit the data
              theData.deviceID = 9;
              theData.var1_usl = millis();
              theData.var2_float = 4;                     // 4 = magnet touching sensor (door closed)
              theData.var3_float = switchCounter2;	  // switch toggle counter
              radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
    
              delay(500);                                // delay a bit to make sure the transmission is done before continuing
      
              digitalWrite(ledpin2, HIGH);                // LED turns on when door is closed   
          
              Serial.println("Device 9, RFM, Door 2 Closed");   // if the current state is HIGH then the magnet is close to the switch
        }
  }
  
  lastSwitchState2 = switchState2;                        // save the current state as the last state, for next time through the loop
  readSensor1 = 1;
}

}  // End Void Loop




