#!/bin/bash

set -e # fail on errors

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
  # replace all #include <iotocnnect*> with quoted include to indicate local file inclusions for Arduino framework
  sed -iE 's/#include <(iotc[a-z_.]+)>.*/#include "\1"/g' esp-sensors-demo/esp-sensors-demo.cpp
  popd
fi