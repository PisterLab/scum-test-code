# scum-test-code

## Build

* install
    * https://www.keil.com/demo/eval/arm.htm, default settings (`MDK528A.EXE` known to work)
* open `scum-test-code\scm_v3c\code.uvprojx`
* creates `scum-test-code\scm_v3c\code.bin`

## Bootload

* install
    * https://www.python.org/downloads/ (`Python 3.7.4` known to work)
    * Arduino IDE (`arduino-1.8.9-windows.zip` known to work)
    * Teensyduino (`TeensyduinoInstall.exe` known to work)
* connect both Teensy+SCuM board to computer, creates a COM port for each
* run `scum-test-code\scm_v3c\3wb.py`
