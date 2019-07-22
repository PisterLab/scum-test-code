// Change Log
//  v9:
//  Fixed gpio issues: && and off by 1
//  Updated control for ANYOUT regulators to drive low/hi-Z
//  Slowed down 3wb bootloader and have it strobe hard reset first
//  Changed pin states on DSC pins to start hi-Z to avoid drive fights with gpio --untested

//  v10:
//  Added I2C back in.  Move i2c init calls to after the 3.3V regulator has been turned on

//  v11:
//  I2C changes

//  v12:
//  Added external interrupt strobe
//  Merge I2C changes
//  Add ability to pulse N clock cycles or output continuous 5MHz clock (reuse optical pin - 38)

//  v13:
//  Add modulation pin for LC

//  v14:
//  Only assert ASC_source_select when actually using scan chain, otherwise default it to internal
//  v16:
//  Add digital data output from matlab (for fpga, re-uses pins 33 and 34)
//  Change hard reset strobe time to 500 ms to support hard reset via teensy on fpga

//  v18:
//  Add delay during optical programming to allow for reset time
//  Add 4b5b version of optical programming

//  v19:
//  Add function for doing optical data transfer
//  Add function for optical boot without hard reset

// v20:
// Fixes to digital scan chain readout

//#include <i2c_t3.h>

// PIN MAPPINGS (Teensy 3.6)
// ---------------
// digital data output
const int CLK_OUT = 33;
const int DATA_OUT = 34;
 
// 3wb bootloader
const int clkPin = 32;
const int enablePin = 31;
const int dataPin = 30;

// Misc
const int hReset = 0;
const int sReset = 1;
const int gpo_0 = 2;
const int interrupt_b = 4;
const int asc_source_select = 5;

// Optical bootloader
const int optical_out = 38;

// Digital Scan Chain (DSC)
const int dPHI = 23;
const int dPHIb = 22;
const int dSCANi0o1 = 21;
const int dSCANOUT = 20;

// Analog Scan Chain (ASC)
const int aPHI = 34;
const int aPHIb = 35;
const int aLOAD = 36;
const int aSCANIN = 37;
const int aSCANi0o1 = 33;
const int aSCANOUT = 39;

// LDO Programmable voltages
const int vdda_50 = 10;
const int vdda_100 = 11;
const int vdda_200 = 12;
const int vdda_400 = 24;
const int vddd_50 = 26;
const int vddd_100 = 27;
const int vddd_200 = 28;
const int vddd_400 = 29;

// GPIO direction shift register
const int gpio_clk = 15;
const int gpio_scan_data = 14;

// LDO enables
const int vbat_en = 3;
const int gpio_en = 7;
const int debug_en = 8;
const int vdda_en = 9;
const int vddd_en = 25;
const int ldo33_en = 6;
// ---------------

// Variables for command interpreter
String inputString = "";
boolean stringComplete = false;

// Variables for copying binary bootload data to memory
int iindex;
byte ram[220000];

// Optical programmer variables
int PREAMBLE_LENGTH = 100;  // How many 0x55 bytes to send to allow RX to settle
int REBOOT_BYTES = 200;    // How many 0x55 bytes to send while waiting for cortex to hard reset

int dig_data_bytes = 1500;//4*2000 + 1;

// Delay times for high/low, long/short optical pulses
int p1, p2, p3, p4;

// Runs once at power-on
void setup() {
  // Open USB serial port; baud doesn't matter; 12Mbps regardless of setting
  Serial.begin(9600);
  
  // Reserve 200 bytes for the inputString:
  inputString.reserve(200);

  // Setup pins for 3wb output
  pinMode(clkPin, OUTPUT); // CLK
  pinMode(enablePin, OUTPUT); // EN
  pinMode(dataPin, OUTPUT); // DATA

  // Setup misc pins
  pinMode(hReset, OUTPUT);
  digitalWrite(hReset, HIGH);
  pinMode(sReset, OUTPUT);
  digitalWrite(sReset, HIGH);
  pinMode(interrupt_b, OUTPUT);
  digitalWrite(interrupt_b, HIGH);
  pinMode(asc_source_select, OUTPUT);
  digitalWrite(asc_source_select, LOW);

  // Setup pins for digital scan chain
  // Leave these high-Z until needed or else will fight GPIO
  pinMode(dPHI, INPUT);
  pinMode(dPHIb, INPUT);
  pinMode(dSCANi0o1, INPUT);
  pinMode(dSCANOUT, INPUT);

  // Setup pins for analog scan chain
  pinMode(aPHI, OUTPUT);
  pinMode(aPHIb, OUTPUT);
  pinMode(aLOAD, OUTPUT);
  pinMode(aSCANIN, OUTPUT);
  pinMode(aSCANi0o1, OUTPUT);
  pinMode(aSCANOUT, INPUT);
  digitalWrite(aPHI, LOW);
  digitalWrite(aPHIb, LOW);
  digitalWrite(aLOAD, LOW);
  digitalWrite(aSCANIN, LOW);
  digitalWrite(aSCANi0o1, LOW);

  pinMode(gpio_clk, OUTPUT);
  digitalWrite(gpio_clk, LOW);
  pinMode(gpio_scan_data, OUTPUT);
  digitalWrite(gpio_scan_data, LOW);

  pinMode(vbat_en, OUTPUT);
  digitalWrite(vbat_en, LOW);
  pinMode(gpio_en, OUTPUT);
  digitalWrite(gpio_en, LOW);
  pinMode(debug_en, OUTPUT);
  digitalWrite(debug_en, LOW);
  pinMode(vdda_en, OUTPUT);
  digitalWrite(vdda_en, LOW);
  pinMode(vddd_en, OUTPUT);
  digitalWrite(vddd_en, LOW);
  pinMode(ldo33_en, OUTPUT);
  digitalWrite(ldo33_en, LOW);

  // Pin for optical TX
  pinMode(38, OUTPUT);
  digitalWrite(38, LOW);

}


