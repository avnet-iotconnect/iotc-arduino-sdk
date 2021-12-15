/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */
   
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include "iotc_mqtt_client.h"

#define MQTT_SECURE_PORT 8883
#define DEFAULT_BUFFER_SIZE 2048

static PubSubClient* client = NULL;
static char *publish_topic = NULL;
static IotConnectC2dCallback c2d_msg_cb = NULL; // callback for inbound messages
static IotConnectStatusCallback status_cb = NULL; // callback for connection status

static void mqtt_deinit() {
    if (client) {
        client->disconnect();
        delete client;
    }
    if (publish_topic) {
        free(publish_topic);
    }
    publish_topic = NULL;
    client = NULL;    
    c2d_msg_cb = NULL;
    status_cb = NULL;
}

static void mqtt_message_callback(char* topic, byte* payload, unsigned int length) {
    if (c2d_msg_cb) {
        c2d_msg_cb(payload, length);
    }    
}

int iotc_mqtt_client_disconnect() {
    if (client) {
        client->disconnect();
    }
    mqtt_deinit();
    return 0;
}

bool iotc_mqtt_client_is_connected() {
    if (!client) {
        return false;
    }
    return client->state() == MQTT_CONNECTED;
}

int iotc_mqtt_client_send_message(const char *message) {
    if (!client || !publish_topic) {
        printf("iotc_mqtt_client_send_message(): Client not initialized!\n");
        return -2;
    }
    return client->publish(publish_topic, message) ? 0 : -1;
}

void iotc_mqtt_client_loop() {
    if (!client || !publish_topic) {
        printf("iotc_mqtt_client_loop(): Client not initialized!\n");
        return;
    }
    if (!client->connected()) {
        if (status_cb) {
            status_cb(IOTC_CS_MQTT_DISCONNECTED);
        }
    }
    client->loop();
}


int iotc_mqtt_client_init(IotConnectMqttClientConfig *c) {
    if (!c->net) {
        printf("ERROR: Must supply a secure client in config!\n");
        return -1;
    }
    mqtt_deinit(); // reset all locals

    publish_topic = strdup(c->sr->broker.pub_topic);
    if (!publish_topic) {
        printf("ERROR: Unable to allocate memory for pub topic copy!\n");
        return -1;
    }

    client = new PubSubClient(
        c->sr->broker.host,
        MQTT_SECURE_PORT,
        *(c->net)
    );
    if (NULL == client) {
        printf("ERROR: Unable to allocate memory the MQTT client!\n");
        mqtt_deinit();
        return -1;
    }

    if (0 == c->mqtt_buffer_size) {
        c->mqtt_buffer_size = DEFAULT_BUFFER_SIZE;
    }
    client->setBufferSize(c->mqtt_buffer_size);

    int connect_tries = 10;
    do   {
        if (client->connect(
            c->sr->broker.client_id,
            c->sr->broker.user_name,
            c->sr->broker.pass
        )) {
            break;
        }
        printf("Failed to connect to MQTT server. Retries left %d...\n", connect_tries);
        delay(1000);
        connect_tries--;
    }  while (connect_tries > 0);
    if (connect_tries <= 0) {
        printf("ERROR: Unable to connect the MQTT client!");
        mqtt_deinit();
        return -2;
    }

    client->setCallback(mqtt_message_callback);
    if (!client->subscribe(c->sr->broker.sub_topic)) {
        mqtt_deinit();
        printf("ERROR: Unable to subscribe for C2D messages!");
        return -2;
    }
    
    c2d_msg_cb = c->c2d_msg_cb;
    status_cb = c->status_cb;

    if (status_cb) {
        status_cb(IOTC_CS_MQTT_CONNECTED);
    }

    return 0;
}
