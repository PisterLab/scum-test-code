import numpy as np
import scipy as sp
from scipy import stats
import matplotlib.pyplot as plt
import serial
import visa
import time
import random
from data_handling import *

def program_cortex(teensy_port="COM15", uart_port="COM18", file_binary="./code.bin",
		boot_mode='optical', skip_reset=False, insert_CRC=False,
		pad_random_payload=False):
	"""
	Inputs:
		teensy_port: String. Name of the COM port that the Teensy
			is connected to.
		uart_port: String. Name of the COM port that the UART 
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
		No return value. Feeds the input from file_binary to the Teensy to program SCM
		and programs SCM. 
	Raises:
		ValueError if the boot_mode isn't 'optical' or '3wb'.
	Notes:
		When setting optical parameters, all values can be toggled to improve
		success when programming. In particular, too small a third value can
		cause the optical programmer to lose/eat the short pulses.
	"""
	# Open COM port to Teensy
	teensy_ser = serial.Serial(
		port=teensy_port,
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
	teensy_ser.write(b'transfersram\n')
	print(teensy_ser.readline())
	# Send the binary data over uart
	teensy_ser.write(bindata)

	if insert_CRC:
	    # Have Teensy calculate 32-bit CRC over the code length 
	    # It will store the 32-bit result at address 0x0000FFFC
		teensy_ser.write(b'insertcrc\n')

	if boot_mode == 'optical':
	    # Configure parameters for optical TX
		teensy_ser.write(b'configopt\n')
		teensy_ser.write(b'80\n')
		teensy_ser.write(b'80\n')
		teensy_ser.write(b'8\n')
		teensy_ser.write(b'80\n')
		
	    # Encode the payload into 4B5B for optical transmission
		teensy_ser.write(b'encode4b5b\n')

		if not skip_reset:
	        # Do a hard reset and then optically boot
			teensy_ser.write(b'bootopt4b5b\n')
		else:
	        # Skip the hard reset before booting
			teensy_ser.write(b'bootopt4b5bnorst\n')

	    # Display confirmation message from Teensy
		print(teensy_ser.readline())
	elif boot_mode == '3wb':
	    # Execute 3-wire bus bootloader on Teensy
		teensy_ser.write(b'boot3wb\n')

	    # Display confirmation message from Teensy
		print(teensy_ser.readline())
	else:
		raise ValueError("Boot mode '{}' invalid.".format(boot_mode))

	teensy_ser.write(b'opti_cal\n');
	teensy_ser.close()

	# Open UART connection to SCM
	uart_ser = serial.Serial(
		port=uart_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=.5)

	# After programming, several lines are sent from SCM over UART
	print(uart_ser.readline())
	print(uart_ser.readline())
	print(uart_ser.readline())
	print(uart_ser.readline())
	print(uart_ser.readline())
	# uart_ser.flushOutput()

	return

def test_adc_spot(uart_port="COM16", iterations=1,
		skip_reset=False, insert_CRC=False,
		pad_random_payload=False):
	"""
	Inputs:
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		iterations: Integer. Number of times to take readings.
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
		Note that this assumes that SCM has already been programmed.
	"""
	adc_outs = []

	uart_ser = serial.Serial(
		port=uart_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=.5)

	for i in range(12):
		time.sleep(.2)
		print(uart_ser.readline())

	for i in range(iterations):
		uart_ser.write(b'adc\n')
		time.sleep(.5)
		print(uart_ser.readline())
		adc_out = uart_ser.readline()
		adc_outs.append(adc_out)
	
	uart_ser.close()

	return adc_outs

def test_adc_psu(
		vin_vec, uart_port="COM18", 
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
	"""
	Inputs:
		vin_vec: 1D collection of floats. Input voltages in volts 
			to feed to the ADC.
		uart_port: String. Name of the COM port that the UART 
			is connected to.
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
	# Opening up the UART connection
	uart_ser = serial.Serial(
		port=uart_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=.5)


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
			uart_ser.write(b'adc\n')
			time.sleep(.5)
			print(uart_ser.readline())
			adc_out = uart_ser.readline()
			adc_outs[vin].append(adc_out)
			print("Vin={}V/Iteration {} -- {}".format(vin, i, adc_outs[vin][i]))


	# Due diligence for closing things out
	uart_ser.close()
	psu.close()
	
	return adc_outs

def test_adc_dnl(vin_min, vin_max, num_bits, 
		uart_port="COM18", 
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
	"""
	Inputs:
		vin_min/max: Float. The min and max voltages to feed to the
			ADC.
		num_bits: Integer. The bit resolution of the ADC.
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		psu_name: String. Name to use in the connection for the 
			waveform generator.
		iterations: Integer. Number of times to take a reading for
			a single input voltage.
	Outputs:
		Returns a collection of DNLs for each nominal LSB. The length
		should be 2**(num_bits)-1. Note that this assumes that SCM has 
		already been programmed.
	"""
	# Constructing the vector of input voltages to give the ADC
	vlsb_ideal = (vin_max-vin_min)/2**num_bits
	vin_vec = np.arange(vin_min, vin_max, vlsb_ideal/10)

	# Running the test
	adc_outs = test_adc_psu(vin_vec, psu_name, iterations)
	
	return calc_adc_dnl(adc_outs, vlsb_ideal)

def calc_adc_dnl(adc_outs, vlsb_ideal):
	"""
	Inputs: 
		adc_outs: A dictionary of ADC codes where the key is the vin,
			and the value is a list of measured codes (more than one
			measurement can be taken).
		vlsb_ideal: The nominal voltage associated with a single LSB.
	Outputs:
		Returns a collection of DNLs for each nominal LSB. The length
		should be 2**(num_bits)-1
	"""
	# Averaging over all the iterations to get an average of what the ADC
	# returns.
	adc_outs_avg = {vin:round(np.average(codes)) for (vin,codes) \
					in adc_outs.items()}

	vin_prev = min(adc_outs_avg.keys())
	code_prev = adc_outs_avg[vin_prev]
	DNLs = []

	# Calculate the DNL for every step
	for vin in sorted(adc_outs_avg.keys()):
		code = adc_outs_avg[vin]
		if code > code_prev:
			DNL_val = (vin-vin_prev)/vlsb_ideal - 1
			DNLs.append(DNL_val)
			vin_prev = vin
			# If a code is skipped, make the one it skipped functionally
			# zero width.
			if code > code_prev + 1:
				DNLs.append(0)
			code_prev = code
	return DNLs


def test_adc_inl_straightline(vin_min, vin_max, num_bits, 
		uart_port="COM18", 
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
	"""
	Inputs:
		vin_min/max: Float. The min and max voltages to feed to the
			ADC.
		num_bits: Integer. The bit resolution of the ADC.
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		psu_name: String. Name to use in the connection for the 
			waveform generator.
		iterations: Integer. Number of times to take a reading for
			a single input voltage.
	Outputs:
		Returns the slope, intercept of the linear regression
		of the ADC code vs. vin. Note that this assumes that SCM 
		has already been programmed.
	"""
	# Constructing the vector of input voltages to give the ADC
	vlsb_ideal = (vin_max-vin_min)/2**num_bits
	vin_vec = np.arange(vin_min, vin_max, vlsb_ideal/4)

	# Running the test
	adc_outs = test_adc_psu(vin_vec, psu_name, iterations)

	return calc_adc_inl_straightline(adc_outs, vlsb_ideal)

def calc_adc_inl_straightline(adc_outs, vlsb_ideal):
	"""
	Inputs: 
		adc_outs: A dictionary of ADC codes where the key is the vin,
			and the value is a list of measured codes (more than one
			measurement can be taken).
		vlsb_ideal: The nominal voltage associated with a single LSB.
	Outputs:
		Returns the slope, intercept of the linear regression of the 
		ADC codes vs. vin.

	"""
	# Averaging over all the iterations to get an average of what the ADC
	# returns.
	adc_outs_avg = {vin:round(np.average(codes)) for (vin,codes) \
					in adc_outs.items()}

	# Using linear regressiontto figure out the slope and intercept
	x = list(adc_outs_avg.keys())
	y = [adc_outs_avg[k] for k in x]
	slope, intercept, r_value, p_value, std_error = stats.linregress(x, y)

	return slope, intercept

def test_adc_inl_endpoint(vin_min, vin_max, num_bits, 
		uart_port="COM18", 
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
	"""
	Inputs:
		vin_min/max: Float. The min and max voltages to feed to the
			ADC.
		num_bits: Integer. The bit resolution of the ADC.
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		psu_name: String. Name to use in the connection for the 
			waveform generator.
		iterations: Integer. Number of times to take a reading for
			a single input voltage.
	Outputs:
		Returns a collection of endpoint INLs taken from vin_min 
		up to vin_max. Note that this assumes that SCM has already 
		been programmed.
	"""
	# Constructing the vector of input voltages to give the ADC
	vlsb_ideal = (vin_max-vin_min)/2**num_bits
	vin_vec = np.arange(vin_min, vin_max, vlsb_ideal/4)

	# Running the test
	adc_outs = test_adc_psu(vin_vec, psu_name, iterations)

	return calc_adc_inl_endpoint(adc_outs, vlsb_ideal)

def calc_adc_inl_endpoint(adc_outs, vlsb_ideal):
	"""
	Inputs: 
		adc_outs: A dictionary of ADC codes where the key is the vin,
			and the value is a list of measured codes (more than one
			measurement can be taken).
		vlsb_ideal: The nominal voltage associated with a single LSB.
	Outputs:
		Returns a collection of endpoint INLs taken from the minimum
		input voltage up to the maximum input voltage.
	"""
	# Averaging over all the iterations to get an average of what the ADC
	# returns.
	adc_outs_avg = {vin:round(np.average(codes)) for (vin,codes) \
					in adc_outs.items()}

	vin_min = min(adc_outs_avg.keys())
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
		program_cortex_specs = dict(teensy_port="COM14",
									uart_port="COM21",
									file_binary="../code.bin",
									boot_mode="optical",
									skip_reset=False,
									insert_CRC=True,
									pad_random_payload=False,)
		program_cortex(**program_cortex_specs)

	### Programming the Cortex and then attempting to ###
	### run a spot check with the ADC.				  ###
	if True:
		program_cortex_specs = dict(teensy_port="COM14",
									uart_port="COM21",
									file_binary="../code.bin",
									boot_mode="optical",
									skip_reset=False,
									insert_CRC=False,
									pad_random_payload=False,)
		program_cortex(**program_cortex_specs)

		test_adc_spot_specs = dict(
			uart_port="COM21",
			iterations=10,
			skip_reset=False,
			insert_CRC=False,
			pad_random_payload=False)
		adc_out = test_adc_spot(**test_adc_spot_specs)
		print(adc_out)
		adc_out_dict = dict()
		adc_out_dict[0.0] = adc_out

		fname = './spot_check.csv'
		write_adc_data(adc_out_dict, fname)

	### Programming the cortex and running many iterations on a large ###
	### sweep. ###
	if False:
		# program_cortex_specs = dict(teensy_port="COM14",
		# 							uart_port="COM21",
		# 							file_binary="../code.bin",
		# 							boot_mode="optical",
		# 							skip_reset=False,
		# 							insert_CRC=False,
		# 							pad_random_payload=False,)
		# program_cortex(**program_cortex_specs)

		test_adc_psu_specs = dict(vin_vec=[0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9],
								uart_port="COM21",
								psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
								iterations=100)
		adc_outs = test_adc_psu(**test_adc_psu_specs)

		ts = time.gmtime()
		datetime = time.strftime("%Y%m%d_%H%M%S",ts)
		write_adc_data(adc_outs, 'psu_{}'.format(datetime))

	### Reading in data and plotting appropriately ###
	if False:
		vlsb_ideal = 1.2/2**10

		file_sweep = './data/psu_run_first_linear.csv'
		adc_sweep = read_adc_data(file_sweep)

		dnl = calc_adc_dnl(adc_sweep, vlsb_ideal)
		# inl_endpoint = calc_adc_inl_endpoint(adc_sweep, vlsb_ideal)
		slope, intercept = calc_adc_inl_straightline(adc_sweep, vlsb_ideal)

		dnl_max = max(dnl)
		dnl_min = min(dnl)
		# inl_endpoint_max = max(inl_endpoint)
		# inl_endpoint_min = min(inl_endpoint)

		# print(dnl)

		print("DNL Min/Max: {}/{}".format(dnl_min, dnl_max))
		# print("INL Endpoint Min/Max: {}/{}".format(inl_endpoint_min, inl_endpoint_max))
		print("INL Straightline Slope/Intercept: {}/{}".format(slope, intercept))

		x = list(adc_sweep.keys())
		adc_avg = {vin:round(np.average(codes)) for (vin,codes) \
					in adc_sweep.items()}
		y = [adc_avg[i] for i in x]
		plt.plot(x,y,label="ADC Readings")
		plt.plot(x, [slope*i+intercept for i in x], label="INL")
		plt.plot(x, [1024/1.2*i for i in x], label="Ideal")
		plt.legend()
		plt.grid()
		plt.xlabel("$V_{IN}$ (V)")
		plt.ylabel("ADC Code")
		plt.title("$V_{DD}$ = 1.2V, Average over 5 Samples")
		plt.show()

	if False:
		file_noise = './data/psu_run_noise.csv'
		adc_noise = read_adc_data(file_noise)

		for vin in adc_noise.keys():
			plt.figure()
			plt.hist(adc_noise[vin], bins=100)
			plt.title("Vin={} V".format(vin))
			plt.xlabel("Code")
			plt.ylabel("Occurrences")
			plt.show()