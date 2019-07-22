import serial
import random
import os
# import fcntl
# import termios
import sys
import time
import struct
import difflib
import visa
from subprocess import Popen, PIPE

####################################################
####################################################
################## Scan Functions ################## 
####################################################
####################################################

def program_scan(com_port, ASC):
	"""
	Inputs:
		com_port: String. Name of the COM port to connect to.
		ASC: List of integers. Analog scan chain bits.
	Outputs:
		None. Programs the scan chain and prints if scan in matches
		scan out.
	"""
	# Open COM port to teensy to bit-bang scan chain
	ser = serial.Serial(
	    port=com_port,
	    baudrate=19200,
	    parity=serial.PARITY_NONE,
	    stopbits=serial.STOPBITS_ONE,
	    bytesize=serial.EIGHTBITS
	)

	# ASC configuration
	time.sleep(0.1)

	# Convert array to string for uart
	ASC_string = ''.join(map(str,ASC));

	# Send string to uC and program into IC
	ser.write(b'ascwrite\n');
	print(ser.readline());
	ser.write((ASC_string[::-1]).encode());
	print(ser.readline());

	# Execute the load command to latch values inside chip
	ser.write(b'ascload\n');
	print(ser.readline());

	# Read back the scan chain contents
	ser.write(b'ascread\n');
	x = ser.readline();
	scan_out = x[::-1];
	print(ASC_string)
	print(scan_out.decode())

	# Compare what was written to what was read back
	if int(ASC_string) == int(scan_out.decode()):
		ser.close();
		print('Read matches Write')
	else:
		ser.close();
		raise ValueError('Read/Write Comparison Incorrect')

def construct_ASC(radio_en_tx=[0], radio_lo_ftune = [0,0,0,0,0,0],
					radio_lo_itune = [0,0,0], radio_en_lo = [0],
					radio_lo_fine = [0,0], radio_en_debug_degen = [0],
					radio_en_debug_driver = [0], radio_en_output_degen = [0],
					radio_en_output_drive = [0], cam_row = [0,0,0,0],
					cam_col = [0,0,0,0,0], cam_read = [0,0,0,0,0,0,0,0,0,0],
					cam_exposure = [0,0,0,0,0,0,0,0,0,0,0,0,0,0], cam_en_dig = [0],
					cam_gain = [0,0], cam_en_pga = [0], cam_en_pixel_out = [0]):
	"""
	Inputs:
		radio_en_tx: 			TX enable
		radio_lo_ftune: 		LO ftune <5:0>
		radio_lo_itune: 		LO itune <2:0>
		radio_en_lo: 			LO enable
		radio_lo_fine: 			LO fine <1:0>
		radio_en_debug_degen: 	Debug degeneration enable
		radio_en_debug_driver: 	Debug driver enable
		radio_en_output_degen: 	Output degeneration enable
		radio_en_output_drive: 	Output drive enable

		cam_row: 				SC row choice <3:0>
		cam_col: 				SC column choice <4:0>
		cam_read: 				SC read cycles <9:0>
		cam_exposure: 			SC exposure cycles <13:0>
		cam_en_dig: 			SC digital enable
		cam_gain: 				SC gain control <1:0>
		cam_en_pga: 			SC PGA enable
		cam_en_pixel_out: 		SC pixel out buffer enable
	Outputs:
		The constructed ASC list. You should be able to use things 
		out of the box and not worry about inverting/reversing.
	"""

	# Reversing for breaking into chunks and for ease of indexing.
	cam_exposure_rev = cam_exposure[::-1]
	radio_lo_ftune_rev = radio_lo_ftune[::-1]
	radio_lo_itune_rev = radio_lo_itune[::-1]
	radio_lo_fine_rev = radio_lo_fine[::-1]

	ASC = radio_en_tx + \
			radio_lo_ftune_rev[3:6] + \
			radio_lo_ftune_rev[0:3] + \
			[radio_lo_itune_rev[1]] + \
			[radio_lo_itune_rev[0]] + \
			[radio_lo_itune_rev[2]] + \
			radio_en_lo + \
			radio_lo_fine_rev + \
			radio_en_debug_degen + \
			radio_en_debug_driver + \
			radio_en_output_degen + \
			radio_en_output_drive + \
			[0]*17 + \
			cam_gain[::-1] + \
			cam_en_pga + \
			cam_en_pixel_out + \
			cam_row + \
			cam_read + \
			cam_exposure_rev[9:0:-1] + \
			cam_exposure_rev[13:9:-1] + \
			[cam_exposure_rev[0]] + \
			cam_en_dig + \
			cam_col
	ASC = ASC[::-1]
	ASC[:] = [int(1-x) for x in ASC] # invert the scan chain :)

	return ASC