// Runs repeatedly
// Monitors uart for commands and then makes function call
void loop() {
  // Do nothing until a '\n' terminated string is received
  if (stringComplete) {
    if (inputString == "boardconfig\n") {
      board_config();
    }

    else if (inputString == "transfersram\n") {
      transfer_sram();
    }

    else if (inputString == "transfersram4b5b\n") {
      transfer_sram_4b5b();
    }

    else if (inputString == "transfersdigdata\n") {
      transfer_dig_data();
    }

    else if (inputString == "digdata\n") {
      output_digital_data();
    }

    else if (inputString == "transmit_chips\n") {
      transmit_chips();
    }

    else if (inputString == "boot3wb\n") {
      bootload_3wb();
    }

    else if (inputString == "boot3wb_debug\n") {
      bootload_3wb_debug();
    }

    else if (inputString == "boot3wb4b5b\n") {
      bootload_3wb_4b5b();
    }
    
    else if (inputString == "optdata\n") {
      optical_data_transfer(p1, p2, p3, p4);
    }

    else if (inputString == "bootopt\n") {
      bootload_opt(p1, p2, p3, p4);
    }
    else if (inputString == "bootopt_debug\n") {
      bootload_opt_debug(p1, p2, p3, p4);  
    }

    else if (inputString == "bootopt4b5b\n") {
      bootload_opt_4b5b(p1, p2, p3, p4);
    }

    else if (inputString == "bootopt4b5bnorst\n") {
      bootload_opt_4b5b_no_reset(p1, p2, p3, p4);
    }

    else if (inputString == "configopt\n") {
      optical_config();
    }

    else if (inputString == "ascwrite\n") {
      asc_write();
    }

    else if (inputString == "ascload\n") {
      asc_load();
    }

    else if (inputString == "ascread\n") {
      asc_read();
    }

    else if (inputString == "dscread\n") {
      dsc_read();
    }
    
//    else if (inputString == "i2cgpioconfig\n") {
//      i2cgpio_config();
//    }

    else if (inputString == "interrupt\n") {
      toggle_interrupt();
    }

    else if (inputString == "stepclk\n") {
      step_clock();
    }

    else if (inputString == "clockon\n") {
      clock_on();
    }

    else if (inputString == "clockoff\n") {
      clock_off();
    }

    else if (inputString == "togglehardreset\n") {
      togglehardreset();
    }

    // Reset to listen for a new '\n' terminated string over serial
    inputString = "";
    stringComplete = false;
  }
}


