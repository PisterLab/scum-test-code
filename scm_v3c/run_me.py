from calibration.LC import *
from calibration.data_handling import *
from bootload import *
import csv

if __name__ == "__main__":
	programmer_port = "COM13"
	scm_port = "COM15"
	
	# Program SCM
	if True:
		program_cortex_specs = dict(teensy_port=programmer_port,
								uart_port=scm_port,
								file_binary="./code.bin",
								boot_mode="optical",
								skip_reset=False,
								insert_CRC=True,
								pad_random_payload=False)

		program_cortex(**program_cortex_specs)


	# LC sweep and measure (preliminary measurements for figuring out
	# how to calibrate)
	if True:
		file_out = './calibration/data/blep.csv'

		test_LC_sweep_specs = dict(uart_port=scm_port,
								num_codes=2048)

		LC_outs = test_LC_sweep(**test_LC_sweep_specs)

		write_LC_data(LC_outs, file_out)