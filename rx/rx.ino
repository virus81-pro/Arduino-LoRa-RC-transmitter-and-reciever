/***************************************************************************************************
* Code for receiver microcontroller
* BUK 2020
* buk7456@gmail.com

* Tested to compile on Arduino IDE 1.8.9 or later
* Sketch should compile without warnings even with -WALL
***************************************************************************************************/


//Comment to disable serial print out
// #define DEBUG

#include <SPI.h>
#include "LoRa.h"
// NOTE: This library exposes the LoRa radio directly, and allows you to send data to 
//       any radios in range with same radio parameters. 
//       All data is broadcasted and there is no addressing. 
//       Any LoRa radio that are configured with the same radio parameters and in range 
//       can see the packets you send.
  
#include "crc16.h"

//----------------------------------------------
//Pins
#define CH1PIN 2
#define CH2PIN 5
#define CH3PIN 3
#define CH4PIN 4
#define CH5PIN A5
#define CH6PIN A4
#define CH7PIN A3
#define CH8PIN A2
#define CH9PIN A1
#define CH10PIN A0
#define GREEN_LED_PIN 7
#define ORANGE_LED_PIN 6

//-----------------------------------------------
//the servo pulse range. ie between maximum and minimum 0 to 180 degrees
#define SERVOPULSERANGE 1000
#define SERVO_MAX_ANGLE_FROM_CENTER 100
//initially was 50 but  these micro servos (tower pro) only covered 50 entirely 

#include <Servo.h>
Servo servoCh1;
Servo servoCh2;
Servo servoCh3;
Servo servoCh4;
Servo servoCh5;
Servo servoCh6;
Servo servoCh7;
Servo servoCh8;

bool servosAreAttached = false;
long lowerLimMicroSec;
long upperLimMicroSec;

//--------------------------------------------------


bool radioInitialised = false;
bool hasValidPacket = false;
unsigned long lastValidPacketMillis = 0;


#define MCURXBUFFSIZE 20
uint8_t rxBuffer[MCURXBUFFSIZE]; //buffer to store the incoming message bytes

long ch1to8Vals[8] = {0,0,0,0,0,0,0,0};
uint8_t digitalChVal[2] = {0,0}; //channels 9 and 10

bool failsafeBeenReceived = false;
long ch1to8Failsafes[8] = {0,0,0,0,0,0,0,0};

long rxPacketCount = 0;         //debug 
long validPacketCount = 0;      //debug
uint8_t validPacketsPerSecond;  //debug 
unsigned long prevValidPacketCount = 0; //debug
int rssi = 0;  //debug


//==================================================================================================
void setup()
{ 
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, HIGH);
  pinMode(ORANGE_LED_PIN, OUTPUT);
  digitalWrite(ORANGE_LED_PIN, LOW);
  pinMode(CH9PIN, OUTPUT);
  digitalWrite(CH9PIN, LOW);
  pinMode(CH10PIN, OUTPUT);
  digitalWrite(CH10PIN, LOW);
  
  lowerLimMicroSec = SERVOPULSERANGE/2;
  lowerLimMicroSec *= SERVO_MAX_ANGLE_FROM_CENTER;
  lowerLimMicroSec /= 90;
  lowerLimMicroSec = 1500 - lowerLimMicroSec;
  
  upperLimMicroSec = SERVOPULSERANGE/2;
  upperLimMicroSec *= SERVO_MAX_ANGLE_FROM_CENTER;
  upperLimMicroSec /= 90;
  upperLimMicroSec += 1500;
  
#if defined (DEBUG)
  Serial.begin(115200);
#endif
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /// Override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(10, 8, 2); //pin 2 actually not used unless we have a callback 

  if (LoRa.begin(433E6))
  {
    LoRa.setSpreadingFactor(7); //default is 7
  
    LoRa.setSignalBandwidth(250E3);
    // signalBandwidth - signal bandwidth in Hz, defaults to 125E3.
    // Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, and 250E3.
    
    LoRa.setCodingRate4(5);
    /*codingRateDenominator - denominator of the coding rate, defaults to 5
    Supported values are between 5 and 8, these correspond to coding rates of 4/5 and 4/8. 
    The coding rate numerator is fixed at 4. */
  
    radioInitialised = true;
  }
  else
    radioInitialised = false;

}

