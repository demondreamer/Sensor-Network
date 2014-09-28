Sensor-Network
==============

Sensor network using Arduinos with RFM69HW transceivers

My sensor network is inspired by and uses code from Eric Tsai. Much thanks go to his work. Here is his github:
https://github.com/homeautomationyay/OpenHab-RFM69
The sketches for the RFM69/Wiznet Arduinos can be found there.

I'm using arduino clones, specifically the Buono UNO R3. They are around $10 on ebay and have some nice features like 3.3v switch and an onboard regulator that supports much wider input voltages. The trancievers I'm using are the RFM69HW. So far I get stable link at least 1200 feet with a simple wire antenna soldered on, and that test was with the reciever indoors. Now that the base station is mounted outside, I expect that to double at least. The base station is 2 Buono Arduinos tied together via the SPI bus. One to handle the Wiznet ethernet shield communications and the other to handle the RFM69HW communications. Apparantly the Wiznet and RFM69 don't play well together on the same Arduino. On my computer I'm running OpenHAB as my interface and rules engine and MQTT to provide transport of my sensor info. Again, most of the hard work was done by Eric Tsai. I just followed his examples and methods.
