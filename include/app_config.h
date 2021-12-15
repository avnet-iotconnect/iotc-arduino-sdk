//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 6/15/20.
//

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "iotconnect.h"

#define IOTCONNECT_CPID "your-cpid"
#define IOTCONNECT_ENV  "your-env"

// Device Unique ID
#define IOTCONNECT_DUID "your-device-unique-id"

// from iotconnect.h IotConnectAuthType
#define IOTCONNECT_AUTH_TYPE IOTC_AT_TOKEN

// PEM format certificate and private key, if using X509 type auth.
// For example:
#define IOTCONNECT_DEVICE_CERT \
"-----BEGIN CERTIFICATE-----\n"\
"...\n"\
"-----END CERTIFICATE-----"

#define IOTCONNECT_DEVICE_KEY \
"-----BEGIN RSA PRIVATE KEY-----\n"\
"...\n"\
"-----END RSA PRIVATE KEY-----"

#endif // APP_CONFIG_H