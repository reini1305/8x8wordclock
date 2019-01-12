# 8x8wordclock
This project is about an Attiny85-based word clock. The underlying kit can be purchased at https://shop.cyntech.co.uk/products/wordclock-kit.

While the original kit is built around a Raspberry Pi, I wanted to use an Adafruit Trinket and a DS3231 RTC instead. The Raspberry Pi just seemed to be overkill.

The source is based on the project of Andy Doro: https://github.com/andydoro/WordClock-NeoMatrix8x8

I've had to heavily modify the code to fit it within 5 KB of Flash and 512 bytes of RAM.

![Finished](pictures/img1.png)

Hardware
-------
 
 - [Trinket 5V](https://www.adafruit.com/product/1501) 
 - [DS3231 RTC breakout](https://www.neuhold-elektronik.at/catshop/product_info.php?products_id=7269) (or similar)
 - [Wordclock kit 8x8](https://shop.cyntech.co.uk/products/wordclock-kit)
 - A switch to select the color mode (optional)

Wiring
-------

SOON
 
Software
-------
 
This code requires the following libraries:
 
 - [RTClib](https://github.com/adafruit/RTClib)
 - [Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
 - [Bounce2](https://github.com/thomasfredericks/Bounce2)
