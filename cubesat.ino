#include <SPI.h>
#include <Wire.h>

#include "helper.h"


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
  Serial.begin(115200);
  Wire.begin();
  SPI.begin();
  pinMode(LED_BUILTIN, OUTPUT);

  if (initializeEnvShield()==true &&
      initializeCamera()==true &&
      initializeGPSShield()==true)
  {
    Serial.println("Everything is initialized OK.");
    blinkLED(50, 200);
  }
  else
  {
    Serial.println("Something failed.");
  }
}


void loop()
{
//  captureImage();
//  delay(1000);
}
