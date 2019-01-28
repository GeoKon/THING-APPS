Hardware Setup
--------------

Run the PEBBLE application with GKE's extensions. Layout is shown below. 

![](https://i.imgur.com/9n1zp5E.jpg)

Source is listed below.
```
IC||60|128|1||U?|||NodeMCU|IC||NodeMCU_1
Wire||155|402|21||11|#FF9900|2|11||10|
Note||13|448|2||||1||GND||NOTEPAD_12
Note||13|476|2||||1||+3.3V||NOTEPAD_12
Wire||128|402|21||11|#FFFF00|3|11||10|
Wire||128|44|21||11|#FF9900|2|11||10|
Note||15|3|2||||1||GND||NOTEPAD_12
Note||14|36|2||||1||5V||NOTEPAD_12
Wire||155|17|21||11|#FFFF00|3|11||10|
Wire||263|17|21||11|#FFFF00|3|11||10|
Wire||344|402|21||11|#FF9900|2|11||10|
IC||656|9|1||U?|||HM10|IC||HM10_1
Wire||723|402|21||11|#FF9900|2|11||10|
Wire||695|402|21||11|#FFFF00|3|11||10|
IC||837|46|2||U?|||OLED128x64|IC||OLED128x64_2
Wire||1047|402|21||11|#FF9900|2|11||10|
Wire||1020|402|21||11|#FFFF00|3|11||10|
Wire||481|391|12||11|#FF0000|19|11||10|
Wire||454|403|13||22|#FF0000|19|11||12|
Wire||238|336|12||11|#FF0000|20|11||10|
Wire||265|317|13||11|#FF0000|18|11||10|
Note||233|391|2||||3||Tx<br/>D||NOTEPAD_32
Note||260|392|2||||3||Rx<br/>D||NOTEPAD_32
Note||385|60|1||||2||<h4>Bluetooth LE to WiFi Gateway for B35T meter</h4>||NOTEPAD_21
BREADBOARDSTYLE=BB36
SHOWTHETOPAREA=false
```