## Pico-Pong

Pong running on a Raspberry Pi Pico written in C++, the paddle is controlled by a potentiometer, the game is displayed on an SSD1306 OLED screen.  
I2C is used to communicate with the oled. ADC is used for the potentiometer.

### Hardware

-Raspberry Pi Pico/Pico W  
-10kOhm potentiometer  
-128x64px monochrome SSD1306 OLED using I2C  