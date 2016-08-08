# Wireless-433Mhz-Rain-Sensor (Part 1 The Transmitter)
Arduino-based wireless rain sensor

Oh no! It's raining and the washing (or cat) is still out! It's VERY easy to create a wireless rain sensor that communicates via a 433Mhz (or 315Mhz) wireless link giving you that all important heads-up that the weather has changed.

This is the transmitter part of the project (with a very simple breadboard receiver to prove it's all working) that transmits via a 433Mhz link the temperature, the humidity, whether it's raining (and the level of rain falling) and the light level too just for good measure. Oh, and it has a capacitive touch sensor just for good measure. Well, why not?

This was originally meant to be an early-warning system to alert us that it was starting to rain and the cat needed to be brought in from his cat run, but has turned into a mini-weather station (that might be expanded in the future). Joy!

----------------------------------------------------------------------------------
The Arduino sketches (code) can all be found here as well as on GitHub:
----------------------------------------------------------------------------------

1. Very simple capacitive rain sensor 433Mhz transmitter:

http://bit.ly/2anN82p 


2. A very simple 433Mhz receiver to prove it all works

http://bit.ly/2a3EzNG 


3. The final, full-blown transmitter sketch, using just a pinch of C++ pointers, a soupçon of a template and a je-ne-sais-quoi of techniques you may find useful (but not compulsory) in your own projects, yes, really!

http://bit.ly/2aaSdx0


The DHT11, OneWire and Touch Capacitive libraries are zipped up here but you may wish to find the most up-to-date ones on the Internet.

http://bit.ly/2am7c8A
