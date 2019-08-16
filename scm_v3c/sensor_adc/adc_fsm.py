def trigger_spot(uart_ser, mode='uart'):
	"""
	Inputs:
		uart_ser: The serial connection (type Serial) associated with the UART
			serial connection. This is _not_ a string!
		mode: String 'uart', 'loopback', or 'gpio'. uart indicates a spot reading
			taken with the on-chip FSM controlling the ADC. loopback indicates
			the Cortex using GPIO loopback to control the ADC. gpio indicates
			some external driver (e.g. a Teensy) controlling the ADC via GPIO.
	Outputs: 
		No return value. Triggers a single ADC conversion.
	"""
	if mode == 'uart':
		uart_ser.write(b'ad1\n')
	elif mode == 'loopback':
		uart_ser.write(b'ad2\n')
	elif mode == 'gpio':
		# uart_ser.write(b'ad3\n')
		print("External GPIO control under construction")

	return

def trigger_continuous(uart_ser, mode='uart'):
	"""
	Inputs:
		uart_ser: The serial connection (type Serial) associated with the UART
			serial connection. This is _not_ a string!
		mode: String 'uart', 'loopback', or 'gpio'. uart indicates a spot reading
			taken with the on-chip FSM controlling the ADC. loopback indicates
			the Cortex using GPIO loopback to control the ADC. gpio indicates
			some external driver (e.g. a Teensy) controlling the ADC via GPIO.
	Outputs: 
		No return value. Triggers never-ending consecutive ADC conversions until
		halt_continuous() is called.
	"""
	if mode == 'uart':
		uart_ser.write(b'ad4\n')
	elif mode == 'loopback':
		uart_ser.write(b'ad5\n')
	elif mode == 'gpio':
		# uart_ser.write(b'ad6\n')
		print("External GPIO control under construction")
	return

def halt_continuous(uart_ser):
	"""
	Inputs:
		uart_ser: The serial connection (type Serial) associated with the UART
			serial connection. This is _not_ a string!
	Outputs:
		No return value. Halts a continuous ADC conversion by setting an internal
		flag. Otherwise does nothing.
	"""
	uart_ser.write(b'ad0\n')
	return


def read_uart(uart_ser):
	"""
	Inputs:
		uart_ser: The serial connection (type Serial) associated with the UART
			serial connection. This is _not_ a string!
	Outputs:
		Returns the integer ADC output. Reads the ADC value via UART. This prints
		out some of the intermediate excess that gets sent over UART. A return value of
		2048 indicates a blank string read. 4096 indicates an inability to convert the 
		non-empty read value to an integer.
	"""
	print(uart_ser.readline())
	adc_out_raw = uart_ser.readline()
	if adc_out_raw.decode('utf-8') == '':
		return 2048
	else:
		adc_out_str = adc_out_raw.decode('utf-8').replace('\n', '')

	try:
		adc_out = int(adc_out_str)
	except ValueError:
		print("WARNING: {} not an int".format(adc_out_str))
		return 4096

	return adc_out

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
		teensy_uC_adc.ino. A return value of 2048 indicates a timeout while
		waiting on the ADC to finish converting.

		sensoradcinitialize should have run on the Teensy at some point 
		before running this function.

		Untested.
	"""
	print(teensy_ser.readline())
	teensy_ser.write(b'sensoradcread\n')
	adc_out_str = teensy_ser.readline().decode('utf-8').replace('\n','')
	return int(adc_out_str)

def initialize_gpio(teensy_ser):
	"""
	Inputs:
		teensy_ser: The serial connection (type Serial) associated with the 
			Teensy you'll be going through to talk to the chip.
	Outputs:
		No return value. Initializes all the GPIO in/outs to the correct settings
		on the Teensy (NOT on SCM!)
	Notes:
		Untested.
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
		The Teensy should have been flashed with the code in teensy_uC_adc.ino

		sensoradcinitialize should have run on the Teensy at some point 
		before running this function.

		Note that this is just an alternative to using on-chip GPIO loopback.

		Untested.
	"""
	teensy_ser.write(b'sensoradctrigger\n')
	teensy_ser.write(adc_settle_cycles)
	teensy_ser.write(pga_bypass)
	teensy_ser.write(pga_settle_us)
	return