//
// Copyright: Avnet, Softweb Inc. 2021
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Arduino.h"
#include "Client.h"
#include "iotconnect_lib.h"
#include "iotconnect_telemetry.h"
#include "iotconnect_discovery.h"
#include "iotc_http_request.h"
#include "iotc_mqtt_client.h"
#include "iotconnect_certs.h"
#include "iotconnect.h"

#define HTTP_DISCOVERY_URL_FORMAT "https://%s/api/sdk/cpid/%s/lang/M_C/ver/2.0/env/%s"
#define HTTP_SYNC_URL_FORMAT "https://%s%ssync?"

static IotclConfig lib_config = {0};
static IotConnectClientConfig config = {0};

// cached discovery/sync response:
static IotclDiscoveryResponse *discovery_response = NULL;
static IotclSyncResponse *sync_response = NULL;

static void dump_response(const char *message, IotConnectHttpResponse *response) {
    printf("%s", message);
    if (response->data) {
        printf(" Response was:\n----\n%s\n----\n", response->data);
    } else {
        printf(" Response was empty\n");
    }
}

static void report_sync_error(IotclSyncResponse *response, const char *sync_response_str) {
    if (NULL == response) {
        printf("Failed to obtain sync response?\n");
        return;
    }
    switch (response->ds) {
        case IOTCL_SR_DEVICE_NOT_REGISTERED:
            printf("IOTC_SyncResponse error: Not registered\n");
            break;
        case IOTCL_SR_AUTO_REGISTER:
            printf("IOTC_SyncResponse error: Auto Register\n");
            break;
        case IOTCL_SR_DEVICE_NOT_FOUND:
            printf("IOTC_SyncResponse error: Device not found\n");
            break;
        case IOTCL_SR_DEVICE_INACTIVE:
            printf("IOTC_SyncResponse error: Device inactive\n");
            break;
        case IOTCL_SR_DEVICE_MOVED:
            printf("IOTC_SyncResponse error: Device moved\n");
            break;
        case IOTCL_SR_CPID_NOT_FOUND:
            printf("IOTC_SyncResponse error: CPID not found\n");
            break;
        case IOTCL_SR_UNKNOWN_DEVICE_STATUS:
            printf("IOTC_SyncResponse error: Unknown device status error from server\n");
            break;
        case IOTCL_SR_ALLOCATION_ERROR:
            printf("IOTC_SyncResponse internal error: Allocation Error\n");
            break;
        case IOTCL_SR_PARSING_ERROR:
            printf("IOTC_SyncResponse internal error: Parsing error. Please check parameters passed to the request.\n");
            break;
        default:
            printf("WARN: report_sync_error called, but no error returned?\n");
            break;
    }
    printf("Raw server response was:\n--------------\n%s\n--------------\n", sync_response_str);
}

static IotclDiscoveryResponse *run_http_discovery(Client *net, const char *cpid, const char *env) {
    char *json_start = NULL;
    IotclDiscoveryResponse *ret = NULL;
    char *url_buff = (char *) malloc(sizeof(HTTP_DISCOVERY_URL_FORMAT) +
                            sizeof(IOTCONNECT_DISCOVERY_HOSTNAME) +
                            strlen(cpid) +
                            strlen(env) - 4 /* %s x 2 */
    );

    sprintf(url_buff, HTTP_DISCOVERY_URL_FORMAT,
            IOTCONNECT_DISCOVERY_HOSTNAME, cpid, env
    );

    IotConnectHttpResponse response;
    iotconnect_https_request(
        net,
        &response,
        url_buff,
        NULL
    );

    if (NULL == response.data) {
        dump_response("Unable to parse HTTP response,", &response);
        goto cleanup;
    }
    json_start = strstr(response.data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.", &response);
        goto cleanup;
    }
    if (json_start != response.data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.", &response);
    }

    ret = iotcl_discovery_parse_discovery_response(json_start);
    if (!ret) {
        printf("Error: Unable to get discovery response for environment \"%s\". Please check the environment name in the key vault.\n", env);
    }

    // fall through
    cleanup:
    iotconnect_free_https_response(&response);
    return ret;
}

