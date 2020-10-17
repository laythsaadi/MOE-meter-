# MOE-meter
Please note, you have to use a dual-core ESP32 module to sense the disk rotation in one core and sending data at the second core. The code has been written according to no battery supported just for cost-saving. So, it has been added 50 rotations after restoring the power as a substitution. However,  you are free to support that if you need it and remove this statement to be a more accurate reading. 
Sim 800l module is used as data transmitter. TX, RX are connected to ESP32 pin 17, 16 respectively. 
IR sensor input to ESP32 pin number is 34. 
