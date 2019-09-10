import csv
import matplotlib.pyplot as plt
import numpy as np
import scipy as sp

def write_LC_data(LC_outs, file_out):
	"""
	Inputs:
		LC_outs: A dictionary of LC frequencies where the key is the code,
			and the value is a list of measured frequencies (more than one
			measurement can be taken).
		file_out: String. Path to the file to hold the data.
	Outputs:
		No return value. The first column of the output file is 
		the code. The second column is the first of all the 
		iterations of the LC frequencies measured for that code.

		code1 freq1_1 freq_1_2 ...
		code2 freq2_1 freq_2_2 ...
		code3 freq3_1 freq_3_2 ...
	"""
	with open(file_out, 'w') as f:
		fwriter = csv.writer(f)
		for code in LC_outs.keys():
			row = [code] + LC_outs[code]
			fwriter.writerow(row)
	return

def read_LC_data(file_in):
	"""
	Inputs:
		file_in: String. Path to the file containing the data. The format
			should have it that the first column is the code values, and all
			subsequent columns are the frequencies for the code of the
			same row.

			code1 freq1_1 freq_1_2 ...
			code2 freq2_1 freq_2_2 ...
			code3 freq3_1 freq_3_2 ...
	Outputs:
		Returns a dictionary of LC frequencies where the key is the code
			and the value is a list of measured frequencies.
	Raises:
		ValueError if anything in the input file can't be cast 
		to a float.
	"""
	LC_outs = dict()
	with open(file_in, 'r') as f:
		freader = csv.reader(f)
		for row in freader:
			# Sometimes the thing reads empty rows where there are none
			if len(row) == 0:
				continue
			# Casting call, anyone?
			code = row[0]
			frequencies = [float(i.replace("b'", '').replace("\n'",'')) for i in row[1:]]
			LC_outs[code] = frequencies
	return LC_outs