#################################################
#################################################
################## PGA Testing ################## 
#################################################
#################################################

def program_scan_pga(com_port, gain_bits):
	"""
	Inputs:
		com_port: String. Name of the COM port to connect to.
		gain_bits: The gain setting <1:0>. The mapping is
			00 = 1
			01 = 4/3
			10 = 2
			11 = 4
	Outputs:
		None. Configures the scan chain to PGA-only testing.
	"""
	ASC_parts = dict(radio_en_tx=[0],	
					radio_lo_ftune = [0,0,0,0,0,0],
					radio_lo_itune = [0,0,0],
					radio_en_lo = [0],
					radio_lo_fine = [0,0],
					radio_en_debug_degen = [0],
					radio_en_debug_driver = [0],
					radio_en_output_degen = [0],
					radio_en_output_drive = [0],
					cam_row = [0,0,0,0],
					cam_col = [0,0,0,0,0],
					cam_read = [0,0,0,0,0,0,0,0,0,0],
					cam_exposure = [0,0,0,0,0,0,0,0,0,0,0,0,0,0],
					cam_en_dig = [0],
					cam_gain = gain_bits,
					cam_en_pga = [1],
					cam_en_pixel_out = [0])
	ASC = construct_ASC(**ASC_parts)
	ASC[:] = [int(1-x) for x in ASC] # invert the scan chain
	program_scan(com_port, ASC)
	return