static IotclSyncResponse *run_http_sync(Client *net, const char *cpid, const char *uniqueid) {
    char *json_start = NULL;
    IotclSyncResponse *ret = NULL;
    char *url_buff = (char *)malloc(sizeof(HTTP_SYNC_URL_FORMAT) +
                            strlen(discovery_response->host) +
                            strlen(discovery_response->path)
    );
    char *post_data = (char *)malloc(IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN + 1);

    if (!url_buff || !post_data) {
        printf("run_http_sync: Out of memory!");
        free(url_buff); // one of them could have succeeded
        free(post_data);
        return NULL;
    }

    sprintf(url_buff, HTTP_SYNC_URL_FORMAT,
            discovery_response->host,
            discovery_response->path
    );
    snprintf(post_data,
             IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN, /*total length should not exceed MTU size*/
             IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_TEMPLATE,
             cpid,
             uniqueid
    );

    IotConnectHttpResponse response;
    iotconnect_https_request(
        net,
        &response,
        url_buff,
        post_data
    );

    free(url_buff);
    free(post_data);

    if (NULL == response.data) {
        dump_response("Unable to parse HTTP response.", &response);
        goto cleanup;
    }
    json_start = strstr(response.data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.", &response);
        goto cleanup;
    }
    if (json_start != response.data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.", &response);
    }

    ret = iotcl_discovery_parse_sync_response(json_start);
    if (!ret || ret->ds != IOTCL_SR_OK) {
        if (config.auth_info.type == IOTC_AT_TPM && ret && ret->ds == IOTCL_SR_DEVICE_NOT_REGISTERED) {
            // malloc below will be freed when we iotcl_discovery_free_sync_response
            ret->broker.client_id = (char *) malloc(strlen(uniqueid) + 1 /* - */ + strlen(cpid) + 1);
            sprintf(ret->broker.client_id, "%s-%s", cpid, uniqueid);
            printf("TPM Device is not yet enrolled. Enrolling...\n");
        } else {
            report_sync_error(ret, response.data);
            iotcl_discovery_free_sync_response(ret);
            ret = NULL;
        }
    }

    cleanup:
    iotconnect_free_https_response(&response);
    // fall through

    return ret;
}

static void on_mqtt_c2d_message(unsigned char *message, size_t message_len) {
    char *str = (char *)malloc(message_len + 1);
    memcpy(str, message, message_len);
    str[message_len] = 0;
    printf("event>>> %s\n", str);
    if (!iotcl_process_event(str)) {
        printf("Error encountered while processing %s\n", str);
    }
    free(str);
}

void iotconnect_sdk_disconnect() {
    printf("Disconnecting...\n");
    if (0 == iotc_mqtt_client_disconnect()) {
        printf("Disconnected.\n");
    }
}

bool iotconnect_sdk_is_connected() {
    return iotc_mqtt_client_is_connected();
}

IotConnectClientConfig *iotconnect_sdk_init_and_get_config() {
    memset(&config, 0, sizeof(config));
    return &config;
}

IotclConfig *iotconnect_sdk_get_lib_config() {
    return iotcl_get_config();
}

static void on_message_intercept(IotclEventData data, IotConnectEventType type) {
    switch (type) {
        case ON_FORCE_SYNC:
            iotconnect_sdk_disconnect();
            iotcl_discovery_free_discovery_response(discovery_response);
            iotcl_discovery_free_sync_response(sync_response);
            sync_response = NULL;
            discovery_response = run_http_discovery(config.net, config.cpid, config.env);
            if (NULL == discovery_response) {
                printf("Unable to run HTTP discovery on ON_FORCE_SYNC\n");
                return;
            }
            sync_response = run_http_sync(config.net, config.cpid, config.duid);
            if (NULL == sync_response) {
                printf("Unable to run HTTP sync on ON_FORCE_SYNC\n");
                return;
            }
            printf("Got ON_FORCE_SYNC. Disconnecting.\n");
            iotconnect_sdk_disconnect(); // client will get notification that we disconnected and will reinit

        case ON_CLOSE:
            printf("Got a disconnect request. Closing the mqtt connection. Device restart is required.\n");
            iotconnect_sdk_disconnect();
        default:
            break; // not handling nay other messages
    }

    if (NULL != config.msg_cb) {
        config.msg_cb(data, type);
    }
}

int iotconnect_sdk_send_packet(const char *data) {
    return iotc_mqtt_client_send_message(data);
}

void iotconnect_sdk_loop() {
    return iotc_mqtt_client_loop();
}


