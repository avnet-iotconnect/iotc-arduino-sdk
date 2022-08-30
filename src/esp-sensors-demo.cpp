/*
  Basic ESP32 MQTT example
  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP32 board/library.
  It connects to an MQTT server then:
  - publishes "connected to MQTT" to the topic "outTopic"
  - subscribes to the topic "inTopic", printing out messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the "ON" payload is send to the topic "inTopic" , LED will be turned on, and acknowledgement will be send to Topic "outTopic"
  - If the "OFF" payload is send to the topic "inTopic" , LED will be turned OFF, and acknowledgement will be send to Topic "outTopic"
  It will reconnect to the server if the connection is lost using a
  reconnect function.
*/

#include <stdio.h>
#include <Arduino.h>
#include <ESP.h>
#include <WiFiClientSecure.h>

#include <IoTConnectSDK.h>
#include "wifi_config.h"
#include "app_config.h"

WiFiClientSecure net;

#define BUTTON 0
#define LED 5
#define APP_VERSION "01.00.00"

static void fatal_error(const char* msg) {
  printf(msg);
  delay(2000);
  ESP.restart();
  while(true) { // never reaches on ESP
    delay(100);
  }
}

static bool button_state_changed() {
  static int last_state = digitalRead(BUTTON);
  int now_state = digitalRead(BUTTON);
  int ret = now_state != last_state;
  last_state = now_state;
  return ret;
}

static void on_connection_status(IotConnectConnectionStatus status) {
    // Add your own status handling
    switch (status) {
        case IOTC_CS_MQTT_CONNECTED:
            printf("IoTConnect Client Connected\n");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            printf("IoTConnect Client Disconnected\n");
            break;
        default:
            printf("IoTConnect Client ERROR\n");
            break;
    }
}

static void command_status(IotclEventData data, bool status, const char *command_name, const char *message) {
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, status, message);
    printf("command: %s status=%s: %s\n", command_name, status ? "OK" : "Failed", message);
    printf("Sent CMD ack: %s\n", ack);
    iotconnect_sdk_send_packet(ack);
    free((void *) ack);
}

static void on_command(IotclEventData data) {
    char *command = iotcl_clone_command(data);
    if (NULL != command) {
      if (0 == strcmp(command, "led-on")) {        
        digitalWrite(LED, HIGH);
        command_status(data, true, command, "Success");
      } else if (0 == strcmp(command, "led-off")) {
        digitalWrite(LED, LOW);
        command_status(data, true, command, "Success");
      } else  {
        command_status(data, false, command, "Not implemented");
      }
      free((void *) command);
    } else {
        command_status(data, false, "?", "Internal error");
    }
}

static bool is_app_version_same_as_ota(const char *version) {
    return strcmp(APP_VERSION, version) == 0;
}

static bool app_needs_ota_update(const char *version) {
    return strcmp(APP_VERSION, version) < 0;
}

static void on_ota(IotclEventData data) {
    const char *message = NULL;
    char *url = iotcl_clone_download_url(data, 0);
    bool success = false;
    if (NULL != url) {
        printf("Download URL is: %s\n", url);
        const char *version = iotcl_clone_sw_version(data);
        if (is_app_version_same_as_ota(version)) {
            printf("OTA request for same version %s. Sending success\n", version);
            success = true;
            message = "Version is matching";
        } else if (app_needs_ota_update(version)) {
            printf("OTA update is required for version %s.\n", version);
            success = false;
            message = "Not implemented";
        } else {
            printf("Device firmware version %s is newer than OTA version %s. Sending failure\n", APP_VERSION,
                   version);
            // Not sure what to do here. The app version is better than OTA version.
            // Probably a development version, so return failure?
            // The user should decide here.
            success = false;
            message = "Device firmware version is newer";
        }

        free((void *) url);
        free((void *) version);
    } else {
        // compatibility with older events
        // This app does not support FOTA with older back ends, but the user can add the functionality
        const char *command = iotcl_clone_command(data);
        if (NULL != command) {
            // URL will be inside the command
            printf("Command is: %s\n", command);
            message = "Old back end URLS are not supported by the app";
            free((void *) command);
        }
    }
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, success, message);
    if (NULL != ack) {
        printf("Sent OTA ack: %s\n", ack);
        iotconnect_sdk_send_packet(ack);
        free((void *) ack);
    }
}