void board_config() {

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }

  int ldo_trim = inputString.toInt();

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }
  int gpio_dir = inputString.toInt();

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }
  int ldo_enables = inputString.toInt();

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  //Serial.println("Received " + String(ldo_trim) + " " + String(gpio_dir) + " " + String(ldo_enables));

  // Program adjustable ldos
  if(ldo_trim & 0x128) {
    pinMode(vdda_50, OUTPUT);
    digitalWrite(vdda_50, LOW);
  }
  else{
    pinMode(vdda_50, INPUT);
  }   
   
  if(ldo_trim & 0x64) {
    pinMode(vdda_100, OUTPUT);
    digitalWrite(vdda_100, LOW);
  }
  else{
    pinMode(vdda_100, INPUT);
  }
  
  if(ldo_trim & 0x32) {
    pinMode(vdda_200, OUTPUT);
    digitalWrite(vdda_200, LOW);
  }
  else{
    pinMode(vdda_200, INPUT);
  }
  
  if(ldo_trim & 0x16) {
    pinMode(vdda_400, OUTPUT);
    digitalWrite(vdda_400, LOW);
  }  
  else{
    pinMode(vdda_400, INPUT);
  }

  // VDDD
  if(ldo_trim & 0x8) {
    pinMode(vddd_50, OUTPUT);
    digitalWrite(vddd_50, LOW);
  }
  else{
    pinMode(vddd_50, INPUT);
  }   
   
  if(ldo_trim & 0x4) {
    pinMode(vddd_100, OUTPUT);
    digitalWrite(vddd_100, LOW);
  }
  else{
    pinMode(vddd_100, INPUT);
  }
  
  if(ldo_trim & 0x2) {
    pinMode(vddd_200, OUTPUT);
    digitalWrite(vddd_200, LOW);
  }
  else{
    pinMode(vddd_200, INPUT);
  }
  
  if(ldo_trim & 0x1) {
    pinMode(vddd_400, OUTPUT);
    digitalWrite(vddd_400, LOW);
  }  
  else{
    pinMode(vddd_400, INPUT);
  }

  // Turn on 3.3V LDO (so the shifter register works)
  digitalWrite(ldo33_en, HIGH);
  delay(20);

  // Program GPIO Shift register
  for (int ii = 0; ii < 17; ii++) {
    if (gpio_dir & (0x01 << ii)) {
      digitalWrite(gpio_scan_data, HIGH);
    }
    else {
      digitalWrite(gpio_scan_data, LOW);
    }

    delay(1);
    digitalWrite(gpio_clk, HIGH);
    delay(1);
    digitalWrite(gpio_clk, LOW);
  }

  // setup I2C
  //Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 100000); // Wire bus, SCL pin 19, SDA pin 18, ext pullup, 400kHz
  //i2cgpio_reset();

  // Program LDO enables
  digitalWrite(gpio_en, ldo_enables & 0x08);
  delay(1);
  digitalWrite(vdda_en, ldo_enables & 0x02);
  delay(1);
  digitalWrite(vddd_en, ldo_enables & 0x01);
  delay(1);
  digitalWrite(vbat_en, ldo_enables & 0x10);
  delay(1);
  digitalWrite(debug_en, ldo_enables & 0x04);

}

void transfer_sram() {
  Serial.println("Executing SRAM Transfer");
  int doneflag = 0;
  iindex = 0;

  // Loop until entire 64kB received over serial
  while (!doneflag) {

    // Retrieve SRAM contents over serial
    if (Serial.available()) {
      ram[iindex] = (byte)Serial.read();
      iindex++;
    }

    if (iindex == 65536)  {
      doneflag = 1;
      Serial.println("SRAM Transfer Complete");
    }
  }
}

void transfer_sram_4b5b() {
  Serial.println("Executing SRAM Transfer");
  int doneflag = 0;
  iindex = 0;

  // Loop until entire 64kB received over serial
  while (!doneflag) {

    // Retrieve SRAM contents over serial
    if (Serial.available()) {
      ram[iindex] = (byte)Serial.read();
      iindex++;
    }

    if (iindex == 81920)  {
      doneflag = 1;
      Serial.println("SRAM Transfer Complete");
    }
  }
}

void transfer_dig_data() {
  //Serial.println("Executing Digital Data Transfer");
  int doneflag = 0;
  iindex = 0;

  // Loop until entire data set received over serial
  while (!doneflag) {

    // Retrieve SRAM contents over serial
    if (Serial.available()) {
      ram[iindex] = (byte)Serial.read();
      iindex++;
    }

    if (iindex == dig_data_bytes)  {
      doneflag = 1;
      //Serial.println("Digital Data Transfer Complete");
    }
  }
}

// Call this function to configure pulse widths for optical TX
void optical_config() {

  //Serial.println("Calling optical config");

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }

  p1 = inputString.toInt();
  //Serial.println("Received " + String(p1));

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }
  p2 = inputString.toInt();
  //Serial.println("Received " + String(p2));

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }
  p3 = inputString.toInt();
  //Serial.println("Received " + String(p3));

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;

  while (stringComplete == false) {
    serialEvent();
  }
  p4 = inputString.toInt();
  //Serial.println("Received "  + String(p4));

  // Reset to listen for a new '\n' terminated string over serial
  inputString = "";
  stringComplete = false;
}


