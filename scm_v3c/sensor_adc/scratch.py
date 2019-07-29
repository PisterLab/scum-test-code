from adc import *
from data_handling import *

if __name__ == "__main__":
	### YOUR CODE HERE ###
	if True:
		file_noise = './data/psu_run_noise.csv'
		adc_noise = read_adc_data(file_noise)

		for vin in adc_noise.keys():
			plt.figure()
			plt.hist(adc_noise[vin], bins=100)
			plt.title("Vin={} V".format(vin))
			plt.xlabel("Code")
			plt.ylabel("Occurrences")
			plt.show()