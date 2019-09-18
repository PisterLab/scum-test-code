"""
Lydia Lee, 2019
This file contains the code to run various tests on the ADC. The functions in this file
are meant to communicate with other entities like the Teensy, SCM, and others. For post-
processing and data reading/writing/plotting, please see data_handling.py. This assumes
that you're working strictly through the Cortex and that outputs will be read from the
UART.
"""
# import numpy as np
# import scipy as sp
import serial
import visa
# import time
# import random
# from data_handling import *
from sensor_adc.adc_fsm import *
# from pprint import pprint

# import sys
# sys.path.append('..')
# from bootload import *

def test_adc_spot(port="COM16", control_mode='uart', read_mode='uart', iterations=1,
		gpio_settings=dict()):
	"""
	Inputs:
		port: String. Name of the COM port that the UART or the reading Teensy
			is connected to.
		control_mode: String 'uart', 'loopback' 'gpio'. Determines if you'll be triggering
			the FSM via UART, GPIO loopback, or externally-controlled (e.g. Teensy) GPI.
		read_mode: String 'uart' or 'gpio'. Determines if you're reading via GPIO.
		iterations: Integer. Number of times to take readings.
		gpio_settings: Dictionary with the following key:value pairings
			adc_settle_cycles: Integer [1,256]. Unclear what the control in the original 
				FSM is, but this will dictate the number of microseconds to wait 
				for the ADC to settle. Applicable only if the Teensy is used rather than
				UART.
			pga_settle_us: Integer. Number of microseconds to wait for the PGA output to
				to settle. Applicable only if the Teensy is used rather than
				UART.
	Outputs:
		Returns an ordered collection of ADC 
		output codes where the ith element corresponds to the ith reading.
		Note that this assumes that SCM has already been programmed.
	Raises:
		ValueError if the control mode is not 'uart', 'gpio', or 'loopback'.
		ValueError if the read mode is not 'uart' or 'gpio'.
	"""
	if control_mode not in ['uart', 'loopback', 'gpio']:
		raise ValueError("Invalid control mode {}".format(control_mode))
	if read_mode not in ['uart', 'gpio']:
		raise ValueError("Invalid read mode {}".format(read_mode))
	
	adc_outs = []

	ser = serial.Serial(
		port=port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=1)

	# Initializing the Teensy settings for GPIO control
	if control_mode == 'gpio':
		initialize_gpio(teensy_ser)

	# Take all the readings
	for i in range(iterations):
		trigger_spot(ser, control_mode)

		if read_mode == 'uart':
			read_func = read_uart
		elif read_mode == 'gpio':
			read_func = read_gpo

		adc_out = read_func(ser)
		adc_outs.append(adc_out)
	
		# TODO: atm we're soft resetting, but this shouldn't 
		# be necessary.
		# ser.write(b'sft\n')
		# print(ser.readline())
		# print(ser.readline())
		# print(ser.readline())
		# print(ser.readline())
		# print(ser.readline())
		# print(ser.readline())

		
	# Due diligence
	ser.close()

	return adc_outs

def test_adc_psu(
		vin_vec, port="COM18", control_mode='uart', read_mode='uart',
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1, gpio_settings=dict()):
	"""
	Inputs:
		vin_vec: 1D collection of floats. Input voltages in volts 
			to feed to the ADC.
		port: String. Name of the COM port that the UART or the reading Teensy
			is connected to.
		control_mode: String 'uart', 'loopback' 'gpio'. Determines if you'll be triggering
			the FSM via UART, GPIO loopback, or externally-controlled (e.g. Teensy) GPI.
		read_mode: String 'uart' or 'gpio'. Determines if you're reading via GPIO.
		psu_name: String. Name to use in the connection for the 
			waveform generator.
		iterations: Integer. Number of times to take a reading for
			a single input voltage.
		gpio_settings: Dictionary with the following key:value pairings
			adc_settle_cycles: Integer [1,256]. Unclear what the control in the original 
				FSM is, but this will dictate the number of microseconds to wait 
				for the ADC to settle. Applicable only if the Teensy is used rather than
				UART.
			pga_settle_us: Integer. Number of microseconds to wait for the PGA output to
				to settle. Applicable only if the Teensy is used rather than
				UART.
	Outputs:
		Returns a dictionary adc_outs where adc_outs[vin][i] will give the 
		ADC code associated with the i'th iteration when the input
		voltage is 'vin'.

		Uses a serial as well as an arbitrary waveform generator to
		sweep the input voltage to the ADC. Bypasses the PGA. Does 
		NOT program scan.
	Raises:
		ValueError if the control mode is not 'uart', 'loopback', or 'gpio'.
		ValueError if the read mode is not 'uart' or 'gpio'.
	Notes:
		Externally controlled GPIO is under construction.
	"""
	if control_mode not in ['uart', 'loopback', 'gpio']:
		raise ValueError("Invalid control mode {}".format(control_mode))
	if read_mode not in ['uart', 'gpio']:
		raise ValueError("Invalid read mode {}".format(read_mode))

	# Opening up the UART connection
	ser = serial.Serial(
		port=port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=1)

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
	if control_mode == 'gpio':
		print('External GPIO control under construction')
		# initialize_gpio(teensy_ser)

	adc_outs = dict()
	for vin in vin_vec:
		adc_outs[vin] = []
		psu.write("SOURCE2:VOLTAGE:OFFSET {}".format(vin))
		for i in range(iterations):
			trigger_spot(ser, control_mode)

			if read_mode == 'uart':
				read_func = read_uart
			elif read_mode == 'gpio':
				read_func = read_gpo
			
			adc_out = read_func(ser)
			
			adc_outs[vin].append(adc_out)
			print("Vin={}V/Iteration {} -- {}".format(vin, i, adc_outs[vin][i]))

			# TODO: atm we're soft resetting, but this shouldn't 
			# be necessary.
			# ser.write(b'sft\n')
			# print(ser.readline())
			# print(ser.readline())
			# print(ser.readline())
			# print(ser.readline())
			# print(ser.readline())
			# print(ser.readline())
	# Due diligence for closing things out
	ser.close()
	psu.write("SOURCE2:VOLTAGE:OFFSET 0")
	psu.write("OUTPUT2 OFF")
	psu.close()
	
	return adc_outs

if __name__ == "__main__":
	print("Don't modify this file. Use ../run_me.py ")