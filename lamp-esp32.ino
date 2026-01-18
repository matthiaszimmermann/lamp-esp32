#include "arduino.h"
#include <Preferences.h>

//RemoteXY Setup
#define REMOTEXY__DEBUGLOG    

// RemoteXY select connection mode and include library 
#define REMOTEXY_MODE__ESP32CORE_BLE

#include <BLEDevice.h>

// RemoteXY connection settings 
#define REMOTEXY_BLUETOOTH_NAME "RemoteXY"

#include "RemoteXY.h"

// ============ PREFERENCES (persistent storage) ============
Preferences preferences;
uint8_t prev_R = 0;  // Track previous values to detect actual changes
uint8_t prev_G = 0;
uint8_t prev_B = 0;
unsigned long lastChangeTime = 0;
const unsigned long SAVE_DELAY_MS = 2000;  // Save 2 seconds after last change
bool pendingSave = false;

//RemoteXY GUI configuration  
#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   // 37 bytes V19 
  { 255,3,0,0,0,30,0,19,0,0,0,80,97,112,97,32,76,97,109,112,
  101,0,24,1,106,200,1,1,1,0,6,18,63,72,72,0,24 };

// this structure defines all the variables and events of your control interface  
struct {
  // input variables
  uint8_t rgb_R; // =0..255 Red color value, from 0 to 255
  uint8_t rgb_G; // =0..255 Green color value, from 0 to 255
  uint8_t rgb_B; // =0..255 Blue color value, from 0 to 255

  // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0

} RemoteXY;   
#pragma pack(pop)

// ============ GPIO Settings ============
const int pwmPins[4] = {18, 39, 40, 21};     // R G B W

// ============ PWM SETTINGS =============
const int pwmFreq = 10000;                // 5 kHz PWM
const int pwmResolution = 8;            // 8-bit (0–255)
char rgbw_values[4];

void setup() {
  RemoteXY_Init ();  // initialization by macros

  // Set up each channel + pin
  for (int i = 0; i < 4; i++) {
    ledcAttach(pwmPins[i], pwmFreq, pwmResolution);
  }

  // Load saved RGB values from flash
  preferences.begin("lamp", false);  // namespace "lamp", read-write mode
  RemoteXY.rgb_R = preferences.getUChar("R", 0);  // default to mid brightness
  RemoteXY.rgb_G = preferences.getUChar("G", 128);
  RemoteXY.rgb_B = preferences.getUChar("B", 0);
  prev_R = RemoteXY.rgb_R;
  prev_G = RemoteXY.rgb_G;
  prev_B = RemoteXY.rgb_B;
  preferences.end();
}

void loop() { 
  //RemoteXY
  RemoteXYEngine.handler ();   
  // TODO you loop code
  // use the RemoteXY structure for data transfer
  // do not call delay(), use instead RemoteXYEngine.delay() 
  
  // Convert RGB to RGBW - extract white component for efficiency
  uint8_t r = RemoteXY.rgb_R;
  uint8_t g = RemoteXY.rgb_G;
  uint8_t b = RemoteXY.rgb_B;
  
  // Find the minimum of R, G, B - this is the white component
  uint8_t w = min(r, min(g, b));
  
  // Subtract white from RGB channels
  rgbw_values[0] = (r - w) / 2;  // R
  rgbw_values[1] = (g - w) / 2;  // G
  rgbw_values[2] = (b - w) / 2;  // B
  rgbw_values[3] = w / 2;        // W (white channel)

  // Check if RGB values changed from previous iteration (not just different from saved)
  if (RemoteXY.rgb_R != prev_R || 
      RemoteXY.rgb_G != prev_G || 
      RemoteXY.rgb_B != prev_B) {
    lastChangeTime = millis();
    pendingSave = true;
    prev_R = RemoteXY.rgb_R;
    prev_G = RemoteXY.rgb_G;
    prev_B = RemoteXY.rgb_B;
  }

  // Save to flash after delay (to avoid excessive writes while user is adjusting)
  if (pendingSave && (millis() - lastChangeTime >= SAVE_DELAY_MS)) {
    preferences.begin("lamp", false);
    preferences.putUChar("R", RemoteXY.rgb_R);
    preferences.putUChar("G", RemoteXY.rgb_G);
    preferences.putUChar("B", RemoteXY.rgb_B);
    preferences.end();
    pendingSave = false;
  }

  for (int i = 0; i < 4; i++) {
    ledcWrite(pwmPins[i], rgbw_values[i]);
  }
  RemoteXYEngine.delay(1);
}