void bootload_opt_4b5b(int p1, int p2, int p3, int p4) {

  #define NOP1 "nop\n\t"

  int bitmask = 0x01;
  noInterrupts();
  int xx;

  int start_symbol[4] = {169,176,167,50};
  int start_symbol_without_hard_reset[4] = {184,84,89,40};
  int preamble = 85;

  // Send Preamble of 101010 to allow optical RX to settle
  for (int i = 0; i < PREAMBLE_LENGTH; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send start symbol to intiate hard reset
  for (int i = 0; i < 4; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (start_symbol[i] & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send REBOOT_BYTES worth of preamble to give cortex time to do hard reset
  // The REBOOT_BYTES variable must be the same value as in the verilog
  for (int i = 0; i < REBOOT_BYTES; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send program data encoded in 4b5b format
  for (int i = 0; i < 81920; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (ram[i] & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Need to send 5 extra bits to clock last value through
   for (int j = 0; j < 6; j++) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
    }
  
  interrupts();
}




void bootload_opt_4b5b_no_reset(int p1, int p2, int p3, int p4) {

  #define NOP1 "nop\n\t"

  int bitmask = 0x01;
  noInterrupts();
  int xx;

  int start_symbol[4] = {169,176,167,50};
  int start_symbol_without_hard_reset[4] = {184,84,89,40};
  int preamble = 85;

  // Send Preamble of 101010 to allow optical RX to settle
  for (int i = 0; i < PREAMBLE_LENGTH; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send start symbol to intiate boot without hard reset
  for (int i = 0; i < 4; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (start_symbol_without_hard_reset[i] & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }
  
  // Send program data encoded in 4b5b format
  for (int i = 0; i < 81920; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (ram[i] & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Need to send 5 extra bits to clock last value through
   for (int j = 0; j < 6; j++) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
    }
  
  interrupts();
}


void optical_data_transfer(int p1, int p2, int p3, int p4) {

  #define NOP1 "nop\n\t"

  int bitmask = 0x01;
  noInterrupts();
  int xx;

  int sfd[4] = {221,176,231,47};
  int preamble = 85;

  // Send Preamble of 101010 to allow optical RX to settle
  for (int i = 0; i < PREAMBLE_LENGTH; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send start symbol to throw interrupt for aligning data
  for (int i = 0; i < 4; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (sfd[i] & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

      // The very first data bit gets lost so send it twice to re-align
      if (ram[0] & (bitmask)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
      


  // Send payload in raw binary (no 4B5B done in hardware)
  for (int i = 0; i < dig_data_bytes; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (ram[i] & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  
  interrupts();
}




































void bootload_opt(int p1, int p2, int p3, int p4) {

  #define NOP1 "nop\n\t"

  int bitmask = 0x01;
  noInterrupts();
  int xx;

  int start_symbol[4] = {169,176,167,50};
  int preamble = 85;


//for (int xxx = 100; xxx < 4000; xxx+=100) {

int xxx = 500;
  

  // Send Preamble of 101010 to allow optical RX to settle
  for (int i = 0; i < PREAMBLE_LENGTH; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send start symbol to intiate hard reset
  for (int i = 0; i < 4; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (start_symbol[i] & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

    sendOnes(p1, p2, 32);

//    for (int j = 0; j < 8; j++) {
//
//      //data = 1, long pulse
//      if (ram[0] & (bitmask << j)) {
//
//        digitalWriteFast(38, HIGH);
//        for (xx = 0; xx < p1; xx++) {
//          __asm__(NOP1);
//        }
//        digitalWriteFast(38, LOW);
//        for (xx = 0; xx < p2; xx++) {
//          __asm__(NOP1);
//        }
//      }
//      else {    // data = 0, short pulse
//
//        digitalWriteFast(38, HIGH);
//        for (xx = 0; xx < p3; xx++) {
//          __asm__(NOP1);
//        }
//        digitalWriteFast(38, LOW);
//        for (xx = 0; xx < p4; xx++) {
//          __asm__(NOP1);
//        }
//      }
//    }

    delayMicroseconds(xxx);

  
  // Send program data
  for (int i = 0; i < 65536; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (ram[i] & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

//  // Send Preamble of 101010 to be sure soft reset occurs
//  for (int i = 0; i < 5000; i++) {      
//    for (int j = 0; j < 8; j++) {
//
//      //data = 1, long pulse
//      if (preamble & (bitmask << j)) {
//
//        digitalWriteFast(38, HIGH);
//        for (xx = 0; xx < p1; xx++) {
//          __asm__(NOP1);
//        }
//        digitalWriteFast(38, LOW);
//        for (xx = 0; xx < p2; xx++) {
//          __asm__(NOP1);
//        }
//      }
//      else {    // data = 0, short pulse
//
//        digitalWriteFast(38, HIGH);
//        for (xx = 0; xx < p3; xx++) {
//          __asm__(NOP1);
//        }
//        digitalWriteFast(38, LOW);
//        for (xx = 0; xx < p4; xx++) {
//          __asm__(NOP1);
//        }
//      }
//    }
//  }

//delay(1000);
//}


  
  interrupts();
  Serial.println("Optical boot complete.");
}


void bootload_opt_debug(int p1, int p2, int p3, int p4) {

  #define NOP1 "nop\n\t"

  int bitmask = 0x01;
  noInterrupts();
  int xx;

  int start_symbol[4] = {169,176,167,50};
  int preamble = 85;


//for (int xxx = 100; xxx < 4000; xxx+=100) {

int xxx = 500;
  

  // Send Preamble of 101010 to allow optical RX to settle
  for (int i = 0; i < PREAMBLE_LENGTH; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (preamble & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

  // Send start symbol to intiate hard reset
  for (int i = 0; i < 4; i++) {      
    for (int j = 0; j < 8; j++) {

      //data = 1, long pulse
      if (start_symbol[i] & (bitmask << j)) {

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p1; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p2; xx++) {
          __asm__(NOP1);
        }
      }
      else {    // data = 0, short pulse

        digitalWriteFast(38, HIGH);
        for (xx = 0; xx < p3; xx++) {
          __asm__(NOP1);
        }
        digitalWriteFast(38, LOW);
        for (xx = 0; xx < p4; xx++) {
          __asm__(NOP1);
        }
      }
    }
  }

//    for (int j = 0; j < 8; j++) {
//
//      //data = 1, long pulse
//      if (ram[0] & (bitmask << j)) {
//
//        digitalWriteFast(38, HIGH);
//        for (xx = 0; xx < p1; xx++) {
//          __asm__(NOP1);
//        }
//        digitalWriteFast(38, LOW);
//        for (xx = 0; xx < p2; xx++) {
//          __asm__(NOP1);
//        }
//      }
//      else {    // data = 0, short pulse
//
//        digitalWriteFast(38, HIGH);
//        for (xx = 0; xx < p3; xx++) {
//          __asm__(NOP1);
//        }
//        digitalWriteFast(38, LOW);
//        for (xx = 0; xx < p4; xx++) {
//          __asm__(NOP1);
//        }
//      }
//    }
    sendOnes(p1, p2, 8);
    delayMicroseconds(xxx);
    sendOnes(p1, p2, 24);

    int n = 64*1024*8-32;
    int num_zeros = 2;
    int idx = 6;
    
    sendOnes(p1, p2, idx);
    sendZeros(p3, p4, num_zeros);
    sendOnes(p1, p2, n-idx-num_zeros);
//    sendZeros(p3, p4, 2);
//    sendOnes(p1, p2, 1);
//    sendZeros(p3, p4, 1);
//    sendOnes(p1, p2, 2);
//    sendZeros(p3, p4, 2);
//    sendOnes(p1, p2, 3);
//    sendZeros(p3, p4, 3);
//    sendOnes(p1, p2, 4);
//    sendZeros(p3, p4, 4);
//    sendOnes(p1, p2, 5);
//    sendZeros(p3, p4, 5);


    // Send Preamble of 101010 to be sure soft reset occurs
//  for (int i = 0; i < 5000; i++) {      
//    for (int j = 0; j < 8; j++) {
//
//      //data = 1, long pulse
//      if (preamble & (bitmask << j)) {
//
//        digitalWriteFast(38, HIGH);
//        for (xx = 0; xx < p1; xx++) {
//          __asm__(NOP1);
//        }
//        digitalWriteFast(38, LOW);
//        for (xx = 0; xx < p2; xx++) {
//          __asm__(NOP1);
//        }
//      }
//      else {    // data = 0, short pulse
//
//        digitalWriteFast(38, HIGH);
//        for (xx = 0; xx < p3; xx++) {
//          __asm__(NOP1);
//        }
//        digitalWriteFast(38, LOW);
//        for (xx = 0; xx < p4; xx++) {
//          __asm__(NOP1);
//        }
//      }
//    }
//  }
  interrupts();
  Serial.println("Optical boot complete.");
}



void output_digital_data() {

#define NOP7 "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
#define NOP10 "nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"
  
  int bitmask = 0x01;
  noInterrupts();
  int xx;
  
  for (int i = 0; i < dig_data_bytes; i++) {      
    for (int j = 0; j < 8; j++) {

      if (ram[i] & (bitmask << j)) {
        digitalWriteFast(DATA_OUT, HIGH);
      }
      else {  
        digitalWriteFast(DATA_OUT, LOW);
      }

      // Toggle clock
      delayMicroseconds(10);
      // __asm__(NOP7);
      digitalWriteFast(CLK_OUT, HIGH);
      delayMicroseconds(10);
      // __asm__(NOP7);
      digitalWriteFast(CLK_OUT, LOW);
      delayMicroseconds(10);
    }
  }
  interrupts();
  Serial.println("Digital data output complete.");
}


// Receives 1200 analog scan chain bits over serial
// Then bitbangs them out to ASC
void asc_write() {
  Serial.println("Executing ASC Write");
  int count = 0;
  char scanbits[1200];

  // Set scan chain source as external
  digitalWrite(asc_source_select, HIGH);

  // Loop until all 1200 scan chain bits received over serial
  while (count != 1200) {

    // Read one bit at a time over serial
    if (Serial.available()) {
      scanbits[count] = Serial.read();
      count++;
    }
  }

  // Once all bits are received, bitbang to ASC input
  for (int x = 0; x < 1200; x++) {
    if (scanbits[x] == '1') {
      digitalWrite(aSCANIN, HIGH);
    }
    else if (scanbits[x] == '0') {
      digitalWrite(aSCANIN, LOW);
    }
    else {
      // There was an error reading in the bits over uart
      Serial.println("Error in ASC Write");
    }
    // Pulse PHI and PHIB
    atick();
  }
  Serial.println("ASC Write Complete");
}

// Latches the loaded data to the outputs of the scan chain
// Must call asc_write() before asc_load()
void asc_load() {
  Serial.println("Executing ASC Load");
  digitalWrite(aLOAD, HIGH);
  digitalWrite(aLOAD, LOW);

  // Set scan chain source back to internal
  delayMicroseconds(1);
  digitalWrite(asc_source_select, LOW);
}


// Transmits 64kB binary over wired 3wb
void bootload_3wb_4b5b()  {
  Serial.println("Executing 3wb Bootload");

  int start_symbol[4] = {169,176,167,50};
  int preamble = 85;

  // Send start symbol
  for (int i = 0; i < 4; i++) {
    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (start_symbol[i] >> j) & 1);

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }

  // Send dummy bytes to wait for reset to finish
  for (int i = 0; i < REBOOT_BYTES; i++) {
    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (preamble >> j) & 1);

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }

  // Need to send at least 64*1024 bytes to get cortex to reset
  for (int i = 1; i < 81921; i++) {
    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (ram[i - 1] >> j) & 1);
      delayMicroseconds(1);

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }

  // Send 5 extra bits to cycle last 4b5b value through
    for (int j = 0; j < 6; j++) {
      digitalWrite(dataPin, 0);

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }


}


// Transmits 64kB binary over wired 3wb
void bootload_3wb()  {
  Serial.println("Executing 3wb Bootload");

  // Execute hard reset
  digitalWrite(hReset, LOW);
  delayMicroseconds(500);
  digitalWrite(hReset, HIGH);
  delay(500);
  //delayMicroseconds(500);

  // Need to send at least 64*1024 bytes to get cortex to reset
  for (int i = 1; i < 65537; i++) {

    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (ram[i - 1] >> j) & 1);
      delayMicroseconds(1);

      // Every 32 bits need to strobe the enable high for one cycle
      if ((i % 4 == 0) && (j == 7)) {
        digitalWrite(enablePin, 1);
      }
      else {
        digitalWrite(enablePin, 0);
      }

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }

//  // Execute soft reset
//  digitalWrite(sReset, LOW);
//  delay(20);
//  digitalWrite(sReset, HIGH);
//  delay(20);
}



// Transmits 64kB binary over wired 3wb
void bootload_3wb_debug()  {
  Serial.println("Executing 3wb Bootload");

  // Execute hard reset
//  digitalWrite(hReset, LOW);
 // delayMicroseconds(500);
 // digitalWrite(hReset, HIGH);
 // delay(500);
  //delayMicroseconds(500);

ram[0] = 0;
ram[1]= 255;
ram[2] = 0;
ram[3] = 255;

  // Need to send at least 64*1024 bytes to get cortex to reset
  for (int i = 1; i < 5; i++) {

    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWrite(dataPin, (ram[i - 1] >> j) & 1);
      delayMicroseconds(1);

      // Every 32 bits need to strobe the enable high for one cycle
      if ((i % 4 == 0) && (j == 7)) {
        digitalWrite(enablePin, 1);
      }
      else {
        digitalWrite(enablePin, 0);
      }

      // Toggle the clock
      delayMicroseconds(1);
      digitalWrite(clkPin, 1);
      delayMicroseconds(1);
      digitalWrite(clkPin, 0);
      delayMicroseconds(1);
    }
  }

}


// Transmits chips at near 2Mcps
void transmit_chips()  {

  #define NOP1 "nop\n\t"

  int lpval = 5;
  int xx;
  
  Serial.println("Executing Raw Chip Output");

  for (int i = 1; i < dig_data_bytes; i++) {

    // Loop through one byte at a time
    for (int j = 0; j < 8; j++) {
      digitalWriteFast(DATA_OUT, (ram[i - 1] >> j) & 1);

      // Toggle the clock
              for (xx = 0; xx < lpval; xx++) {
          __asm__(NOP1);
        }
      digitalWriteFast(CLK_OUT, 1);
              for (xx = 0; xx < lpval; xx++) {
          __asm__(NOP1);
        }
      digitalWriteFast(CLK_OUT, 0);
              for (xx = 0; xx < lpval; xx++) {
          __asm__(NOP1);
        }
    }
  }
}


//10 KHz clock, 40 percent duty cycle
void dtick() {
  digitalWrite(dPHI, HIGH);
  delayMicroseconds(10);

  //Read
  digitalWrite(dPHI, LOW);
  delayMicroseconds(40);

  //Shift
  digitalWrite(dPHIb, HIGH);
  delayMicroseconds(40);
  digitalWrite(dPHIb, LOW);
  delayMicroseconds(10);


}

//Note that Serial.println has a 697 char (699 out) max.
//2157 = 600 + 600 + 600 + 357
void dsc_read() {
  //Serial.println("Executing DSC Read");
  String st = "";
  int val = 0;

  // Setup pins for digital scan chain
  pinMode(dPHI, OUTPUT);
  pinMode(dPHIb, OUTPUT);
  pinMode(dSCANi0o1, OUTPUT);
  digitalWrite(dPHI, LOW);
  digitalWrite(dPHIb, LOW);
  digitalWrite(dSCANi0o1, LOW);   //stay low until ready
  delay(1);

  digitalWrite(dSCANi0o1, HIGH);
  dtick();
  digitalWrite(dSCANi0o1, LOW);
  //First bit should be available
  val = digitalRead(dSCANOUT);
  if (val)
    st = String(st + "1");
  else
    st = String(st + "0");

  //2-600
  for (int i = 1; i < 600 ; i = i + 1) {
    dtick();
    val = digitalRead(dSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  //601-1200
  for (int i = 0; i < 600 ; i = i + 1) {
    dtick();
    val = digitalRead(dSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  //1201-1800
  for (int i = 0; i < 600 ; i = i + 1) {
    dtick();
    val = digitalRead(dSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  //1801-2157
  for (int i = 0; i < 357 ; i = i + 1) {
    dtick();
    val = digitalRead(dSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  Serial.println(); //Terminator

  // Set pins back to hi-Z
  pinMode(dPHI, INPUT);
  pinMode(dPHIb, INPUT);
  pinMode(dSCANi0o1, INPUT);

  return;
}

//10 KHz clock, 40 percent duty cycle
void atick() {
  digitalWrite(aPHI, LOW);
  delayMicroseconds(10);

  //Shift
  digitalWrite(aPHIb, HIGH);
  delayMicroseconds(40);
  digitalWrite(aPHIb, LOW);
  delayMicroseconds(10);

  //Read
  digitalWrite(aPHI, HIGH);
  delayMicroseconds(40);
}

//Note that Serial.println has a 697 char (699 out) max.
//1200 = 600 + 600
void asc_read() {
  //Serial.println("Executing ASC Read");
  String st = "";
  int val = 0;

  // Set scan chain source as external
  digitalWrite(asc_source_select, HIGH);

  //digitalWrite(aSCANi0o1, HIGH);
  //atick();
  //digitalWrite(aSCANi0o1, LOW);
  //First bit should be available
  val = digitalRead(aSCANOUT);
  if (val)
    st = String(st + "1");
  else
    st = String(st + "0");

  //2-600
  for (int i = 1; i < 600 ; i = i + 1) {
    atick();
    val = digitalRead(aSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  //601-1200
  for (int i = 0; i < 600 ; i = i + 1) {
    atick();
    val = digitalRead(aSCANOUT);
    if (val)
      st = String(st + "1");
    else
      st = String(st + "0");
  }
  Serial.print(st);
  st = "";

  Serial.println(); //Terminator

  // Set scan chain source back to internal
  digitalWrite(asc_source_select, LOW);

  return;
}


//void i2cgpio_config() {
//  Serial.println("Configuring I2C GPIO");
//
//  // Reset to listen for a new '\n' terminated string over serial
//  inputString = "";
//  stringComplete = false;
//
//  while (stringComplete == false) {
//    serialEvent();
//  }
//
//  int gpio_setting0 = inputString.toInt();
//
//  // Reset to listen for a new '\n' terminated string over serial
//  inputString = "";
//  stringComplete = false;
//
//  while (stringComplete == false) {
//    serialEvent();
//  }
//
//  int gpio_setting1 = inputString.toInt();
//
//  // Reset to listen for a new '\n' terminated string over serial
//  inputString = "";
//  stringComplete = false;
//
//  // do stuff with gpio_setting
//  uint8_t target = 0b0100000; // i2c address of PCA9535C with A0=A1=A2=0
//
//  /* Command order
//     0x02 -- select output port 0
//      -- set LDOs to disabled & minimum voltage
//     0x03 -- select output port 1
//      -- set LDOs to disabled & minimum voltage
//     0x06 -- configure port 0
//     0x00 -- configure them all as outputs
//     0x07 -- configure port 1
//     0x00 -- configure them all as outputs
//     0x02 -- select output port 0
//     0x00 -- set them to gpio_setting0
//     0x03 -- select output port 1
//     0x00 -- set them to gpio_setting1
//  */
//
//  char databuf0[2];
//  char databuf1[2];
//  char databuf2[2];
//  char databuf3[2];
//
//  databuf0[0] = 0x02;
//  databuf0[1] = gpio_setting0;
//
//  databuf1[0] = 0x03;
//  databuf1[1] = gpio_setting1;
//
//  databuf2[0] = 0x06; // in case reset hasn't been run, set to output too
//  databuf2[1] = 0x00;
//
//  databuf3[0] = 0x07;
//  databuf3[1] = 0x00;
//
//  Serial.print(int(databuf0[1]));
//  Serial.print(" ");
//  Serial.println(int(databuf1[1]));
//  //Serial.print(" ");
//  //Serial.println("Got 2 bank settings");
//  // Reset to listen for a new '\n' terminated string over serial
//  inputString = "";
//  stringComplete = false;
//
//  Wire.beginTransmission(target);   // Slave address
//  Wire.write(databuf0, 2); // Write string to I2C Tx buffer
//  Wire.endTransmission();           // Transmit to Slave
//
//  Wire.beginTransmission(target);   // Slave address
//  Wire.write(databuf1, 2); // Write string to I2C Tx buffer
//  Wire.endTransmission();           // Transmit to Slave
//
//  Wire.beginTransmission(target);   // Slave address
//  Wire.write(databuf2, 2); // Write string to I2C Tx buffer
//  Wire.endTransmission();           // Transmit to Slave
//
//  Wire.beginTransmission(target);   // Slave address
//  Wire.write(databuf3, 2); // Write string to I2C Tx buffer
//  Wire.endTransmission();           // Transmit to Slave
//}
//
//void i2cgpio_reset() {
//
//  // do stuff with gpio_setting
//  uint8_t target = 0b0100000; // i2c address of PCA9535C with A0=A1=A2=0
//
//  /* Command order
//     0x02 -- select output port 0
//      -- set LDOs to disabled & minimum voltage
//     0x03 -- select output port 1
//      -- set LDOs to disabled & minimum voltage
//     0x06 -- configure port 0
//     0x00 -- configure them all as outputs
//     0x07 -- configure port 1
//     0x00 -- configure them all as outputs
//  */
//
//  char databuf0[4];
//  databuf0[0] = 0x02;
//  databuf0[1] = 0b01111011; // enables to 0, all others high-z to use minimum LDO voltage if enabled
//  databuf0[2] = 0x03;
//  databuf0[3] = 0b11011111; // enables to 0, all others high-z to use minimum LDO voltage if enabled
//
//  char databuf1[4];
//  databuf1[0] = 0x06;
//  databuf1[1] = 0x00;
//  databuf1[2] = 0x07;
//  databuf1[3] = 0x00;
//
//
//  Wire.beginTransmission(target);   // Slave address
//  Wire.write(databuf1, 4); // Write string to I2C Tx buffer
//  Wire.endTransmission();           // Transmit to Slave
//
//
//  Wire.beginTransmission(target);   // Slave address
//  Wire.write(databuf0, 4); // Write string to I2C Tx buffer
//  Wire.endTransmission();           // Transmit to Slave
//}
//

// Toggle interrupt pin - active low
void toggle_interrupt() {
    Serial.println("Toggling Interrupt");
    digitalWrite(interrupt_b, LOW);
    delay(3);
    digitalWrite(interrupt_b, HIGH);
}


// Toggle pin a specified number of times for use as clock when debugging
// Note that the period of this clock is < 5MHz (measured 491.8kHz on scope)
void step_clock() {
    // Read number of clock ticks to output
    inputString = "";
    stringComplete = false;
  
    while (stringComplete == false) {
      serialEvent();
    }
  
    int num_ticks = inputString.toInt();
    //pinMode(optical_out, OUTPUT);

    for (int i = 0; i < num_ticks ; i = i + 1) {
      digitalWriteFast(optical_out, HIGH);
      delayMicroseconds(1);
      digitalWriteFast(optical_out, LOW);
      delayMicroseconds(1);
    }
}

// Output a 5MHz clock on pin 38
void clock_on() {
  analogWriteFrequency(optical_out,5000000);
  //analogWriteFrequency(optical_out,10000);
  analogWrite(optical_out, 128);
}

// Turn off clock on pin 38
void clock_off() {
  pinMode(optical_out, OUTPUT);
  digitalWrite(optical_out, LOW);
}


// Toggle hard reset low then high
void togglehardreset() {
  digitalWrite(hReset, LOW);
  delay(1);
  digitalWrite(hReset, HIGH);

}

// Sends bits depending on the encoding
void sendOnes(int t_hi, int t_lo, int num){
  int xx = 0;
  for(int i=0; i<num; i++){
    digitalWriteFast(38, HIGH);
    for(xx=0; xx<t_hi; xx++){
      __asm__(NOP1);
    }
    digitalWriteFast(38, LOW);
    for(xx=0; xx<t_lo; xx++){
      __asm__(NOP1);
    }
  }
}

// Sends bits depending on the encoding
void sendZeros(int t_hi, int t_lo, int num){
  int xx = 0;
  for(int i=0; i<num; i++){
    digitalWriteFast(38, HIGH);
    for(xx=0; xx<t_hi; xx++){
      __asm__(NOP1);
    }
    digitalWriteFast(38, LOW);
    for(xx=0; xx<t_lo; xx++){
      __asm__(NOP1);
    }
  }
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
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
