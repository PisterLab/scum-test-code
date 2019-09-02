"""
2019.
Use this as a testing ground for verifying the correctness of your C code for
SCM. Intended to work in conjunction with test_code.c.
"""

import serial
from bootload import *
from sensor_adc.adc_fsm import *

def test_get_asc_bit(uart_port, output_file):
	"""
	Inputs
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		output_file: String. Path to the file to write the read serial
			port output to.
	Outputs
		No return value. Assuming test_get_asc_bit() in C is running,
		reads the output from that via serial port.
	"""
	uart_ser = serial.Serial(
			port=uart_port,
			baudrate=19200,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS,
			timeout=.5)

	with open(output_file,'wb') as f:
		for i in range(1200*2):
			output = uart_ser.readline()
			print(output)
			f.write(output)

	uart_ser.close()

def test_get_GPIO_enables(uart_port, output_file):
	"""
	Inputs:
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		output_file: String. Path to the file to write the read serial
			port output to.
	Outputs:
		No return value. Assuming test_get_GPIO_enables() in C is running,
		reads the output from that via serial port.
	"""
	uart_ser = serial.Serial(
			port=uart_port,
			baudrate=19200,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS,
			timeout=.5)

	with open(output_file,'wb') as f:
		for i in range(65535):
			read1 = uart_ser.readline()
			print(read1)
			read2 = uart_ser.readline()
			print(read2)
			f.write(read1)
			f.write(read2)

	uart_ser.close()

def test_get_GPI_control(uart_port, output_file):
	"""
	Inputs:
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		output_file: String. Path to the file to write the read serial
			port output to.
	Outputs:
		No return value. Assuming test_get_GPI_control() in C is running,
		reads the output from that via serial port.
	"""
	uart_ser = serial.Serial(
			port=uart_port,
			baudrate=19200,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS,
			timeout=1)

	with open(output_file,'wb') as f:
		# Outer loop is the value being written to the bank
		for i in range(4):
			# Inner loop is the row whose bank is being set
			for i in range(4):
				read = uart_ser.readline()
				print(read)
				f.write(read)

	uart_ser.close()


def test_get_GPO_control(uart_port, output_file):
	"""
	Inputs:
		uart_port: String. Name of the COM port that the UART 
			is connected to.
		output_file: String. Path to the file to write the read serial
			port output to.
	Outputs:
		No return value. Assuming test_get_GPO_control() in C is running,
		reads the output from that via serial port.
	"""
	uart_ser = serial.Serial(
			port=uart_port,
			baudrate=19200,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS,
			timeout=1)

	with open(output_file, 'wb') as f:
		# Outer loop is the value being written to the bank
		for i in range(16):
			# Inner loop is the row whose bank is being set
			for i in range(4):
				read = uart_ser.readline()
				print(read)
				f.write(read)

	uart_ser.close()

if __name__ == "__main__":
	programmer_port = "COM15"
	scm_port = "COM23"

	### Program the cortex ###
	if True:
		program_cortex_specs = dict(teensy_port=programmer_port,
										uart_port=scm_port,
										file_binary="./code.bin",
										boot_mode="optical",
										skip_reset=False,
										insert_CRC=True,
										pad_random_payload=False,)
		program_cortex(**program_cortex_specs)

	if False:
		test_get_asc_bit(scm_port, './testing_scratch/get_asc_bit.output')

	if False:
		test_get_GPIO_enables(scm_port, './testing_scratch/get_GPIO_enables.output')

	if True:
		test_get_GPI_control(scm_port, './testing_scratch/get_GPI_control.output')

	if False:
		test_get_GPO_control(scm_port, './testing_scratch/get_GPO_control.output')
	