import numpy as np
import scipy as sp
from scipy import stats
import serial
import visa

def __program_cortex__(com_port="COM15", file_binary="./code.bin",
		boot_mode='optical', skip_reset=False, insert_CRC=False,
		pad_random_payload=False):
	"""
	Notes:
		INTERNAL USE ONLY! Note that this refrains from closing 
		any open serial connections at the end--example use case 
		can be found in program_cortex().
	Inputs:
		com_port: String. Name of the COM port that the Teensy
			is connected to.
		file_binary: String. Path to the binary file to 
			feed to Teensy to program SCM. This binary file shold be
			compiled using whatever software is meant to end up 
			on the Cortex. This group tends to compile it using Keil
			projects.
		boot_mode: String. 'optical' or '3wb'. The former will assume an
			optical bootload, whereas the latter will use the 3-wire
			bus.
		skip_reset: Boolean. True: Skip hard reset before optical 
			programming. False: Perform hard reset before optical programming.
		insert_CRC: Boolean. True = insert CRC for payload integrity 
			checking. False = do not insert CRC. Note that SCM C code 
			must also be set up for CRC check for this to work.
		pad_random_payload: Boolean. True = pad unused payload space with 
			random data and check it with CRC. False = pad with zeros, do 
			not check integrity of padding. This is useful to check for 
			programming errors over full 64kB payload.
	Outputs:
		Returns the still-open Serial object after programming the cortex.
		Feeds the input from file_binary to the Teensy to program SCM. 
		Does NOT close out the serial connection.
	Raises:
		ValueError if the boot_mode isn't 'optical' or '3wb'.
	
	"""
	# Open COM port to Teensy
	ser = serial.Serial(
		port=com_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS)

	# Open binary file from Keil
	with open(file_binary, 'rb') as f:
		bindata = bytearray(f.read())

	# Need to know how long the binary payload is for computing CRC
	code_length = len(bindata) - 1
	pad_length = 65536 - code_length - 1

	# Optional: pad out payload with random data if desired
	# Otherwise pad out with zeros - uC must receive full 64kB
	if(pad_random_payload):
		for i in range(pad_length):
			bindata.append(random.randint(0,255))
		code_length = len(bindata) - 1 - 8
	else:
		for i in range(pad_length):
			bindata.append(0)

	if insert_CRC:
	    # Insert code length at address 0x0000FFF8 for CRC calculation
	    # Teensy will use this length value for calculating CRC
		bindata[65528] = code_length % 256 
		bindata[65529] = code_length // 256
		bindata[65530] = 0
		bindata[65531] = 0
	
	# Transfer payload to Teensy
	ser.write(b'transfersram\n')
	print(ser.readline())
	# Send the binary data over uart
	ser.write(bindata)

	if insert_CRC:
	    # Have Teensy calculate 32-bit CRC over the code length 
	    # It will store the 32-bit result at address 0x0000FFFC
		ser.write(b'insertcrc\n')

	if boot_mode == 'optical':
	    # Configure parameters for optical TX
		ser.write(b'configopt\n')
		ser.write(b'80\n')	
		ser.write(b'80\n')
		ser.write(b'3\n')
		ser.write(b'80\n')
		
	    # Encode the payload into 4B5B for optical transmission
		ser.write(b'encode4b5b\n')

		if not skip_reset:
	        # Do a hard reset and then optically boot
			ser.write(b'bootopt4b5b\n')
		else:
	        # Skip the hard reset before booting
			ser.write(b'bootopt4b5bnorst\n')

	    # Display confirmation message from Teensy
		print(ser.readline())
	elif boot_mode == '3wb':
	    # Execute 3-wire bus bootloader on Teensy
		ser.write(b'boot3wb\n')

	    # Display confirmation message from Teensy
		print(ser.readline())
	else:
		raise ValueError("Boot mode '{}' invalid.".format(boot_mode))

	ser.write(b'opti_cal\n');
	return ser

