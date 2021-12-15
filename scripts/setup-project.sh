#!/bin/bash

cd $(dirname "$0")

# prevent accidental commits on these files
# need to undo, if changes to these files are actually needed
git update-index --assume-unchanged src/include/app_config.h
git update-index --assume-unchanged src/include/wifi_config.h

# make packages for Arduino IDE
if [[ "$1" == "arduino-ide" ]]; then
  pushd ..
  cp -r src esp-sensors-demo
  cp lib/iotc-arduino-sdk/include/* esp-sensors-demo/
  cp lib/iotc-arduino-sdk/src/* esp-sensors-demo/
  popd
fi