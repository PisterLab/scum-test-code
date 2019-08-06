# teensy_uC_adc.ino
At a low level, this has access to the following:
* All GPIO connections
* Analog scan chain programming
* Digital data output
* 3-wire bus and optical bootloading

This does _not_ have access to:
* Digital scan chain. This was removed because there weren't enough pins available to get all the GPIO connections needed.