def program_cortex(com_port="COM15", file_binary="./code.bin",
		boot_mode='optical', skip_reset=False, insert_CRC=False,
		pad_random_payload=False):
	"""
	Inputs:
		com_port: String. Name of the COM port that the Teensy
			is connected to.
		file_binary: String. Path to the binary file to 
			feed to Teensy to program SCM. This binary file shold be
			compiled using whatever software is meant to end up 
			on the Cortex. This group tends to compile it using Keil
			projects.
		boot_mode: String. 'optical' or '3wb'. The former will assume an
			optical bootload, whereas the latter will use the 3-wire
			bus.
		skip_reset: Boolean. True: Skip hard reset before optical 
			programming. False: Perform hard reset before optical programming.
		insert_CRC: Boolean. True = insert CRC for payload integrity 
			checking. False = do not insert CRC. Note that SCM C code 
			must also be set up for CRC check for this to work.
		pad_random_payload: Boolean. True = pad unused payload space with 
			random data and check it with CRC. False = pad with zeros, do 
			not check integrity of padding. This is useful to check for 
			programming errors over full 64kB payload.
	Outputs:
		No return value. Feeds the input from file_binary
		to the Teensy to program SCM. Closes out the serial at the
		end.
	Raises:
		ValueError if the boot_mode isn't 'optical' or '3wb'.
	"""
	ser = __program_cortex__(com_port, file_binary,
		boot_mode, skip_reset, insert_CRC,
		pad_random_payload)

	# Due diligence
	ser.close()

def test_adc_spot(teensy_port="COM15", uart_port="COM16", iterations=1,
		file_binary="./code.bin",
		boot_mode='optical', skip_reset=False, insert_CRC=False,
		pad_random_payload=False):
	"""
	Inputs:
		teensy_port: String. Name of the COM port that the Teensy
			is connected to.
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		iterations: Integer. Number of times to take readings.
		file_binary: String. Path to the binary file to 
			feed to Teensy to program SCM. This binary file shold be
			compiled using whatever software is meant to end up 
			on the Cortex. This group tends to compile it using Keil
			projects.
		boot_mode: String. 'optical' or '3wb'. The former will assume an
			optical bootload, whereas the latter will use the 3-wire
			bus.
		skip_reset: Boolean. True: Skip hard reset before optical 
			programming. False: Perform hard reset before optical programming.
		insert_CRC: Boolean. True = insert CRC for payload integrity 
			checking. False = do not insert CRC. Note that SCM C code 
			must also be set up for CRC check for this to work.
		pad_random_payload: Boolean. True = pad unused payload space with 
			random data and check it with CRC. False = pad with zeros, do 
			not check integrity of padding. This is useful to check for 
			programming errors over full 64kB payload.
	Outputs:
		Feeds the input from file_binary to the Teensy to program SCM
		and boot the cortex. Returns an ordered collection of ADC 
		output codes where the ith element corresponds to the ith reading.
	Raises:
		ValueError if the boot_mode isn't 'optical' or '3wb'.
	"""
	adc_outs = []

	uart_ser = serial.Serial(
		port=uart_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=5)

	for i in range(iterations):
		teensy_ser = __program_cortex__(teensy_port, file_binary,
			boot_mode, skip_reset, insert_CRC,
			pad_random_payload)
		adc_out = uart_ser.readline()
		adc_outs.append(adc_out)
		teensy_ser.close()
	
	uart_ser.close()

	return adc_outs

