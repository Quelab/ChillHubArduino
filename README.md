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

chInterface ChillHub;

void setup() {
 ChillHub.setup("your_name_here", UUID);
}

void loop() {
 ChillHub.loop();
}
```
Obviously, replacing "your_name_here" you've selected for your ChillHub accessory.  Your UUID needs to be a 
[Version 4](http://en.wikipedia.org/wiki/Universally_unique_identifier#Version_4_.28random.29) random UUID. 

You can set the UUID on your device by using the set-device-uuid.js script located in the
[ChillHub firmware](https://github.com/FirstBuild/chillhub-firmware).  Simple plug your device into you computer and execute:
```
sudo node set-device-uuid.js
```

Available Functions
-------------------
###Data From Fridge
*Note: Communication with the fridge is currently disabled in ChillHub.*
```c++
void subscribe(unsigned char type, chillhubCallbackFunction cb);
void unsubscribe(unsigned char type);
```
These functions allow your USB device to subscribe to or unsubscribe from data originating from the fridge.  ChillHub supports the same data streams as green-bean (https://github.com/GEMakers/gea-plugin-refrigerator).  When using these functions, use values from the ChillHubDataTypes enum (in chillhub.h) for the _type_ field.  When creating your callback function, you'll need to ensure that the argument to your callback function matches the data type returned by the subscription's data stream.

###Alarms and Time
```c++
void setAlarm(unsigned char ID, char* cronString, unsigned char strLength, chillhubCallbackFunction cb);
void unsetAlarm(unsigned char ID);
void getTime(chillhubCallbackFunction cb);
```
These functions give your USB device the ability to find out the current local real-time as well as to be notified when particular times occur.  Note that the ID field in setAlarm and unsetAlarm is a unique identifier for you to manage your device's alarms.  Because of the way that this is used, it is important the ID be an ascii printable character.  Further, note that the callback functions used here accepts a ```unsigned char[4]``` argument and the argument's contents will be _[month, day, hour, minute]_.

###Data to/from the Cloud
Data can be exchanged with the cloud.  The device registers resources with the cloud in order to make data from your device available remotely.
Additionally, a listener can be added for each resource to allow the resource on your device to be modified remotely.

To register a read-only device in the cloud
```c++
ChillHub.createCloudResourceU16("Analog", AnalogID, 0, 0);
```
To register a cloud resource and a listener to update that resource:
```
chillhub.addcloudlistener(ledid, (chillhubcallbackfunction)setled);
chillhub.createcloudresourceu16("led", ledid, 1, 0);

static void setLed(uint8_t value) {
   digitalWrite(LedL, value);
}
```


These functions allow communication to and from the ChilHub data store; see the Inventory Management Platform project (https://github.com/FirstBuild/InventoryMgmt).  The schema for each ChillHub peripheral defines what these message types are and the payload and callback functions used in the Arduino must match the schema.
