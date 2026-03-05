#include <Arduino.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bluetooth_service.h"

#include "muse.h"
#include "lovense.h"

static const char* TAG = "main";

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }

  Serial.println("program started");
  Serial.println();
  Serial.println();
  Serial.println(F("RS485 RTU SDM120***"));

  // Set runtime log level to VERBOSE for our tags
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  // Silence noisy NimBLE logs
  esp_log_level_set("NimBLEAdvertising", ESP_LOG_NONE);

  bluetooth_service_init();

  lovense_init();
  muse_init();
  muse_start();

  bluetooth_service_start();

  ESP_LOGI(TAG, "SETUP DONE.");
}

void loop() {
  // In this example, the main loop is not used since the lovense sensor task runs independently.
  // You can add other tasks or functionality here if needed.
}
