# esp32_rekognition
The goal is to count people in meeting room, and liberate the room if there is nobody inside (our meeting room systems is based on google calendar).
The arduino part just take a shot, and sent it to a python script. The python script use rekognition to draw bbox around people and store it in a webserver.
This was just a PoC (so code is a little bit ugly).

To use it, install ESP-IDF as follow :
https://github.com/espressif/esp-idf

Install arduino components :
https://github.com/espressif/arduino-esp32/blob/master/docs/esp-idf_component.md

Compile & flash : make flash

I've tested it on battery (cr123A), and it can last one year if you enable only one core, you set the frequency at 40 Mhz, and you disable everything possible.


