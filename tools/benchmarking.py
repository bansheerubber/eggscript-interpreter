import argparse
import os
import matplotlib.pyplot as plt
import numpy as np
import scipy.stats
import math
from scipy.stats import norm

parser = argparse.ArgumentParser()
parser.add_argument("--runs", required=False)
parser.add_argument("--file", required=False)
parser.add_argument("--visualize", required=False)
parser.add_argument("--exec", required=False)
args = parser.parse_args()

executable = "../dist/eggscript" if args.exec == None else args.exec

def reject_outliers(data, m=2):
	return data[abs(data - np.median(data)) < m * np.std(data)]

if args.runs != None and args.file != None:
	output = open(f"./benchmarks/{args.file}.results", "w")
	for i in range(0, int(args.runs)):
		time = int(os.popen(f'{executable} ./benchmarks/{args.file} --time | grep -Eo "[0-9]+"').read())
		output.write(str(time) + "\n")
	output.close()
else:
	def read(filename):
		file = open(filename)
		result = []
		for line in file:
			try:
				result.append(int(line))
			except:
				pass
		
		mean, standard_deviation = norm.fit(reject_outliers(np.array(result), 1))

		plt.hist(result, bins="auto", density=True)
		plt.axvline(mean, color='k', linestyle='dashed', linewidth=1)
		plt.xlim(mean - standard_deviation * 7, mean + standard_deviation * 7)
		plt.show()

	read(args.visualize)
