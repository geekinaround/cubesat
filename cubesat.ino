#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <Arduino_MKRENV.h>
#include <Arduino_MKRGPS.h>


#include "helper.h"

unsigned long current_time;


void blinkLED(int max_count, int blink_duration)
{
  for(int counter=0; counter<max_count; counter++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(blink_duration/2);
    digitalWrite(LED_BUILTIN, LOW);
    delay(blink_duration/2);
  }
}

void setup()
{
#if defined(DEBUG_SERIAL)
  Serial.begin(115200);
#endif
  Wire.begin();
  SPI.begin();
  pinMode(LED_BUILTIN, OUTPUT);

  if (initializeEnvShield()==true &&
      initializeCamera()==true &&
      initializeGPSShield()==true)
  {
    //from example SD datalogger 
    File dataFile = SD.open("datalog.csv", FILE_WRITE);
    // if the file is available, write to it:
    if (dataFile)
    {
      dataFile.println("arduinoclock,temperature,pressure,humidity,longitude,latitude,altitude,speed,direction,gpstime");
      dataFile.close();

      SerialPrintln("Everything is initialized OK.");
      blinkLED(50, 200);

      current_time = millis();   
    }
  }
  else
  {
    SerialPrintln("Something failed.");
  }
}


void loop()
{ 
  current_time = current_time + 2000;
  delayUntil(current_time);

  //from example SD datalogger 
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile)
  {
    dataFile.print(millis());
    dataFile.print(',');
    //from example MKRENV ReadSensors
    dataFile.print(ENV.readTemperature());
    dataFile.print(',');
    dataFile.print(ENV.readPressure());
    dataFile.print(',');
    dataFile.print(ENV.readHumidity());
    dataFile.print(',');
    dataFile.print(GPS.longitude(), 6);
    dataFile.print(',');
    // from example MKRGPS GPSlocation
    dataFile.print(GPS.latitude(), 6);
    dataFile.print(',');
    dataFile.print(GPS.altitude());
    dataFile.print(',');
    dataFile.print(GPS.speed()); 
    dataFile.print(',');
    dataFile.print(GPS.course());
    dataFile.print(',');
    dataFile.println(GPS.getTime());
    dataFile.close();
  }
  
  captureImage();
}