def test_pga_variance(gain_bits, iterations=500, vin=0.5, 
		com_port="COM10",
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR'):
	"""
	Inputs:
		gain_bits: The gain setting <1:0>. The mapping is
			00 = 1
			01 = 4/3
			10 = 2
			11 = 4
		com_port: String. Name of the COM port to connect to.
		psu_name: String. Name used by Visa to find the PSU.
		iterations: Integer. Number of readings to take before stepping
			the function generator voltage.
		vin: Float. The voltage to test the PGA gain at.
	Outputs:
		 Returns a list of ydata (PGA output voltage reading) where the nth element 
		 (indexing from 0) is the nth reading. Uses the function generator to set 
		 the input voltage of the test PGA.
	"""
	# Program the scan chain appropriately
	program_scan_pga(com_port, gain_bits)
	ser = serial.Serial(
	    port=com_port,
	    baudrate=19200,
	    parity=serial.PARITY_NONE,
	    stopbits=serial.STOPBITS_ONE,
	    bytesize=serial.EIGHTBITS
	)
	time.sleep(0.1)
	ser.write(b'test_pga\n')
	print(ser.readline());

	# Connect to the function generator and set the conditions
	# for the connection appropriately.
	rm = visa.ResourceManager()
	psu = rm.open_resource(psu_name)
	psu.query("*IDN?")
	psu.write('OUTPUT2:LOAD INF')
	psu.write('SOURCE2:FUNCTION DC')
	psu.write('OUTPUT2 ON')

	# Step the function generator and read from the Teensy analog read.
	vout = []
	for i in iterations:
		vout.extend([ser.readline()])

	# Due diligence for closing things out
	ser.close()
	psu.close()

	return vout

def test_pga_gain(gain_bits, com_port="COM10", 
					psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
					iterations=10):
	"""
	Inputs:
		gain_bits: The gain setting <1:0>. The mapping is
			00 = 1
			01 = 4/3
			10 = 2
			11 = 4
		com_port: String. Name of the COM port to connect to.
		psu_name: String. Name used by Visa to find the PSU.
		iterations: Integer. Number of readings to take before stepping
			the function generator voltage.
	Outputs:
		 Returns the xdata (input voltage) and ydata (PGA output voltage). 
		 Uses the function generator to sweep the input voltage
		 of the test PGA. 
	"""
	# Program the scan chain appropriately
	program_scan_pga(com_port, gain_bits)

	# Open the serial with Teensy and start the PGA and digital clocks
	ser = serial.Serial(
	    port=com_port,
	    baudrate=19200,
	    parity=serial.PARITY_NONE,
	    stopbits=serial.STOPBITS_ONE,
	    bytesize=serial.EIGHTBITS
	)
	time.sleep(0.1)
	ser.write(b'test_pga\n')
	# print(ser.readline());

	# Connect to the function generator and set the conditions
	# for the connection appropriately.
	rm = visa.ResourceManager()
	psu = rm.open_resource(psu_name)
	psu.query("*IDN?")
	psu.write('OUTPUT2:LOAD INF')
	psu.write('SOURCE2:FUNCTION DC')
	psu.write('OUTPUT2 ON')

	# Step the function generator and read from the Teensy analog read.
	vin_vec = np.arange(0, 1, 5e-3)
	vout = dict()
	for vin in vin_vec:
		psu.write('SOURCE2:VOLTAGE:OFFSET {}'.format(vin))
		for i in range(iterations):
			# TODO: Convert from the analogRead output
			# to actual analog values. Match analog resolution!
			vout[vin][i] = ser.readline()

	# Due diligence for closing things out
	ser.close()
	psu.close()

	return vin, vout

#################################################
#################################################
################## Main Method ################## 
#################################################
#################################################

if __name__ == "__main__":
	### Scan chain programming ###
	if True:
		ASC_parts = dict(radio_en_tx=[0],	
						radio_lo_ftune = [0,0,0,0,0,0],
						radio_lo_itune = [0,0,0],
						radio_en_lo = [0],
						radio_lo_fine = [0,0],
						radio_en_debug_degen = [0],
						radio_en_debug_driver = [0],
						radio_en_output_degen = [0],
						radio_en_output_drive = [0],
						cam_row = [0,0,0,0],
						cam_col = [0,0,0,0,0],
						cam_read = [0,0,0,0,0,0,0,0,0,0],
						cam_exposure = [1,0,1,0,0,1,0,0,0,1,0,0,0,0],
						cam_en_dig = [1],
						cam_gain = [0,0],
						cam_en_pga = [1],
						cam_en_pixel_out = [1])

		ASC = construct_ASC(**ASC_parts)

		# This should be 72!
		print(len(ASC))

		# Program the scan chain
		program_scan('COM10', ASC)

	### Scratch: Teensy AnalogRead Sanity Check ###
	if True:
		com_port = 'COM10'
		ser = serial.Serial(
	    port=com_port,
	    baudrate=19200,
	    parity=serial.PARITY_NONE,
	    stopbits=serial.STOPBITS_ONE,
	    bytesize=serial.EIGHTBITS
		)
		time.sleep(0.1)
		ser.write(b'test_pga\n')
		# for i in range(10):
		# 	print(ord(ser.read()))
		ser.close()

	### Scratch: Connecting to the PSU via GPI ###
	if False:
		psu_name = 'USB0::0x0957::0x2C07::MY57801384::0::INSTR'
		rm = visa.ResourceManager()
		psu = rm.open_resource(psu_name)
		psu.query("*IDN?")
		psu.write('OUTPUT2:LOAD INF')
		psu.write('SOURCE2:FUNCTION DC')
		psu.write('OUTPUT2 ON')
		psu.close()

	### PGA variance measurement ###
	if False:
		vin = 0.5
		gain_bits = [0,0]
		iterations = 500
		vout = test_pga_variance(gain_bits, iterations, vin)

		# Plot Vout vs. iteration to see if there are any settling
		# time issues
		plt.figure()
		plt.plot(range(len(vout), vout))
		plt.xlabel("Iteration")
		plt.ylabel("$V_{out}$ (V)")
		plt.title("$V_{in}$ = {} V".format(vin))

		# Plot a histogram of Vout
		plt.figure()
		plt.hist(vout, bins=10)
		plt.xlabel("$V_{out}$ (V)")
		plt.ylabel("Occurrences")