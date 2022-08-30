//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/28/21.
//

#include <stdlib.h>
#include <string.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include "iotc_http_request.h"
#include "iotconnect_certs.h"

int iotconnect_https_request(
        Client *net,
        IotConnectHttpResponse *response,
        const char *url,
        const char *send_str
) {
    int ret;
    HTTPClient *http = new HTTPClient();
    response->data = NULL;
    if (!http->begin(url, CERT_GODADDY_INT_SECURE_G2)) {
        printf("iotconnect_https_request() failed to initate the HTTP connection to %s.\n", url);
        return -1;
    }    
    int tries_left = 5;
    do {
        int data_len;
        if (NULL == send_str) {
            data_len = http->GET();
        } else {
            //printf("DATA>\n%s\n<", send_str);
            http->addHeader("Content-Type", "application/json");
            data_len = http->POST((uint8_t *)send_str, strlen(send_str));
        }
        if (data_len == 0) {
            printf("iotconnect_https_request() no data from url %s.", url);
            ret = -2;
        } else if (data_len < 0) {
            printf("iotconnect_https_request() failed to connect to %s.", url);
            ret = -3;
        } else {
            ret = 0; // all good so far..
            break; 
        }
        tries_left--;
        printf(" Retries left %d...\n", tries_left);
    } while (tries_left > 0);

    if (ret < 0) { // exhausted retries
        return ret;
    }
    String payload = http->getString();     
    //printf("REPONSE>\n%s\n<", payload.c_str());    
    if (NULL == response) {
        printf("iotconnect_https_request() requires a valid IotConnectHttpResponse pointer.");
        return -4;
    }    
    response->data = (char *) malloc(strlen(payload.c_str()) + 1);
    strcpy(response->data, payload.c_str());
    return 0;
}


void iotconnect_free_https_response(IotConnectHttpResponse *response) {
    if (response->data) {
        free(response->data);
    }
    response->data = NULL;
}
