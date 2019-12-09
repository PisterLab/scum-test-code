import serial
from bootload import *

def demo_hello_world(uart_port):
	"""
	Inputs:
		uart_port: String. Name of the COM port that the SCM UART 
			is connected to.
	Outputs:
		No return value. Sends SCM a command over UART and listens
		for its response.
	Notes:
		Assumes SCM has already been programmed.
	"""
	# Opens serial connection to SCM
	uart_ser = serial.Serial(
			port=uart_port,
			baudrate=19200,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS,
			timeout=2)

	# Sends SCM 'zzz\n' over UART
	uart_ser.write(b'zzz\n')

	# SCM should talk back
	print(uart_ser.readline())

	# Due diligence: Close your serial ports!
	uart_ser.close()

	return


def demo_gpio_control(uart_port):
	"""
	Inputs:
		uart_port: String. Name of the COM port that the SCM UART 
			is connected to.
	Outputs:
		No retun value. Sends SCM a command over UART to toggle GPIOs.
	Notes:
		Assumes SCM has already been programmed.
	"""
	# Opens serial connection to SCM
	uart_ser = serial.Serial(
			port=uart_port,
			baudrate=19200,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS,
			timeout=2)

	# Sends SCM 'zzz\n' over UART
	# SCM won't talk back in this case
	uart_ser.write(b'zzz\n')

	# Due diligence: Close your serial ports!
	uart_ser.close()

	return


if __name__ == "__main__":
	programmer_port = "COM5"
	scm_port = "COM13"
#								file_binary="./ALLGPIOToggle.bin",
	# Programs SCM
	program_cortex_specs = dict(teensy_port=programmer_port,
								uart_port=None,
								file_binary="./code.bin",
								boot_mode="optical",
								skip_reset=False,
								insert_CRC=True,
								pad_random_payload=False)

	program_cortex(**program_cortex_specs)

	if False:
		demo_hello_world(uart_port=scm_port)

	if False:
		demo_gpio_control(uart_port=scm_port)