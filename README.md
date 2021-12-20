## About

This repository contains the IoTConnect Arduino SDK and code samples that make use of the SDK.

## Supported Boards

The code has been developed and tested with [SparkFun ESP32 Thing](https://www.sparkfun.com/products/13907)

The SDK depends on the WiFiSecure library and will require a WiFi capable board or a WiFi shield.

The sample provided with the code makes use of the GPIO #2 for LED and #10 for the button example, 
and it requires an ESP32 board with.

## Project Setup

The recommended development environment for this project is PlatformIO, due to improved project structure, 
ease of use, and code completion features. Arduino IDE is also supported.

### PlatformIO

* Clone this repository, or Download and extract the PlatformIO package from the [Releases](https://github.com/avnet-iotconnect/iotc-arduino-sdk/packages)
page into a directory of your choosing.
* Download and install Microsoft [Visual Studio Code](https://code.visualstudio.com/download) (VSCode) for your OS.
* Open VSCode and in the menu navigate to **View -> Extensions**. 
* Search for *PlatformIO* and install the **Platform IO IDE** extension.
* Open the PlatformIO plugin in the lft sidebar and navigate to **PIO Home -> Open** on the left side panel.
* Click the **Open Project** button and navigate to the directory containing platformio.ini, where the project is extracted. 
* Click the **Open <project name>** button in the dialog.
* Click **Yes** when asked about trusting the authors.
* Modify *include/wifi_config.h* and *include/app_config.h* per your WiFi, IoTConnect device, and account credentials.
* Connect the device, open the PlatformIO extension in the sidebar and click **General -> esp32thing -> Upload and Monitor** 
in order to upload the code to your device and see the device console messages.
  
### Arduino IDE

* Download and extract the Arduino IDE package from the [Releases](https://github.com/avnet-iotconnect/iotc-arduino-sdk/packages)
page into a directory of your choosing.
* Download and install [Arduino IDE](https://www.arduino.cc/en/software) for your OS.
* Open the file explorer and open esp-sensors-demo.ino file in the examples/esp-sensors-demo directory of the extracted package.
Alternatively, you can open the IDE and open this sketch by using the **File -> Open** menu option.
* Select the **Sketch -> Include Library -> Add .ZIP Library** menu item,
and *Open* the downloaded IoTConnectSDK.ZIP, or the directory where it is extracted. this will install the IoTConnectSDK library. 
* Using the **Sketch -> Include Library** menu item, install the PubSubClient library.
* If not using an ESP board, install the LibPrintf library (using the zip URL).
* Modify *wifi_config.h* and *app_config.h* per your WiFi, IoTConnect device, and account credentials.
* Connect the device, and click the **Upload** button. If the device fails to upload, 
press the reset button on the board while the upload process is connecting -- while the dots and underscores are printed in the log.  
* Select **Tools -> Serial Monitor** in the menu, in order to see the device console messages.
