ChillHubArduino
===============
ChillHub is a system for creating USB devices that extend the functionality of your fridge.  It consists of a raspberry pi (http://www.raspberrypi.org/), a green bean (https://firstbuild.com/greenbean/), and a USB hub in a refrigerator door.  ChillHub acts as a mailman the delivers messages from the cloud and the fridge to USB devices and from the USB devices back to the cloud.

ChillHub compatible USB peripherals communicate with ChillHub over USB serial.  This project contains an arduino library for developing such peripherals.

Arduino Library
===============
Clone this repository into your Arduino/Libraries directory, and you should see a _chillhub-demo_ example in your Arduino Sketchbook.  That's a good place to look for jump starting your project.

How To Use
----------
Start off your arduino sketch with
```c++
#include "chillhub.h"

void setup() {
 ChillHub.setup("your_name_here", 14);
}

void loop() {
 ChillHub.loop();
}
```
Obviously, replacing "your_name_here" with whatever ID you've selected for your ChillHub accessory.

Available Functions
-------------------
###Data From Fridge
```c++
void subscribe(unsigned char type, chillhubCallbackFunction cb);
void unsubscribe(unsigned char type);
```
These functions allow your USB device to subscribe to or unsubscribe from data originating from the fridge.  ChillHub supports the same data streams as green-bean (https://github.com/GEMakers/gea-plugin-refrigerator).  When using these functions, use values from the ChillHubDataTypes enum (in chillhub.h) for the _type_ field.  When creating your callback function, you'll need to ensure that the argument to your callback function matches the data type returned by the subscription's data stream (see **Message Types** below).

###Alarms and Time
```c++
void setAlarm(unsigned char ID, char* cronString, unsigned char strLength, chillhubCallbackFunction cb);
void unsetAlarm(unsigned char ID);
void getTime(chillhubCallbackFunction cb);
```
These functions give your USB device the ability to find out the current local real-time as well as to be notified when particular times occur.  Note that the ID field in setAlarm and unsetAlarm is a unique identifier for you to manage your device's alarms.  Further, note that the callback functions used here accepts a ```unsigned char[4]``` argument and, as noted below, the argument's contents will be _[month, day, hour, minute]_.

###Data to/from the Cloud
```c++
void addCloudListener(unsigned char msgType, chillhubCallbackFunction cb);
void sendU8Msg(unsigned char msgType, unsigned char payload);
void sendU16Msg(unsigned char msgType, unsigned int payload);
void sendI8Msg(unsigned char msgType, signed char payload);
void sendI16Msg(unsigned char msgType, signed int payload);
void sendBooleanMsg(unsigned char msgType, unsigned char payload);
```
Still under construction.