//====================================== MAIN LOOP =================================================
void loop()
{
  ///-------- READ AND VERIFY, THEN DECODE --------------
  readAndVerifyPacket();
  decodeData(); 
  
  /// -------- CHECK TIME SINCE LAST VALID PACKET ------
  uint32_t ttPktElapsed = millis() - lastValidPacketMillis;
  if(ttPktElapsed > 80) //No valid packet for more than these ms, turn off orange LED
    digitalWrite(ORANGE_LED_PIN, LOW); 

  //No valid packet for more than these milliseconds, trigger fail safes
  if(failsafeBeenReceived == true && ttPktElapsed > 1500) 
  {
    rssi = 0;
    
    for(int i= 0; i < 8; i++)
    {
      if(ch1to8Failsafes[i] != 523) //ignore channels that have failsafe turned off.
        ch1to8Vals[i] = ch1to8Failsafes[i]; 
    }
    digitalChVal[0] = 0; //turn off momentary toggle channel A (ch9)
  }
  
  
  /// ----------- Attach servos ----------------------
  if(failsafeBeenReceived == true && servosAreAttached == false)
  {
    servoCh1.attach(CH1PIN);
    servoCh2.attach(CH2PIN);
    servoCh3.attach(CH3PIN);
    servoCh4.attach(CH4PIN);
    servoCh5.attach(CH5PIN);
    servoCh6.attach(CH6PIN);
    servoCh7.attach(CH7PIN);
    servoCh8.attach(CH8PIN);
    
    servosAreAttached = true;
  }
  
  /// ---------- Write outputs -----------------------
  if(servosAreAttached == true)
  {
    int16_t ch1to8MicroSec[8];
    for(uint8_t i = 0; i<8; i++)
    {
      ch1to8MicroSec[i] = map(ch1to8Vals[i],-500,500,lowerLimMicroSec, upperLimMicroSec);
    }
    servoCh1.writeMicroseconds(ch1to8MicroSec[0]);
    servoCh2.writeMicroseconds(ch1to8MicroSec[1]);
    servoCh3.writeMicroseconds(ch1to8MicroSec[2]);
    servoCh4.writeMicroseconds(ch1to8MicroSec[3]);
    servoCh5.writeMicroseconds(ch1to8MicroSec[4]);
    servoCh6.writeMicroseconds(ch1to8MicroSec[5]);
    servoCh7.writeMicroseconds(ch1to8MicroSec[6]);
    servoCh8.writeMicroseconds(ch1to8MicroSec[7]);
    
    digitalWrite(CH9PIN,  digitalChVal[0]);
    digitalWrite(CH10PIN, digitalChVal[1]);
  }
  
  //--------------------------------------------------

  
#if defined (DEBUG)
  //Calculate packets per second
  static unsigned long ttPrevMillis = 0;
  unsigned long ttElapsed = millis() - ttPrevMillis;
  if (ttElapsed >= 1000)
  {
    ttPrevMillis = millis();

    unsigned long _validPPS = (validPacketCount - prevValidPacketCount) * 1000;
    _validPPS /= ttElapsed;
    validPacketsPerSecond = uint8_t(_validPPS);
    prevValidPacketCount = validPacketCount;
  }
   
  static uint32_t serialLastPrintTT = millis();
  if(millis() - serialLastPrintTT >= 40)
  {
    serialLastPrintTT = millis();
    Serial.print("RSSI:");
    Serial.print(rssi);
    Serial.print(" PPS:");
    Serial.print(validPacketsPerSecond);
    Serial.print(" Ch: ");
    Serial.print(ch1to8Vals[0]/5);  //Ch1
    Serial.print(F(" "));
    Serial.print(ch1to8Vals[1]/5);  //Ch2
    Serial.print(F(" "));
    Serial.print(ch1to8Vals[2]/5);  //Ch3
    Serial.print(F(" "));
    Serial.print(ch1to8Vals[3]/5);  //Ch4
    Serial.print(F(" "));
    Serial.print(ch1to8Vals[4]/5);  //Ch5
    Serial.print(F(" "));
    Serial.print(ch1to8Vals[5]/5);  //Ch6
    Serial.print(F(" "));
    Serial.print(ch1to8Vals[6]/5);  //Ch7
    Serial.print(F(" "));
    Serial.print(ch1to8Vals[7]/5);  //Ch8
    Serial.print(F(" "));
    Serial.print(digitalChVal[0]);  //Ch9
    Serial.print(F(" "));
    Serial.print(digitalChVal[1]);  //Ch10
    Serial.println();
  }

#endif

}

