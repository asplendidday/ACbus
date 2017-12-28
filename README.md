# ACbus

Pebble app to retrieve ASEAG bus schedules. (ASEAG is the local bus service in Aachen, Germany.)

![Pebble Time mockup](mockup1.jpg)
![Pebble Time Steel mockup](mockup2.jpg)
![Pebble Time mockup](mockup3.jpg)

The original is here: https://github.com/asplendidday/ACbus

I implemented the following changes:

* Made the font larger so it's legible in low-light conditions and/or with less-than-perfect eyesight.
* Starts with list of nearest bus stops.
* Add auto repeat to up/down buttons in bus stop selection.
* Can now select a bus with the up/down buttons.
* Pressing Select in the bus list displays the number of minutes until this bus arrives full-screen ("zoom mode").
* Zoom mode has red background if bus stop is too far away to reach before the bus arrives.
* Don't display time since last update all the time. Instead, show a message only if there was no update for >45 seconds.
* Change all messages to German.
* Several fixes and optimizations (details in git log).

---
*Wolfram Rösler • wolfram@roesler-ac.de • https://github.com/wolframroesler • https://twitter.com/wolframroesler • https://www.linkedin.com/in/wolframroesler/*
