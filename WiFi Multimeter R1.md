WiFi Multimeter
---------------

### What it does
Measures the usual electrical quantities and makes them available to WiFi Clients (laptops, mobile phones, tablets) near the multimeter or anywhere on the Internet.

### Use Cases & Applications
- Measuring high voltage or current away from hazardous circuits
- Using your tablet to see measurements more conveniently
- Home automation: measuring anything and watching it remotely
- Measuring soil resistance and access it via the Internet
- Measuring your car or your UPS battery voltage
- Measuring the amperage drain of any AC or DC circuit 
- Logging battery discharge cycles – creating graphs
- Monitoring (and logging) illumination of your plans – photosensor is required
- Monitoring and logging ambient temperatures
- Logging data during long term experiments (hours, days) and have them available real-time
- … and myriad others

### Block diagrams
![](https://i.imgur.com/qxTWXiS.gif)

![](https://i.imgur.com/pdy9suU.gif)

### User Modes of Operation
There are three modes of operation of the Gateway:
- 	CLI Mode
- 	AP Mode
- 	STA Mode

In CLI mode, the user interfaces with the Gateway via a USB connection to the NodeMCU Serial Port via a generic terminal. This mode is useful for configuration and setup of the unit; for example, the user can test various I/O pins, the EEPROM, the display, WiFi parameters, etc.

![](https://i.imgur.com/9dEjg9f.gif)

The AP Mode is used for connecting the Multimeter to a WiFi Client directly, i.e. not using an infrastructure Access Point. The gateway itself behaves as an Access Point discoverable by the WiFi Clients with SSID “GKE_LABs” and no password. After connecting the WiFi Client, the browser can be directed to 192.168.4.1 which is the main web server published by the Gateway.

The STA Mode is used to connect the Multimeter to the Internet via an infrastructure Access Point. The WiFi Credentials are stored in the EEPROM and, if correct, connection is straightforward. If not the user can use the AP Mode to define the infrastructure SSID, the password and optionally the port number for the web server. Futhermore, instead of using the default DNS for obtaining the IP address of this WiFi Client, the user can specify a fixed IP address.

###Typical Web Pages

![](https://i.imgur.com/jWOJUp7.gif)
![](https://i.imgur.com/RSVtaRk.gif)

###Code Organization
![](https://i.imgur.com/JxRDU5J.gif)
![](https://i.imgur.com/EksuuCs.gif)