//==================================================================================================
void readAndVerifyPacket()
{
  if(radioInitialised == false)
  {
    return;
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) //received a packet
  {
    rssi = LoRa.packetRssi();
    rxPacketCount += 1;

    /// read packet
    uint16_t cntr = 0;
    bool rxBufferFull = false;
    while (LoRa.available() > 0) 
    {
      if(rxBufferFull == false) //put data into rxBuffer
      {
        rxBuffer[cntr] = LoRa.read();
        cntr += 1;
        if (cntr >= MCURXBUFFSIZE) //prevent array out of bounds
          rxBufferFull = true; 
      }
      else //Throw away any extra data
        LoRa.read(); 
    }
    
    /// Check if the received data is valid based on crc
    
    uint16_t crcQQ = rxBuffer[10] & 0x1F; //extract first 5 crc bits
    crcQQ = crcQQ << 8;
    crcQQ |= rxBuffer[11]; //add next 8 crc bits
    
    rxBuffer[10] &= 0xE0; //remove crc bits from received data
    
    if(crcQQ == (crc16(rxBuffer, 11) & 0x1FFF)) //compare 
    {
      hasValidPacket = true;
      validPacketCount += 1;
      lastValidPacketMillis = millis();
      digitalWrite(ORANGE_LED_PIN, HIGH);
    }
  }
}

//==================================================================================================
void decodeData()
{
  if(hasValidPacket == false)
  {
    return;
  }
  
  /** Air protocol format
     Chs 1 to 8 encoded as 10 bits

     byte  b0       b1      b2        b3       b4       
        11111111 11222222 22223333 33333344 44444444 
        
           b5       b6      b7        b8       b9
        55555555 55666666 66667777 77777788 88888888

           b10      b11
        abfCCCCC   CCCCCCCC
        
     a is digital ch9, b digital Ch10,  f is failsafe flag
     
     CCCCC CCCCCCCC is the 13 bit CRC (calculated as CRC16)   
  */
  
  //Channels 1 to 8 (rxBuffer[0] to rxBuffer[9])
  
  long _ch1to8Tmp[8];
  
  _ch1to8Tmp[0] = ((uint16_t)rxBuffer[0] << 2 & 0x3fc) | ((uint16_t)rxBuffer[1] >> 6 & 0x03); //ch1
  _ch1to8Tmp[1] = ((uint16_t)rxBuffer[1] << 4 & 0x3f0) | ((uint16_t)rxBuffer[2] >> 4 & 0x0f); //ch2
  _ch1to8Tmp[2] = ((uint16_t)rxBuffer[2] << 6 & 0x3c0) | ((uint16_t)rxBuffer[3] >> 2 & 0x3f); //ch3
  _ch1to8Tmp[3] = ((uint16_t)rxBuffer[3] << 8 & 0x300) | ((uint16_t)rxBuffer[4]      & 0xff); //ch4
 
  _ch1to8Tmp[4] = ((uint16_t)rxBuffer[5] << 2 & 0x3fc) | ((uint16_t)rxBuffer[6] >> 6 & 0x03); //ch5
  _ch1to8Tmp[5] = ((uint16_t)rxBuffer[6] << 4 & 0x3f0) | ((uint16_t)rxBuffer[7] >> 4 & 0x0f); //ch6
  _ch1to8Tmp[6] = ((uint16_t)rxBuffer[7] << 6 & 0x3c0) | ((uint16_t)rxBuffer[8] >> 2 & 0x3f); //ch7
  _ch1to8Tmp[7] = ((uint16_t)rxBuffer[8] << 8 & 0x300) | ((uint16_t)rxBuffer[9]      & 0xff); //ch8
  
  
  //Channels 9 to 10
  digitalChVal[0] = (rxBuffer[10] & 0x80) >> 7;
  digitalChVal[1] = (rxBuffer[10] & 0x40) >> 6;
  
  

  if((rxBuffer[10] & 0x20) == 0x20) //check failsafe flag. If true, This is failsafe data
  {
    failsafeBeenReceived = true;
    //dont modify outputs
    //save failsafes
    for(int i=0; i<8; i++)
    {
      //Check the failsafe data. In a channel has 1023 as value, then failsafe doesnt apply there
      ch1to8Failsafes[i] = _ch1to8Tmp[i] - 500; //Center around 0
    }
  }
  else //copy
  {
    for(int i=0; i<8; i++)
    {
      ch1to8Vals[i] = _ch1to8Tmp[i] - 500; //Center around 0 so range is -500 to 500
    }
  }

  hasValidPacket = false; //Done with this packet, so set flag to false
}


