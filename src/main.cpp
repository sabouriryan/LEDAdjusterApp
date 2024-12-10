#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"

char ssid[50]; // Your network SSID (name)
char pass[50]; 

const int pwmPin = 32; // GPIO pin connected to the LED
//int brightness = 0;    // Brightness level (0-255)
WebServer server(80);

void nvs_access() {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open
    Serial.printf("\n");
    Serial.printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        Serial.printf("Done\n");
        Serial.printf("Retrieving SSID/PASSWD\n");

        size_t ssid_len = sizeof(ssid);
        size_t pass_len;
        err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
        err |= nvs_get_str(my_handle, "pass", pass, &pass_len);

        switch (err) {
            case ESP_OK:
                Serial.printf("Done\n");
                // Uncomment the lines below to print SSID and password for debugging
                Serial.printf("SSID = %s\n", ssid);
                Serial.printf("PASSWD = %s\n", pass);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                Serial.printf("The value is not initialized yet!\n");
                break;
            default:
                Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
    }
    // Close
    nvs_close(my_handle);
}

void setup() {
  Serial.begin(9600);
  nvs_access();
  Serial.printf("SSID = %s, PASS = %s\n", ssid, pass);
  //pinMode(pwmPin, OUTPUT);
  ledcSetup(0, 5000, 8);  // Channel 0, 5 kHz, 8-bit resolution
  ledcAttachPin(pwmPin, 0);
  ledcWrite(0, 10);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    Serial.printf("SSID = %s, PASS = %s\n", ssid, pass);
  }
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());


  // Define HTTP endpoint to set brightness
  server.on("/set_brightness", []() {

    // set CORS policy
    server.sendHeader("Access-Control-Allow-Origin", "*"); // Allow all origins
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

    int brightness = 0;
    if (server.hasArg("value")) {
      brightness = server.arg("value").toInt();
      brightness = constrain(brightness, 0, 255); // Ensure value is within 0-255
      ledcWrite(0, brightness);
      server.send(200, "text/plain", "Brightness updated to " + String(brightness));
    } else {
      server.send(400, "text/plain", "Missing 'value' parameter");
    }
  });

  server.begin();
}

void loop() {
  server.handleClient();
}

