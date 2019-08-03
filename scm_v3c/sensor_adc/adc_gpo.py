"""
Lydia Lee, 2019
This file contains the code to run various tests on the ADC. The functions in this file
are meant to communicate with other entities like the Teensy, SCM, and others. For post-
processing and data reading/writing/plotting, please see data_handling.py. This assumes
that you're working through the Cortex FSM and that outputs will be read via
second Teensy from the GPIOs.
"""
import numpy as np
import scipy as sp
import serial
import visa
import time
import random
from data_handling import *

def test_adc_spot_gpo(uart_port="COM16", teensy_port="COM20", iterations=1):
	"""
	Inputs:
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		teensy_port: String. Name of the COM port that the Teensy is connected to
			which will be interpreting the GPO outputs.
		iterations: Integer. Number of times to take readings.
	Outputs:
		Returns an ordered collection of ADC output codes where the ith element 
		corresponds to the ith reading. Note that this assumes that SCM has 
		already been programmed.
	"""
	adc_outs = []

	uart_ser = serial.Serial(
		port=uart_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=.5)

	teensy_ser = serial.Serial(
		port=teensy_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS,
		timeout=1)

	for i in range(12):
		time.sleep(.2)
		print(uart_ser.readline())

	for i in range(iterations):
		uart_ser.write(b'adc\n')
		time.sleep(.5)
		adc_out_str = teensy_ser.readline().decode('utf-8').replace('\n', '')
		adc_out = int(adc_out_str)
		adc_outs.append(adc_out)
	
	uart_ser.close()

	return adc_outs


def test_adc_psu_gpo(
		vin_vec, uart_port="COM18", teensy_port="COM20", 
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
def test_adc_dnl_gpo(vin_min, vin_max, num_bits, 
		uart_port="COM18", teensy_port="COM20", 
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
def test_adc_inl_straightline_gpo(vin_min, vin_max, num_bits, 
		uart_port="COM18", teensy_port="COM20", 
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):
def test_adc_inl_endpoint_gpo(vin_min, vin_max, num_bits, 
		uart_port="COM18", teensy_port="COM20", 
		psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
		iterations=1):