static void publish_telemetry() {
    IotclMessageHandle msg = iotcl_telemetry_create();

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    iotcl_telemetry_add_with_iso_time(msg, iotcl_iso_timestamp_now());
    iotcl_telemetry_set_string(msg, "version", APP_VERSION);
    iotcl_telemetry_set_number(msg, "cpu", 3.123); // test floating point numbers
    iotcl_telemetry_set_number(msg, "button", digitalRead(BUTTON));

    const char *str = iotcl_create_serialized_string(msg, false);
    iotcl_telemetry_destroy(msg);
    printf("Sending: %s\n", str);
    iotconnect_sdk_send_packet(str); // underlying code will report an error
    iotcl_destroy_serialized(str);
}
void demo_setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  wl_status_t wifi_status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  if (WL_NO_SHIELD == wifi_status) {
    fatal_error("ERROR: The sample code requires WiFi functionality!\n");
    return; // never reaches on ESP
  }

  if (wifi_status != WL_CONNECTED) {
      printf("WiFi Connect Status is %d. Waiting for connection..", wifi_status);
  }
  for(int connect_tries = WIFI_CONNECT_TIMEOUT; connect_tries > 0 && !WiFi.isConnected(); connect_tries--) {
    printf(".");
    delay(1000);
  }
  
  if (!WiFi.isConnected()) {
    printf("Failed to connect to WiFi after %d tries\n", WIFI_CONNECT_TIMEOUT);
    delay(2000);
    ESP.restart();
    while(true) { // never reaches on ESP
      delay(100);
    }
  } else {
      printf("\nWiFi Connection Established\n");
  }

  printf("\n");
  IotConnectClientConfig *config = iotconnect_sdk_init_and_get_config();

  config->cpid = (char *) IOTCONNECT_CPID;
  config->env = (char *) IOTCONNECT_ENV;
  config->duid = (char *) IOTCONNECT_DUID;
  config->auth_info.type = IOTCONNECT_AUTH_TYPE;
  config->net = &net;
  config->ota_cb = on_ota;
  config->status_cb = on_connection_status;
  config->cmd_cb = on_command;

  if (config->auth_info.type == IOTC_AT_X509) {
    config->auth_info.data.cert_info.device_cert = (char*) IOTCONNECT_DEVICE_CERT;
    config->auth_info.data.cert_info.device_key = (char*) IOTCONNECT_DEVICE_KEY;
  }

  for (int ii = 0; ii < 20; ii++)  {
    if (!WiFi.isConnected()) {
      WiFi.reconnect();
      for(int connect_tries = 10; connect_tries > 0 && !WiFi.isConnected(); connect_tries--) {
        printf(".");
        delay(1000);
      }
      if (!WiFi.isConnected()) {
        fatal_error("ERROR: WiFi disconnected. Restarting...\n");
      }
    }
    int ret = iotconnect_sdk_init();
    if (ret == 0) {
      digitalWrite(LED, HIGH); // set the LED on while we are connected. Override with commands
      for (int i = 0; i < 20; i++) {
        if (!iotconnect_sdk_is_connected()) {
          // not connected, but mqtt should try to reconnect
          break;
        } else {
          publish_telemetry();
        }
        // loop for 5 seconds, checking the button state for changes every 100 ms.
        for (int j = 0; j < 50; j++) {
          iotconnect_sdk_loop();
          if (button_state_changed()) {
            publish_telemetry(); // publish as soon as we detect that button state changed
          }
          delay(100);
        }
      }
    } else {
      Serial.println("Encountered an error while initializing the SDK!\n");
      while(true) {
        delay(1000);
      }
    }
    iotconnect_sdk_disconnect();
    digitalWrite(LED, LOW); // set the LED off while we are disconnected
    delay(1000);
  }
  printf("Done.\n");
  while(true) {
    delay(1000);
  }
}

void demo_loop() {
  // In order to simplify this demo we are doing everything in setup() and controlling our own loop
  delay(1000);
}
