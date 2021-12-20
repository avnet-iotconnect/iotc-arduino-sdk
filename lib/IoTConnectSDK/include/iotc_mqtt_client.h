/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTC_MQTT_CLIENT_H
#define IOTC_MQTT_CLIENT_H

#include "Arduino.h"
#include "Client.h"
#include "iotconnect_discovery.h"
#include "IoTConnectSDK.h"


typedef void (*IotConnectC2dCallback)(unsigned char* message, size_t message_len);

typedef struct {
    IotclSyncResponse* sr;
    IotConnectAuthInfo *auth; // Pointer to IoTConnect auth configuration
    IotConnectC2dCallback c2d_msg_cb; // callback for inbound messages
    IotConnectStatusCallback status_cb; // callback for connection status
    Client* net;
    size_t mqtt_buffer_size; // Buffer size for PubSubClient MQTT implementation. 2048, if set to 0 (default)
} IotConnectMqttClientConfig;

int iotc_mqtt_client_init(IotConnectMqttClientConfig *c);

int iotc_mqtt_client_disconnect();

bool iotc_mqtt_client_is_connected();

void iotc_mqtt_client_loop();

int iotc_mqtt_client_send_message(const char *message);

#endif // IOTC_MQTT_CLIENT_H