def trigger_uart(uart_ser):
	"""
	Inputs:
		uart_ser: The serial connection (type Serial) associated with the UART
			serial connection. This is _not_ a string!
	Outputs: 
		No return value. Triggers the ADC conversion via UART.
	"""
	uart_ser.write(b'adc\n')
	return

def initialize_gpio(teensy_ser):
	"""
	Inputs:
		teensy_ser: The serial connection (type Serial) associated with the 
			Teensy you'll be going through to talk to the chip.
	Outputs:
		No return value. Initializes all the GPIO in/outs to the correct settings
		on the Teensy (NOT on SCM!)
	"""
	teensy_ser.write(b'sensoradcinitialize')
	return

def trigger_gpi(teensy_ser, adc_settle_cycles, pga_bypass, pga_settle_us):
	"""
	Inputs:
		teensy_ser: The serial connection (type Serial) associated with the 
			Teensy you'll be going through to talk to the chip.
		adc_settle_cycles: Integer [1,256]. Unclear what the control in the original 
			FSM is, but this will dictate the number of microseconds to wait 
			for the ADC to settle.
		pga_bypass: Integer 0 or 1. 1 = bypass the PGA, otherwise use the PGA.
		pga_settle_us: Integer. Number of microseconds to wait for the PGA output to
			to settle.
	Outputs:
		No return value. Triggers the ADC conversion and controls the FSM with the 
		Teensy in question with the specified ADC settling time.
	Notes:
		The Teensy should have been flashed with the code in teensy_uC_programmer.ino

		sensoradcinitialize should have run on the Teensy at some point 
		before running this function.
	"""
	teensy_ser.write(b'sensoradctrigger\n')
	teensy_ser.write(adc_settle_cycles)
	teensy_ser.write(pga_bypass)
	teensy_ser.write(pga_settle_us)
	return

def read_uart(uart_ser):
	"""
	Inputs:
		uart_ser: The serial connection (type Serial) associated with the UART
			serial connection. This is _not_ a string!
	Outputs:
		Returns the integer ADC output. Reads the ADC value via UART. This prints
		out some of the intermediate excess that gets sent over UART.
	"""
	print(uart_ser.readline())
	adc_out_str = uart_ser.readline().decode('utf-8').replace('\n', '')
	return int(adc_out_str)

def read_gpo(teensy_ser):
	"""
	Inputs:
		teensy_ser: The serial connection (type Serial) associated with the Teensy
			serial connection. This is _not_ a string!
	Outputs:
		Returns the integer ADC output. A return value of 2048 indicates a timeout while
		waiting on the ADC to finish converting.
	Notes:
		If using the Teensy, the Teensy should have been flashed with the code in 
		teensy_uC_programmer.ino. A return value of 2048 indicates a timeout while
		waiting on the ADC to finish converting.

		sensoradcinitialize should have run on the Teensy at some point 
		before running this function.
	"""
	print(teensy_ser.readline())
	teensy_ser.write(b'sensoradcread\n')
	adc_out_str = teensy_ser.readline().decode('utf-8').replace('\n','')
	return int(adc_out_str)