def test_adc_psu(
		vin_vec, com_port="COM10",
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
	"""
	Inputs:
		vin_vec: 1D collection of floats. Input voltages in volts 
			to feed to the ADC.
		com_port: String. The name of the COM port (or whatever port)
			used to communicate with the Teensy/microcontroller.
		psu_name: String. Name to use in the connection for the 
			waveform generator.
		iterations: Integer. Number of times to take a reading for
			a single input voltage.
	Outputs:
		Returns a dictionary adc_outs where adc_outs[vin][i] will give the 
		ADC code associated with the i'th iteration when the input
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
		bytesize=serial.EIGHTBITS,
		timeout=2)

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

	# Sweeping vin and getting the ADC output code	
	adc_outs = dict()
	for vin in vin_vec:
		adc_outs[vin] = []
		psu.write("SOURCE2:VOLTAGE:OFFSET {}".format(vin))
		for i in range(iterations):
			ser.write('adc\n')
			adc_out = ser.read()
			adc_outs[vin].append(adc_out)
			print("Vin={}V/Iteration {} -- {}".format(vin, i, adc_outs[vin][i]))
	
	# Due diligence for closing things out
	ser.close()
	psu.close()
	
	return adc_outs

def test_adc_dnl(vin_min, vin_max, num_bits, 
		com_port="COM10",
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
	"""
	Inputs:
		vin_min/max: Float. The min and max voltages to feed to the
			ADC.
		num_bits: Integer. The bit resolution of the ADC.
		com_port: String. The name of the COM port (or whatever port)
			used to communicate with the Teensy/microcontroller.
		psu_name: String. Name to use in the connection for the 
			waveform generator.
		iterations: Integer. Number of times to take a reading for
			a single input voltage.
	Outputs:
		Returns a collection of DNLs for each nominal LSB. The length
		should be 2**(num_bits)-1
	"""
	# Constructing the vector of input voltages to give the ADC
	vlsb_ideal = (vin_max-vin_min)/2**num_bits
	vin_vec = np.arange(vin_min, vin_max, vlsb_ideal/10)

	# Running the test
	adc_outs = test_adc_psu(vin_vec, com_port, psu_name, iterations)
	
	# Averaging over all the iterations to get an average of what the ADC
	# returns.
	adc_outs_avg = {vin:round(np.average(codes)) for (vin,codes) \
					in adc_outs.items()}

	vin_prev = vin_min
	code_prev = adc_outs_avg[vin_prev]
	DNLs = []

	# Calculate the DNL for every step
	for vin,code in adc_outs_avg.items():
		if code > code_prev:
			DNL_val = (vin-vin_prev)/vlsb_ideal - 1
			DNLs.append(DNL_val)
			# If a code is skipped, make the one it skipped functionally
			# zero width.
			if code > code_prev + 1:
				DNLs.append(0)
			code_prev = code
	return DNLs

def test_adc_inl_straightline(vin_min, vin_max, num_bits, 
		com_port="COM10",
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
	"""
	Inputs:
		vin_min/max: Float. The min and max voltages to feed to the
			ADC.
		num_bits: Integer. The bit resolution of the ADC.
		com_port: String. The name of the COM port (or whatever port)
			used to communicate with the Teensy/microcontroller.
		psu_name: String. Name to use in the connection for the 
			waveform generator.
		iterations: Integer. Number of times to take a reading for
			a single input voltage.
	Outputs:
		Returns the slope, intercept of the linear regression
		of the ADC code vs. vin. 
	"""
	# Constructing the vector of input voltages to give the ADC
	vlsb_ideal = (vin_max-vin_min)/2**num_bits
	vin_vec = np.arange(vin_min, vin_max, vlsb_ideal/4)

	# Running the test
	adc_outs = test_adc_psu(vin_vec, com_port, psu_name, iterations)
	
	# Averaging over all the iterations to get an average of what the ADC
	# returns.
	adc_outs_avg = {vin:round(np.average(codes)) for (vin,codes) \
					in adc_outs.items()}

	vin_prev = vin_min
	code_prev = adc_outs_avg[vin_prev]

	x = adc_outs_avg.keys()
	y = [adc_outs_avg[k] for k in x]
	slope, intercept, r_value, p_value, std_error = stats.linregress(x, y)

	return slope, intercept

def test_adc_inl_endpoint(vin_min, vin_max, num_bits, 
		com_port="COM10",
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
	"""
	Inputs:
		vin_min/max: Float. The min and max voltages to feed to the
			ADC.
		num_bits: Integer. The bit resolution of the ADC.
		com_port: String. The name of the COM port (or whatever port)
			used to communicate with the Teensy/microcontroller.
		psu_name: String. Name to use in the connection for the 
			waveform generator.
		iterations: Integer. Number of times to take a reading for
			a single input voltage.
	Outputs:
		Returns a collection of endpoint INLs taken from vin_min 
		up to vin_max.
	"""
	# Constructing the vector of input voltages to give the ADC
	vlsb_ideal = (vin_max-vin_min)/2**num_bits
	vin_vec = np.arange(vin_min, vin_max, vlsb_ideal/4)

	# Running the test
	adc_outs = test_adc_psu(vin_vec, com_port, psu_name, iterations)
	
	# Averaging over all the iterations to get an average of what the ADC
	# returns.
	adc_outs_avg = {vin:round(np.average(codes)) for (vin,codes) \
					in adc_outs.items()}

	code_low = adc_outs_avg[vin_min]
	INLs = []

	# Calculate the endpoint INL for every step
	for vin,code in adc_outs_avg.items():
		if code == code_low:
			continue
		code_diff = code - code_low
		INL_val = (vin-vin_min)/vlsb_ideal - code_diff
		INLs.append(INLs)

	return INLs

if __name__ == "__main__":
	### Testing programming the cortex ###
	if False:
		program_cortex_specs = dict(com_port="COM15",
									file_binary="../code.bin",
									boot_mode="optical",
									skip_reset=False,
									insert_CRC=False,
									pad_random_payload=False)
		program_cortex(**program_cortex_specs)

	if True:
		test_adc_spot_specs = dict(
			teensy_port="COM15",
			uart_port="COM16",
			iterations=1,
			file_binary="../code.bin",
			boot_mode='optical',
			skip_reset=False,
			insert_CRC=False,
			pad_random_payload=False)
		adc_out = test_adc_spot(**test_adc_spot_specs)
		print(adc_out)