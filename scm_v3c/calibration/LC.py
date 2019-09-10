import numpy as np
import scipy as sp
import serial
"""
2019. Contains Python code to run calibration schemes for LC tank.
"""

def cal_LC(scm_port="COM13", board_ID=None, T=300):
	"""
	Inputs:
		scm_port: String. Name of the COM port that SCM UART is connected to.
		board_ID: String. Name of the board (e.g. "Q7") under test. 
			Case sensitive.
		T: Float. Temperature in Kelvin.
	Outputs:
		No return value. Runs the calibration and has SCM talk back providing
		information about the
	Notes:
		Under construction.
	"""
	# Connecting to SCM over UART
	ser = serial.Serial(
		port=scm_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=1)

	# Tell SCM to start LC tank calibration.
	# TODO: There should be matching C!
	ser.write(b'')

	# There are 5 coarse and 5 mid bits. Fine tuning is locally monotonic 
	# (and roughly linear), but any time a coarse and/or mid bit changes, 
	# there is a skip.
	for i in 1024:

	ser.close()

	return


def test_LC_sweep(uart_port, num_codes=2048):
	"""
	Inputs:
		uart_port: String. Name of the COM port that SCM UART is connected to.
		num_codes: Integer. The expected number of LC codes.
	Outputs:
		Returns a dictionary where keys are LC code and values are lists of
		measured frequencies associated with that code. Triggers the LC to 
		start sweeping across nonmonotonic points and stores it in the 
		dictionary.
	Raises:
		ValueError if the expected number of iterations doesn't come out of SCM
		on the first reading.
		ValueError if expecting the number of iterations but didn't get an integer.
	Note:
		This assumes SCM has already been programmed.
		The number of iterations and code are controlled via C.
		This is intended for use with C function of the same name.
	"""
	LC_outs = dict()

	ser = serial.Serial(
		port=uart_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=1)

	# TODO: Choose a code
	ser.write(b'\n')

	# SCM spits out the number of reps it'll have for each code.
	# If there's nothing, raise an error.
	iterations_raw = ser.readline()
	if iterations_raw.decode('utf-8') == '':
		raise ValueError("Nothing received when expecting number of iterations.")
	else:
		iterations_str = iterations_raw.decode('utf-8').replace('\n', '')

	# Convert the string readout to an integer. If it doesn't convert, raise an error.
	try:
		iterations = int(iterations_str)
	except ValueError:
		raise ValueError("Number of iterations {} is not an int".format(iterations_str))

	# All subsequent readings should take the form CODE:FREQUENCY.
	for _ in range(num_codes):
		for i in range(iterations):
			LC_reading_raw = ser.readline()
			LC_reading = LC_reading_raw.decode('utf-8').replace('\n', '').split(':')
			code = LC_reading[0]
			counter_out = LC_reading[1]

			

			if code in LC_outs.keys():
				LC_outs[code] = LC_outs[code] + [freq]
			else:
				LC_outs[code] = [freq]
	
	# Housekeeping
	ser.close()

	return LC_outs
