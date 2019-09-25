// Load this code onto a Teensy 3.6 to generate fake lighthouse data that can be used for software development on SCM.

// Connect teensy pin 24 to GPIO<3> input on SCM and disable the SCM GPIO output direction
// Also connect a ground

const int out = 24;

// Angles to fake -- in radians
const int fake_azimuth = 1; 
const int fake_elevation = 0.5; 

// All values in us
const int az_width = 63; //83
const int el_width = 73; //94
const int sweep_width = 3;
const int sync_to_sync_time = 8333;

// Convert desired fake angles to time delay in us
const int az_delay = ((1000000*fake_azimuth) / 377); 
const int el_delay = ((1000000*fake_elevation) / 377); 


// Runs once at power-on
void setup() {

  pinMode(out, OUTPUT);
  digitalWriteFast(out, LOW);

}


// Runs repeatedly
// Monitors uart for commands and then makes function call
void loop() {
   
  // Azimuth sync pulse
  digitalWriteFast(out, HIGH);
  delayMicroseconds(az_width);
  digitalWriteFast(out, LOW);

  delayMicroseconds(az_delay);

  // Azimuth sweep
  digitalWriteFast(out, HIGH);
  delayMicroseconds(sweep_width);
  digitalWriteFast(out, LOW);

  delayMicroseconds(sync_to_sync_time - az_width - az_delay - sweep_width);

  // Elevation sync pulse
  digitalWriteFast(out, HIGH);
  delayMicroseconds(el_width);
  digitalWriteFast(out, LOW);
  
  delayMicroseconds(el_delay);

  // Azimuth sweep
  digitalWriteFast(out, HIGH);
  delayMicroseconds(sweep_width);
  digitalWriteFast(out, LOW);

  delayMicroseconds(sync_to_sync_time - el_width - el_delay - sweep_width);
   
}
