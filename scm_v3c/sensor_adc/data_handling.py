import csv

def write_adc_data(adc_outs, file_out):
	"""
	Inputs:
		adc_outs: A dictionary of ADC codes where the key is the vin,
			and the value is a list of measured codes (more than one
			measurement can be taken).
		file_out: String. Path to the file to hold the data.
	Outputs:
		No return value. The first column of the output file is 
		the vin values. The second column is the first of all the 
		iterations of the ADC codes measured for that vin.

		vin1 code_1_1 code_1_2 ...
		vin2 code_2_1 code_2_2 ...
		vin3 code_3_1 code_3_2 ...
	"""
	with open(file_out, 'w') as f:
		fwriter = csv.writer(f)
		for vin in adc_outs.keys():
			row = [vin] + adc_outs[vin]
			fwriter.writerow(row)
	return

def read_adc_data(file_in):
	"""
	Inputs:
		file_in: String. Path to the file containing the data. The format
			should have it that the first column is the vin values, and all
			subsequent columns are the ADC output codes for the vin of the
			same row.

			vin1 code_1_1 code_1_2 ...
			vin2 code_2_1 code_2_2 ...
			vin3 code_3_1 code_3_2 ...
	Outputs:
		Returns a dictionary of ADC codes where the key is the vin
			and the value is a list of measured codes.
	Raises:
		ValueError if anything in the input file can't be cast 
		to a float.
	"""
	adc_outs = dict()
	with open(file_in, 'r') as f:
		freader = csv.reader(f)
		for row in freader:
			# Sometimes the thing reads empty rows where there are none
			if len(row) == 0:
				continue
			# Casting call, anyone?
			vin = float(row[0])
			codes = [float(i) for i in row[1:]]
			adc_outs[vin] = codes
	return adc_outs

if __name__ == "__main__":
	### Testing write ###
	if False:
		adc_outs = {i/2:[j*i for j in range(5)] for i in range(3)}
		print(adc_outs)
		write_adc_data(adc_outs, './test_write.csv')

	### Testing write and read ###
	if False:
		fname = './test_write.csv'
		adc_outs = {i/2:[float(j*i) for j in range(5)] for i in range(3)}
		write_adc_data(adc_outs, fname)
		reread_adc_outs = read_adc_data(fname)
		print(adc_outs)
		print(reread_adc_outs)
		print(adc_outs == reread_adc_outs)