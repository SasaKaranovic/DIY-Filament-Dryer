# How to build firmware

For our firmware development, we are going to use Arduino. Mainly because it's user friendly, easy to use and
most hobbyists are familiar with it or have already used it. While we could build te same or even more efficient
firmware using C and ESP-IDF, we are going to stick with Arduino and hope that this allows the project to be
more friendly and easier to use/modify by wider DIY/hacker community.

## How can I build/compile firmware?
Arduino has many third party libraries that you can use and also build firmware for many different boards.
However, depending for which board you are building firmware and also which libraries your project (sketch) uses,
you might need to manually install them or download them trough board/library manager.

While above mentioned solution works fine, it can lead to issues like installing the "wrong/different" version
of the library, different board files, "wrong/different" build/upload tools and so on. On top of that, since board files
and libraries are shared, we could install a library or a board file while working on another project that breaks our current project.
Hopefully you can see why this is not so great for open-source open-hardware project where we want everyone to be able to easily
checkout a repository and hit the ground running without spending hours setting up and debugging what library or board files are missing.

Luckily we can automate/avoid this by using a build system like PlatformIO. So below we will have two ways of building
the firmware, one will be "easy" by leveraging PlatformiIO build system, and the other one will be a manual one by using
Arduino IDE and manually searching and downloading all the board/library files.

## - The super easy way - Compile and upload using PlatformIO build system  - 

### 1. Install PlatformIO
Installing PlatformIO CLI is pretty straight-forward and also well documented for Windows, Linux and MacOS.
You will need to follow few steps and get PlatformIO CLI installed, detailed tutorial can be found at https://platformio.org/install/cli
Make sure to install [PlatformIO Core](https://docs.platformio.org/en/latest//core/installation.html#installation-methods 'https://docs.platformio.org/en/latest//core/installation.html#installation-methods') and allso that it is available trough [shell](https://docs.platformio.org/en/latest//core/installation.html#piocore-install-shell-commands 'PlatformIO Core - Install Shell Commands¶').

### 2. Build firmware
Open shell/command-prompt and navigate to 'Firmware/platformio' folder.
1. Compile the firmware by typing `pio run`, PlatformIO will download all the required board files and libraries and finally compile the firmware.
2. Upload the firmware with `pio run --target upload --upload-port <COM-PORT>`. Make sure to replace `<COM-PORT>` with your ESP32's COM port (ie COM1 or /dev/ttyACM0)
3. Upload the file system (Web page) with `pio run --target uploadfs --upload-port <COM-PORT>`, again replace `<COM-PORT>` with your ESP32's COM port.

Every time you make a firmware change, you need to run steps #1 and #2.
Every time you make a change to the web page (anything inside `data` folder) you only need to run step #3.


## - The less easy way - Compile using Arduino IDE and manually install all board files and libraries

### 1. Install Arduino IDE
Download and install Arduino IDE from https://www.arduino.cc/en/software

### 2. Install ESP32 board files
Add ESP32 board files to you Arduino IDE
1. Go to `File -> Preferences`
2. Find `Additional Board Manager URLs:` and add `https://dl.espressif.com/dl/package_esp32_index.json`
3. Go to `Boards -> Board manager`.
4. Search for `esp32`.
5. Find a board package title `esp32 -> by Espressif Systems` and install it.
(Currently we are using version 1.0.6, in case you have any issues, try installing this exact version)

### 3. Install libraries used by the project
Install third party libraries that required
1. Go to `Sketch -> Include Library -> Manage Libraries... (CTRL+SHIFT+I)`
2. Find libraries from the list below and install them one by one

- Adafruit SHT31 Library (version 2.0.0)
- Adafruit Si7021 Library (version 1.5.0)
- DallasTemperature (version 3.9.0)
- OneWire (version 2.3.6)

### 4. Install some more libraries
You will manually have to install following libraries.
One way is to create a `Libraries` folder in your sketch folder. Then download the libraries and place them in Libraries folder.

- https://github.com/me-no-dev/AsyncTCP
- https://github.com/me-no-dev/ESPAsyncWebServer

### 5. Select build board
You need to tell Arduino IDE for which board we want to build the firmware.
Go to `Tools -> Board: -> ESP32 Arduino` and select `ESP32 Dev Module`
Go to `Tools -> Port` and select your ESP32 COM port

### 6. Compile and upload the firmware
You need to compile and upload firmware.
Go to `Sketch -> Upload`
You will need to do this step every time you make a change to the firmware.

### 7. Upload the file system
You need to tell Arduino IDE which partition scheme we want to use
Go to `Tools -> Partition Scheme` and select `1MB APP / 3MB SPIFF`
Now you can go upload file system trough `Tools -> ESP32 Sketch Data Upload` menu item.
If you are missing above menu item, you will need to manually install [ESP32 Sketch Data Upload tool](https://github.com/me-no-dev/arduino-esp32fs-plugin).
You will need to do this step every time you make a change to the web page (or anything inside the data folder)


[<- Go back to repository root](../README.md)
