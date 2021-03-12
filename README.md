# MOE-meter
Please note, you have to use a dual-core ESP32 module to sense the disk rotation in one core and sending data at the second core. The code has been written according to no battery supported just for cost-saving. I used photosensor resistor to detect small 220v bulb to detect blackout and saving the rotation counter and KWh before esp power down.  
Sim 800l module is used as a data transmitter. TX, RX are connected to ESP32 pin 17, 16 respectively. 
IR sensor input to ESP32 pin number is 15. 

In regard thingsboard, the JSON file in above could be imported to the thingsboard platform and link with the ESP32 code. 
