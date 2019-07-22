// PIN MAPPINGS (Teensy 3.6)
// ------------------

// Analog Scan Chain
const int pin_scan_in = 7;
const int pin_scan_out = 10;
const int pin_scan_clk = 8;
const int pin_scan_load = 9;

// LDO enables
const int pin_en_cam = 22;
const int pin_en_if = 21;
const int pin_en_rf = 20;

// Camera Giblets
//const int pin_cam_clk_WRONG = 19; // Accidentally chose a pin without PWM
const int pin_cam_clk = 29;
const int pin_pix_out = 14;
//const int pin_pga_clk_WRONG = 18; // Accidentally chose a pin without PWM
const int pin_pga_clk = 30;
const int pin_pga_out = 15;
const int pin_test_pga_out = 17;
const int pin_test_pga_in = 16;

// Radio Giblets
const int pin_en_tx = 6;
const int pin_PA_ftune_0 = 5;
const int pin_PA_ftune_1 = 4;
const int pin_PA_amp_tune_0 = 3;
const int pin_PA_amp_tune_1 = 2;

// Variables for command interpreter
String inputString = "";   
boolean stringComplete = false; 

// Runs once at power-on
void setup() {
  // Open USB serial port; baud doesn't matter; 12Mbps regardless of setting
  Serial.begin(9600);

  // Reserve 200 bytes for the inputString:
  inputString.reserve(200);

  // Setup pins for analog scan chain
  pinMode(pin_scan_in, OUTPUT);
  digitalWrite(pin_scan_in, HIGH);
  pinMode(pin_scan_clk, OUTPUT);
  digitalWrite(pin_scan_clk, HIGH);
  pinMode(pin_scan_load, OUTPUT);
  digitalWrite(pin_scan_load, HIGH);
  pinMode(pin_scan_out, INPUT);

  // Setup pins for LDO enables
  pinMode(pin_en_cam, OUTPUT);
  digitalWrite(pin_en_cam, HIGH);
  pinMode(pin_en_if, OUTPUT);
  digitalWrite(pin_en_if, HIGH);
  pinMode(pin_en_rf, OUTPUT);
  digitalWrite(pin_en_rf, HIGH);

  // Setup pins for camera giblets
  pinMode(pin_cam_clk, OUTPUT);
  pinMode(pin_pga_out, INPUT);
  pinMode(pin_pix_out, INPUT);
  pinMode(pin_pga_clk, OUTPUT);
  pinMode(pin_test_pga_out, INPUT);
  pinMode(pin_test_pga_in, INPUT);   // Unnecessary; comes from the function generator.

  // Setup pins for radio giblets
  pinMode(pin_en_tx, OUTPUT);
  digitalWrite(pin_en_tx, LOW);
  pinMode(pin_PA_ftune_0, OUTPUT);
  digitalWrite(pin_PA_ftune_0, LOW);
  pinMode(pin_PA_ftune_1, OUTPUT);
  digitalWrite(pin_PA_ftune_1, LOW);
  pinMode(pin_PA_amp_tune_0, OUTPUT);
  digitalWrite(pin_PA_amp_tune_0, LOW);
  pinMode(pin_PA_amp_tune_1, OUTPUT);
  digitalWrite(pin_PA_amp_tune_1, LOW);
}

// Runs repeatedly
// Monitors uart for commands and then makes a function call
void loop() {
  // Do nothing until a '\n' terminated string is received
  if (stringComplete) {

    if (inputString == "ascwrite\n"){
      asc_write();
    }

    else if (inputString == "ascload\n") {
      asc_load();
    }

    else if (inputString == "ascread\n") {
      asc_read();
    }

    else if (inputString == "test_pga\n") {
      test_pga();
    }

    // Reset to listen for a new '\n' terminated string over serial
    inputString = "";
    stringComplete = false;
  }
}

void asc_write() {
  Serial.println("Executing ASC Write");
  int count = 0;
  char scanbits[72];

  // Loop until all 72 scan chain bits received over serial
  while (count != 72) {

    // Read one bit at a time over serial
    if (Serial.available()) {
      scanbits[count] = Serial.read();
      count++;
    }
  }

  // Once all bits are received, bitbang to ASC input
  for (int x=0; x<72; x++) {
    if (scanbits[x] == '1') {
      digitalWrite(pin_scan_in, HIGH);
    }
    else if (scanbits[x] == '0') {
      digitalWrite(pin_scan_in, LOW);
    }
    else {
      Serial.println("Error in the ASC Write");
    }
    // Pulse the clock
    atick();
  }
  Serial.println("ASC Write Complete");
}

void asc_load() {
  Serial.println("Executing ASC Load");
  digitalWrite(pin_scan_load, LOW);
  delayMicroseconds(1);
  digitalWrite(pin_scan_load, HIGH);
}

void asc_read() {
  String st = "";
  int val = 0;
  // First bit
  val = digitalRead(pin_scan_out);
  if (val)
    st = String(st + "1");
  else
    st = String(st + "0");

  // Next 71 bits
  for (int i=1; i<72; i = i+1) {
    atick();
    val = digitalRead(pin_scan_out);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  Serial.println(); //Terminator

  return;
}

void test_pga() {
  /*
  Starts the PGA clock at 50% duty cycle and
  analog reads the test PGA output at the rising edge (ideally before the output
  has moved significantly) of the clock. Why oh why did I not introduce ready/valid 
  or at least a "done" in this thing?! 
  */

  // Set analog read resolution
  analogReadResolution(10);

  // Get PGA clock running with 50% duty cycle
  analogWriteResolution(2);
  analogWriteFrequency(pin_pga_clk, 50000);  // 100kHz
  analogWrite(pin_pga_clk, 2);

  // Err, how do I sample when the clock is low and is about to go high?
  attachInterrupt(digitalPinToInterrupt(pin_pga_clk), _ISR_test_pga_, RISING);
  return;
}

void _ISR_test_pga_() {
  /*
  Analog reads the test PGA's analog output pin and sends it 
  to the serial. Internal use only.
  */
  int val = analogRead(pin_test_pga_out);
  float voltage = val * (3.3/1023);
  Serial.println(voltage, DEC);
}

void atick() {
  digitalWrite(pin_scan_clk, LOW);
  delayMicroseconds(10);
  digitalWrite(pin_scan_clk, HIGH);
}

void serialEvent() {
  if (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
