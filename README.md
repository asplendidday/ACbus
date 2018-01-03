# ACbus

App for the Pebble smartwatch to retrieve ASEAG bus schedules. (ASEAG is the local bus service in Aachen, Germany.)

![Photo of bus stop list](photo.jpg)
![Emu screenshot with bus list](mockup1.jpg)
![Emu screenshot in zoom mode](mockup2.jpg)

The original is here: https://github.com/asplendidday/ACbus

I implemented the following changes:

* Made the font larger so it's legible in low-light conditions and/or with less-than-perfect eyesight.
* Starts with list of nearest bus stops.
* Add auto repeat to up/down buttons in bus stop selection.
* Can now select a bus with the up/down buttons.
* Pressing Select in the bus list displays the number of minutes until this bus arrives full-screen ("zoom mode").
* Zoom mode has red background if bus stop is too far away to reach before the bus arrives.
* Reload bus data from the ASEAG server every 20 seconds instead of 30.
* Don't display time since last update all the time. Instead, show a message if there was no update for >30 seconds.
* If offline (=no update of bus data for >30 seconds), estimate bus arrival times based on time since last update.
* Change all messages to German.
* Several fixes and optimizations (details in git log).

To install it on your Pebble, open the pbw file in the Pebble app on your phone, or (after intalling the Pebble SDK on your computer) build it from source and use the `pebble install` command.

---
*Wolfram Rösler • wolfram@roesler-ac.de • https://github.com/wolframroesler • https://twitter.com/wolframroesler • https://www.linkedin.com/in/wolframroesler/*
