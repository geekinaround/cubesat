#if 0
This file is created by Team Norway mentors. It includes functionality not directly involved in the solution,
and covers functionality too complex to reasonably expect the four 14-year old members to create (none of them
had coded in C before participating in Global Challenge)
#endif

#include "helper.h"

#include <ArduCAM.h>
#include <SD.h>
#include <SPI.h>

#include <Arduino_MKRENV.h>
#include <Arduino_MKRGPS.h>


ArduCAM camera(OV5642, CAMERA_CS_PIN);

bool dumpImageToSD();

bool initializeEnvShield()
{
  Serial.print("Initializing ENV shield...");
  if (!ENV.begin()) {
    Serial.println("Failed to initialize ENV shield");
    return false;
  }
  Serial.println("ENV shield initialized.");

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card failed, or not present");
    return false;
  }
  Serial.println("SD card initialized.");
  return true;
}

bool initializeGPSShield()
{
  Serial.print("Initializing GPS...");
  if (!GPS.begin(GPS_MODE_I2C)) {
    Serial.println("Failed to initialize GPS");
    return false;
  }
  Serial.println("GPS initialized.");

  bool led_state = LOW;
  unsigned long next_time = millis();
  while (GPS.satellites()==0)
  {
    led_state = !led_state;
    digitalWrite(LED_BUILTIN, led_state);
    Serial.println("Waiting for GPS satellite lock.");
    next_time += 1000;
    delayUntil(next_time);
  }
  Serial.print("Got lock at ");
  Serial.print(GPS.latitude(), 7);
  Serial.print(", ");
  Serial.println(GPS.longitude(), 7);
  return true;
}

bool initializeCamera()
{
  Serial.println("ArduCAM Start!");

  pinMode(CAMERA_CS_PIN, OUTPUT);
  digitalWrite(CAMERA_CS_PIN, HIGH);
  
  //Reset the CPLD
  camera.write_reg(0x07, 0x80);
  delay(100);
  camera.write_reg(0x07, 0x00);
  delay(100); 

  uint8_t temp;
  do
  {
    //Check if the ArduCAM SPI bus is OK
    camera.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = camera.read_reg(ARDUCHIP_TEST1);
    if(temp == 0x55)
    {
      Serial.println("SPI interface OK.");
    }
    else
    {
      Serial.println("SPI interface Error!");
      delay(1000);
    }
  } while (temp != 0x55);
  
  uint8_t vid, pid;
  do
  {
    //Check if the camera module type is OV5642
    camera.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    camera.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    if (vid==0x56 && pid==0x42)
    {
      Serial.println("OV5642 detected.");
    }
    else
    {
      Serial.println("Can't find OV5642 module!");
      delay(1000);
    }
  } while (vid!=0x56 || pid!=0x42);

  //Change to JPEG capture mode and initialize the module
  camera.set_format(JPEG);
  camera.InitCAM();
  camera.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  camera.clear_fifo_flag();
  camera.write_reg(ARDUCHIP_FRAMES, 0x00);

  camera.OV5642_set_JPEG_size(OV5642_640x480);
  camera.OV5642_set_Light_Mode(Auto); //Auto,Sunny,Cloudy,Office,Home
  camera.OV5642_set_Color_Saturation(Saturation2); //Saturation4,Saturation3,Saturation2,Saturation1,Saturation0,Saturation_1,Saturation_2,Saturation_3,Saturation_4
  camera.OV5642_set_Brightness(Brightness1); //Brightness4,Brightness3,Brightness2,Brightness1,Brightness0,Brightness_1,Brightness_2,Brightness_3,Brightness_4
  camera.OV5642_set_Contrast(Contrast0); //Contrast4,Contrast3,Contrast2,Contrast1,Contrast0,Contrast_1,Contrast_2,Contrast_3,Contrast_4
  camera.OV5642_set_Special_effects(Normal); //Antique,Bluish,Greenish,Reddish,BW,Negative,BWnegative,Normal,Sepia,Overexposure,Solarize,Blueish,Yellowish
  camera.OV5642_set_hue(degree_0); //degree_180,degree_150,degree_120,degree_90,degree_60,degree_30,degree_0,degree30,degree60,degree90,degree120,degree150
  camera.OV5642_set_Exposure_level(EV_2); //EV3,EV2,EV1,EV0,EV_1,EV_2,EV_3
  camera.OV5642_set_Sharpness(Auto_Sharpness_default); //Auto_Sharpness_default,Auto_Sharpness1,Auto_Sharpness2,Manual_Sharpnessoff,Manual_Sharpness1,Manual_Sharpness2,Manual_Sharpness3,Manual_Sharpness4,Manual_Sharpness5
  camera.OV5642_set_Compress_quality(default_quality); //high_quality,default_quality,low_quality

  return true;
}

void captureImage()
{
  camera.flush_fifo();
  camera.clear_fifo_flag();
  camera.start_capture();
  while (!camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)); 
  dumpImageToSD();
  camera.clear_fifo_flag();
}

bool dumpImageToSD()
{
  uint32_t length = camera.read_fifo_length();
  if (length >= MAX_FIFO_SIZE) //8M
  {
    Serial.println("Over size.");
    return false;
  }
  if (length == 0)
  {
    Serial.println("Size is 0.");
    return false;
  } 

  static int file_index = 0;
  File outFile;
  bool is_header = false;
  byte buf[256]; 

  camera.CS_LOW();
  camera.set_fifo_burst();
  int index = 0;
  uint8_t temp=0, temp_last=0;
  while (length--)
  {
    temp_last = temp;
    temp = SPI.transfer(0x00);
    //Read JPEG data from FIFO
    if (temp==0xD9 && temp_last==0xFF) //Found the end?
    {
      buf[index++] = 0xD9; //save the last 0xD9
      //Write the remaining bytes in the buffer
      camera.CS_HIGH();
      outFile.write(buf, index);
      outFile.close();
      is_header = false;
      camera.CS_LOW();
      camera.set_fifo_burst();
      index = 0;
    }  

    if (is_header == true)
    { 
      //Write image data to buffer if not full
      if (index < 256)
      {
        buf[index++] = temp;
      }
      else
      {
        //Write 256 bytes image data to file
        camera.CS_HIGH();
        outFile.write(buf, 256);
        index = 0;
        buf[index++] = temp;
        camera.CS_LOW();
        camera.set_fifo_burst();
      }        
    }
    else if (temp==0xD8 && temp_last==0xFF)
    {
      is_header = true;
      camera.CS_HIGH();
      //Create a jpg file
      char filename[10];
      itoa(++file_index, filename, 10);
      strcat(filename, ".jpg");
      //Open the new file
      outFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);
      if (!outFile)
      {
        Serial.println("File open failed");
        return false;
      }
      camera.CS_LOW();
      camera.set_fifo_burst();   
      buf[index++] = temp_last;
      buf[index++] = temp;   
    }
  }
  camera.CS_HIGH();
  return true;
}

void delayUntil(unsigned long timestamp)
{
  while(millis() < timestamp)
  {
    GPS.available();
  }
}
