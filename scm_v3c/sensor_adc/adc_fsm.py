def trigger_uart(uart_ser):
	"""
	Inputs:
		uart_ser: The serial connection (type Serial) associated with the UART
			serial connection. This is _not_ a string!
	Outputs: 
		No return value. Triggers the ADC conversion via UART.
	"""
	uart_ser.write(b'adc\n')

def trigger_gpi():
	"""
	Inputs:
	Outputs:
	"""
