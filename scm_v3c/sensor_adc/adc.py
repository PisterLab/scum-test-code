import numpy as np
import scipy as sp
from scipy import stats
import serial
import visa

def program_cortex(com_port="COM15", file_binary="./code.bin"):
	"""
	Inputs:
		com_port: String. Name of the COM port that the Teensy
			is connected to.
		file_binary: String. Path to the binary file to program
			SCM via the Teensy.
	Outputs:
		No return value. Feeds the input from file_binary
		to the Teensy to run via optical bootload.
	"""
	ser = serial.Serial(
		port=com_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS)

	ser.write()
	ser.close()

def test_adc_internal(com_port="COM10", iterations=1):
	"""
	Inputs:
		com_port: String. Name of the COM port that the Teensy
			is connected to.
		iterations: Integer. Number of times to take readings.
	Outputs:
		Returns an ordered collection of ADC output codes
		where the ith element corresponds to the ith reading.
	"""
	adc_outs = []
	for i in range(iterations):
		ser.write(b'\n')
		adc_out = ser.readline()
		adc_outs.append(adc_out)
	
	ser.close()
	
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

	# Sweeping vin and getting the ADC output code	
	adc_outs = dict()
	for vin in vin_vec:
		adc_outs[vin] = []
		psu.write("SOURCE2:VOLTAGE:OFFSET {}".format(vin))
		for i in range(iterations):
			ser.write('adc\n')
			adc_out = ser.readline()
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
	if True:
		print("blep")
		ser = serial.Serial(
			port="COM15",
			baudrate=19200,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS)
			
		ser.write(b'adc\n')
		print(ser.readline())
		ser.close()
		# vbatDiv4_outs = test_adc_internal(com_port="COM15", iterations=1)
		# print(vbatDiv4_outs)