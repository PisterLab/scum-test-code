def test_adc_psu(
		vin_vec, com_port="COM10",
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
	"""
	Inputs:
		vin_vec: 1D collection of floats. 
		com_port: String. The name of the COM port (or whatever port)
			used to communicate with the Teensy/microcontroller.
		psu_name: String. Name to use in the connection for the 
			waveform generator.
		iterations: Integer. Number of times to take a reading for
			a single input voltage.
	Outputs:
		Returns a dictionary vouts where vouts[vin][i] will give the 
		vout value associated with the i'th iteration when the input
		voltage is 'vin'.

		Uses a serial as well as an arbitrary waveform generator to
		sweep the input voltage to the ADC. Bypasses the PGA. Does 
		NOT program scan.
	"""
	# Opening up serial connection to Teensy
	ser = serial.Serial(
		port=com_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS)

	# Connecting to the arbitrary waveform generator
	rm = visa.ResourceManager()
	psu = rm.open_resource(psu_name)

	# Sanity checking that it's the correct device
	psu.query("*IDN?")
	
	# Managing settings for the waveform generator appropriately
	psu.write("OUTPUT2:LOAD INF")
	psu.write("SOURCE2:FUNCTION DC")

	# Setting the waveform generator to 0 initially to 
	# avoid breaking things
	psu.write("SOURCE2:VOLTAGE:OFFSET 0")
	psu.write("OUTPUT2 ON")

	# Sweeping vin and getting the output	
	vouts = dict()
	for vin in vin_vec:
		psu.write("SOURCE2:VOLTAGE:OFFSET {}".format(vin))
		for i in range(iterations):
			vouts[vin][i] = ser.readline()
			print("Vin={}V/Iteration {} -- {}".format(vin, i, vouts[vin][i]))
	
	# Due diligence for closing things out
	ser.close()
	psu.close()
	
	return vouts