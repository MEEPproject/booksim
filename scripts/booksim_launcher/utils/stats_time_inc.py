#!/usr/bin/python3

# Copyright (c) 2014-2020, University of Cantabria
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Author: Ivan Perez

# This script includes the simulation times, saved in err files, in the CSV
# stat files.

# Usage: python3 util/stats_time_inc.py <list of files>

import sys
import csv

def CSVAddExtraColumn(csv_file, data_values):
	# Output CSV name
	outcsv = csv_file.replace(".csv",".csv_time")

	# TODO: support for mulitple packet classes
	with open(csv_file, 'r') as csvfile:
		reader = csv.DictReader(csvfile, delimiter=',')

		fieldnames = reader.fieldnames
		
		# Abort if the number of elements in the columns and the time list is
		# different
		#print(reader)
		#if len(data_values) != len(reader):
		#	print("ERROR: number of columns does not match")
		#	raise NameError("NumCols")

		# Write result in CSV file
		with open(outcsv, "w") as outcsvfile:
			fieldnames.append("runtime")
			writer = csv.DictWriter(outcsvfile, fieldnames=fieldnames)

			writer.writeheader()
			for indx, row in enumerate(reader):
				new_row = row
				new_row["runtime"] = data_values[indx]
				writer.writerow(new_row)

def ReadTimes(filename):
	# String of interest
	str_oi = "Total run time " 

	# Open error file
	with open(filename,'r') as errorfile:
		time_sec = list()
		for line in errorfile.readlines():
			# Check that the line contains the simulation time.
			if str_oi in line:
				# Read value, convert and append it
				time_sec.append(float(line.replace(str_oi,"")))
			# Skip this file if there is another message
			else:
				import warnings
				warnings.simplefiler("File {} has the following error: {}"\
						.format(filename, line))
		return time_sec



def main():
	file_list = sys.argv[1::]
	print("Time stats includer. Files to process:")
	for ind, item in enumerate(file_list):
		print("{}: {}".format(ind+1, item))

	for file_indx, file_item in enumerate(file_list):
		print("Processing file nº {}: {}".format(
											file_indx,
											file_item
											)
			)
	
		# Read times from error
		error_file = file_item.replace(".csv",".err")
		data_values = ReadTimes(error_file)
	
		# Add times to CSV
		CSVAddExtraColumn(file_item, data_values)


if __name__=="__main__":
	main()
