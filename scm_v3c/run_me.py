import numpy as np
import scipy as sp
import serial
import visa
import time
import random
import matplotlib.pyplot as plt
import csv

# from calibration.LC import *
# from calibration.data_handling import *
from sensor_adc.adc import *
from sensor_adc.adc_fsm import *
from sensor_adc.data_handling import *
from bootload import *


if __name__ == "__main__":
	programmer_port = "COM5"
	scm_port = "COM3"

	# Program SCM
	if False:
		program_cortex_specs = dict(teensy_port=programmer_port,
								uart_port=scm_port,
								file_binary="./code.bin",
								boot_mode="optical",
								skip_reset=False,
								insert_CRC=True,
								pad_random_payload=False)

		program_cortex(**program_cortex_specs)


	# Send zzz over UART to SCM
	if False:
		ser = serial.Serial(
			port=scm_port,
			baudrate=19200,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS,
			timeout=.5)
		ser.write(b'zzz\n')
		ser.close()

	##################
	### SENSOR ADC ###
	##################
	control_mode = 'loopback'
	read_mode = 'uart'

	### Running a spot check with the ADC ###
	if False:
		test_adc_spot_specs = dict(
			port=scm_port,
			control_mode=control_mode,
			read_mode=read_mode,
			iterations=3)
		adc_out = test_adc_spot(**test_adc_spot_specs)
		print(adc_out)

	### Running many iterations on a large ###
	### sweep. ###
	if False:
		test_adc_psu_specs = dict(vin_vec=np.arange(0, 1.2, .2e-3),
								port=scm_port,
								control_mode=control_mode,
								read_mode=read_mode,
								psu_name='USB0::0x0957::0x2C07::MY57801384::0::INSTR',
								iterations=20,
								gpio_settings=dict())
		adc_outs = test_adc_psu(**test_adc_psu_specs)

		ts = time.gmtime()
		datetime = time.strftime("%Y%m%d_%H%M%S",ts)
		write_adc_data(adc_outs, './sensor_adc/data/psu_{}.csv'.format(datetime))

	### Testing the temperature sensor many times. ###
	if False:
		for i in range(200):
			test_temp_sensor_specs = dict(scm_port=scm_port,
									temp_port="COM7",
									control_mode='loopback',
									read_mode='uart',
									iterations=500)
			temp_data = test_temp_sensor(**test_temp_sensor_specs)
			ts = time.gmtime()
			datetime = time.strftime("%Y%m%d_%H%M%S",ts)
			write_temp_data(temp_data, './sensor_adc/data/temp_{}_{}.csv'.format(datetime, i))

	# Plotting ADC data
	if False:
		fname = "./sensor_adc/data/psu_20190819_173425_cropped.csv"

		plot_adc_data_specs = dict(adc_outs=read_adc_data(fname),
								plot_inl=False,
								plot_ideal=True,
								vdd=1.2,
								num_bits=10)
		plot_adc_data(**plot_adc_data_specs)


	# Calculating and plotting ADC DNL
	if False:
		fname = "./sensor_adc/data/psu_20190819_173425_cropped.csv"
		calc_adc_dnl_endpoint_specs = dict(adc_outs=read_adc_data(fname))

		DNLs = calc_adc_dnl_endpoint(**calc_adc_dnl_endpoint_specs)
		DNLs_cleaned = [x for x in DNLs if not np.isnan(x)]

		plt.plot(range(len(DNLs)), DNLs)
		plt.xlabel("ADC Code")
		plt.ylabel("DNL (LSBs)")
		plt.grid()
		plt.title("Peak Positive DNL: {0}\nPeak Negative DNL: {1}".format(max(DNLs_cleaned), min(DNLs_cleaned)))
		plt.show()

	# Calculating and plotting ADC INL
	if False:
		fname = "./sensor_adc/data/psu_20190819_173425_cropped.csv"
		calc_adc_inl_endpoint_specs = dict(adc_outs=read_adc_data(fname))

		INLs = calc_adc_inl_endpoint(**calc_adc_inl_endpoint_specs)
		INLs_cleaned = [x for x in INLs if not np.isnan(x)]

		plt.plot(range(len(INLs)), INLs)
		plt.xlabel("ADC Code")
		plt.ylabel("INL (LSBs)")
		plt.grid()
		plt.title("Peak Positive INL: {0}\nPeak Negative INL: {1}".format(max(INLs_cleaned), min(INLs_cleaned)))
		plt.show()

	# Plotting ADC code vs. temperature
	if True:
		fname = "./sensor_adc/data/20191017_0/temp_20191017_gain5.csv"
		temp_data = read_temp_data(fname)
		plot_temp_data_specs = dict(temp_data=temp_data,
				pga_gain="5",
				linefit=True)

		plot_temp_data(**plot_temp_data_specs)
