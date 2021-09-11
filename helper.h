#if 0
This file is created by Team Norway mentors. It includes functionality not directly involved in the solution,
and covers functionality too complex to reasonably expect the four 14-year old members to create (none of them
had coded in C before participating in Global Challenge)
#endif

const int CAMERA_CS_PIN  = 3;
const int SD_CS_PIN      = 4;
const int GPS_EXTINT_PIN = 5;


bool initializeEnvShield();
bool initializeGPSShield();
bool initializeCamera();

void captureImage();

void delayUntil(unsigned long timestamp);
