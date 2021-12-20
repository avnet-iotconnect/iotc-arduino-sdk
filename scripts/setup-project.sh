#!/bin/bash

set -e # fail on errors

cd $(dirname "$0")

# prevent accidental commits on these files
# need to undo, if changes to these files are actually needed
# it doesn't do anything for github actions, but use it for local development
git update-index --assume-unchanged include/app_config.h | true
git update-index --assume-unchanged include/wifi_config.h | true

# make the packages for Arduino IDE
if [[ "$1" == "arduino-ide" ]]; then
  pushd ..
  mkdir -p arduino-pkg/examples/esp-sensors-demo
  cp src/* include/* arduino-pkg/examples/esp-sensors-demo
  cp lib/IoTConnectSDK/library.properties README.md lib/IoTConnectSDK/include/* lib/IoTConnectSDK/src/* arduino-pkg/
  # replace all #include <iotocnnect*> with quoted include to indicate local file inclusions for Arduino framework:
  sed -Ei 's/#include <(iotc[a-z_.]+)>.*/#include "\1"/g' arduino-pkg/esp-sensors-demo/esp-sensors-demo.cpp
  popd
fi