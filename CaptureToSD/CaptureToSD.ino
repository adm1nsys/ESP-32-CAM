#include "esp_camera.h"
#include "SD_MMC.h"
#include "FS.h"

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_PIN 4  // Pin for the flash
#define BUTTON_PIN 0 // Pin for the button (BOOT button)

bool initializeCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Init with low specs to reduce load
  config.frame_size = FRAMESIZE_QVGA; // 320x240
  config.jpeg_quality = 12; // Lower quality for smaller size
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  return true;
}

void resetCamera() {
  esp_camera_deinit();
  delay(100);
  initializeCamera();
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize the camera
  if (!initializeCamera()) {
    return;
  } else {
    Serial.println("Camera init succeeded");
  }

  // Initialize the SD card
  if(!SD_MMC.begin("/sdcard", true)){
    Serial.println("Card Mount Failed");
    return;
  } else {
    Serial.println("SD Card Mount succeeded");
  }

  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  } else {
    Serial.println("SD Card attached");
  }

  // Ensure the video directory exists
  if (!SD_MMC.exists("/video")) {
    if (SD_MMC.mkdir("/video")) {
      Serial.println("Directory /video created successfully");
    } else {
      Serial.println("Failed to create directory /video");
      return;
    }
  } else {
    Serial.println("Directory /video already exists");
  }

  // Initialize flash and button
  pinMode(FLASH_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // Wait for button press
  if (digitalRead(BUTTON_PIN) == LOW) {
    // Flash the LED once to indicate start of video capture
    digitalWrite(FLASH_PIN, HIGH);
    delay(100);
    digitalWrite(FLASH_PIN, LOW);

    // Take a video sequence
    for (int i = 0; i < 100; i++) { // Capture 100 frames
      if (!captureAndSaveFrame(i)) {
        resetCamera(); // Reset camera if capture failed
        delay(2000); // Delay to stabilize
      } else {
        delay(1000); // Increase delay for frame rate
      }
    }
  }

  delay(100); // Debounce delay
}

bool captureAndSaveFrame(int frameNumber) {
  camera_fb_t * fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();  

  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  // Path where new picture will be saved in SD Card
  String path = "/video/frame" + String(frameNumber) + ".jpg";
  Serial.printf("Saving file to path: %s\n", path.c_str());
  
  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.printf("Failed to open file in writing mode. Path: %s\n", path.c_str());
    esp_camera_fb_return(fb);
    return false;
  } else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
  }
  file.close();
  esp_camera_fb_return(fb); // Return the frame buffer to free the memory
  return true;
}
