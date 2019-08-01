// PIN MAPPINGS (Teensy 3.6)
// -------------------------

/*
  The sole purpose of this code is to use the Teensy to read the output of 
  a pin and pass it to the computer. It is assumed that the data is 
  aligned to a particular clock, which is provided (via some other GPIO).

  This is intended primarily for debugging purposes.
*/

// Pins for reading out ADC data from on-chip memory via GPIO
const int pin_GPIO_CLK_IN = 2;
const int pin_GPIO_DATA_IN = 6;

// Pins for self-testing the Teensy
const int pin_GPIO_CLK_OUT = 3;
const int pin_GPIO_DATA_OUT = 5;

void setup() {
  // Open USB serial port; baud doesn't matter, 12Mbps regardless of setting
  Serial.begin(9600);

  analogWriteResolution(2);

  // Set up pins for reading out ADC data
  pinMode(pin_GPIO_CLK_IN, INPUT);
  pinMode(pin_GPIO_DATA_IN, INPUT);

  // Attaching an interrupt to the input clock to read data on clock edge
  attachInterrupt(digitalPinToInterrupt(pin_GPIO_CLK_IN), memread_ISR, RISING);

  if (false) {
    // Set up pins for supplying dummy clock and data
    pinMode(pin_GPIO_CLK_OUT, OUTPUT);
    pinMode(pin_GPIO_DATA_OUT, OUTPUT);
  
    // Turning on the clock and data
    analogWriteFrequency(pin_GPIO_CLK_OUT, 5000000);
    analogWriteFrequency(pin_GPIO_DATA_OUT, 1000000);
    analogWrite(pin_GPIO_CLK_OUT, 2);
    analogWrite(pin_GPIO_DATA_OUT, 2);
  }
}

void loop() {
}

void  memread_ISR() {
   int val = digitalRead(pin_GPIO_DATA_IN);
   Serial.println("Ah!");
   Serial.println(val);
}