///////////////////////////////////////////////////////////////////////////////////
// this the Initialization os IoTConnect SDK
int iotconnect_sdk_init() {
    int ret;

    if (!config.net)  {
        printf("A network interface (client) must be configured.\n");
        return -1;
    }
    if (config.auth_info.type == IOTC_AT_TPM)  {
        printf("TPM authentication type is not supported by the SDK.\n");
        return -1;
    }
    if (config.auth_info.type == IOTC_AT_SYMMETRIC_KEY) {
        printf("Symmetric Key authentication type is not supported by the SDK.\n");
        return -1;
    }

    if (config.auth_info.type == IOTC_AT_X509) {
        config.net->setCertificate(config.auth_info.data.cert_info.device_cert);
        config.net->setPrivateKey(config.auth_info.data.cert_info.device_key);
    }

    if (!discovery_response) {
        discovery_response = run_http_discovery(config.net, config.cpid, config.env);
        if (NULL == discovery_response) {
            // get_base_url will print the error
            return -1;
        }
        printf("Discovery response parsing successful.\n");
    }

    if (!sync_response) {
        sync_response = run_http_sync(config.net, config.cpid, config.duid);
        if (NULL == sync_response) {
            // Sync_call will print the error
            return -2;
        }
        printf("Sync response parsing successful.\n");
    }

    // We want to print only first 4 characters of cpid
    lib_config.device.env = config.env;
    lib_config.device.cpid = config.cpid;
    lib_config.device.duid = config.duid;

    if (!config.env || !config.cpid || !config.duid) {
        printf("Error: Device configuration is invalid. Configuration values for env, cpid and duid are required.\n");
        return -1;
    }

    lib_config.event_functions.ota_cb = config.ota_cb;
    lib_config.event_functions.cmd_cb = config.cmd_cb;
    lib_config.event_functions.msg_cb = on_message_intercept;

    lib_config.telemetry.dtg = sync_response->dtg;

    char cpid_buff[5];
    strncpy(cpid_buff, config.cpid, 4);
    cpid_buff[4] = 0;
    printf("CPID: %s***\n", cpid_buff);
    printf("ENV:  %s\n", config.env);


    if (config.auth_info.type != IOTC_AT_TOKEN &&
        config.auth_info.type != IOTC_AT_X509 &&
        config.auth_info.type != IOTC_AT_TPM &&
        config.auth_info.type != IOTC_AT_SYMMETRIC_KEY
        ) {
        printf("Error: Unsupported authentication type!\n");
        return -1;
    }

    if (config.auth_info.type == IOTC_AT_SYMMETRIC_KEY || config.auth_info.type == IOTC_AT_TPM) {
        printf("Error: Configuration tyoe \"SymmetricKey\" is not supported.\n");
        return -1;
    }

    if (config.auth_info.type == IOTC_AT_X509 && (
            !config.auth_info.data.cert_info.device_cert ||
            !config.auth_info.data.cert_info.device_key)) {
        printf("Error: Configuration authentication info is invalid.\n");
        return -1;
    }

    if (config.auth_info.type == IOTC_AT_TOKEN) {
        if (!sync_response->broker.pass || strlen(sync_response->broker.pass) == 0) {
            printf("Error: Unable to obtain SAS token for Token authentication.\n");
            return -1;
        }
    }
    if (!iotcl_init(&lib_config)) {
        printf("Error: Failed to initialize the IoTConnect Lib\n");
        return -1;
    }

    // MQTT connection certificate setup:
    config.net->setCACert(CERT_BALTIMORE_ROOT_CA);
    if (config.auth_info.type == IOTC_AT_X509) {
        config.net->setCertificate(config.auth_info.data.cert_info.device_cert);
        config.net->setPrivateKey(config.auth_info.data.cert_info.device_key);
    }

    IotConnectMqttClientConfig mqtt_config;
    mqtt_config.sr = sync_response;
    mqtt_config.status_cb = config.status_cb;
    mqtt_config.c2d_msg_cb = on_mqtt_c2d_message;
    mqtt_config.auth = &config.auth_info;
    mqtt_config.net = config.net;
    mqtt_config.mqtt_buffer_size = config.mqtt_buffer_size;    
    ret = iotc_mqtt_client_init(&mqtt_config);

    if (ret) {
        printf("Failed to connect!\n");
        return ret;
    }

    return ret;
}