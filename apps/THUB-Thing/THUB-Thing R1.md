Thermostat Hub
--------------
(GKE Labs Rev 1: Feb 3, 2019)

** Overview **
`Thermostat Hub` connects multiple thermostats to a single dashboard panel. It collects temperature and humidity readings from thermostats using a LAN API, and presents a single stream to the Internet.

This implementation is specific to:
1. the Radio Thermostats CT50 or CT80
2. the NodeMCU or Heltec modulo
3. the Thinger.io IoT cloud platform

The above components were selected for good reasons; see explanations in the "Components" section.

** Benefits **
Traditional home thermostats connect with the Internet independently of each other, either as "web servers" or as "clients" to a cloud application. Creating local web server (including the MQTT protocol) is simple and attractive to the IoT community because of its simplicity. But this requires special configurations to the home gateway with port forwarding or DMZ. The second approach teats all thermostats as "clients" to a cloud web application; in turn, this application requires individual connections with each thermostat.

Using the Thermostat-Hub, all thermostats are connected via WiFi to a single point which in turn connects to a cloud application -- in this case the Thinger.io platform. 

** Block Diagram **
The following block diagram illustrates the system configuration.

![](https://i.imgur.com/h7X0xhR.gif)

** Components **
1. The reason for selecting the Radio Thermostats is the well documented REST API available in the local LAN. These thermostats have been around for many years, and use USNAP modules for either Z-WAVE or WiFi connectivity.
![](https://i.imgur.com/RgLUPFI.gif)

2. NodeMCU is the cheapest (under $4.00) platform that combines a 32-bit MCU, USB interface to local host, full WiFi connectivity (STA and SoftAP modes), and a superb development environment -- the Arduino. The Heltec module adds an integrated OLED display on the same module.
 
![](https://i.imgur.com/76a7q8D.gif)
3. Thinker.io is an excellent IoT Cloud Platform with highly optimized client code and open-source of the cloud servers. It contains "just enough" of all cloud services a typical developer needs, such as administrator panels, dashboards, cloud storage and superb documentation. The support community is also very active.

** Dashboard **
The Thinger.io dashboard is shown below.
![](https://i.imgur.com/MnQrrMk.gif)
The left side green boxes display status information and include selected controls. For example, you can see which device is queried (1st upper left box), the communication error rates to each thermostat, enabling or disabling collection of data to Dropbox, interval of collection, etc.

** Selected EEPROM Parameters **
- `ssid` set by default to home SSID
- `pwd` set by default to home SSID password
The LAN home network is hardcoded to 192.168.0.xx; the last byte, also known as IP3, is managed by the EEPROM parameters: 
- `OfficeIP3` defines the IP3 for the OfficeTemp thermostat
- `BackYardIP3` defines the IP3 for the BackyardTemp thermostat
- `PatioIP3` defines the IP3 for the PatioTemp thermostat
- `HumidityIP3` defines the IP3 for the Humidity sensor
In this configuration, all thermostats use DHCP -- so the above parameters need to be set correctly to query the thermostats correctly.

