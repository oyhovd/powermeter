import sys
import os
import csv
import re
import math
import matplotlib.pyplot as plt

#Generating two smoothed curves as well, each of these are in minutes to smooth over
smoothingtime1 = 20
smoothingtime2 = 60
smoothingtime3 = 60*24

#use
#  nrfjprog --memrd <address> -n <bytes>
#and write it to a file, pass that file as argument:
#  python3 grapher.py memrd.log
with open(sys.argv[1], 'r') as memdumpfile:
    plt.figure(1)
    lines = list(csv.reader(memdumpfile, delimiter=' '))
    values = []
    smoothed_1 = []
    smoothed_2 = []
    smoothed_3 = []
    timestamps_hr = []
    timestamp_min = 0
    i = 0
    for line in lines:
        for element in range(1,5):
            if line[element] == 'FFFFFFFF':
                continue
            if line[element] == '00000000':
                continue

            timestamp_min += 1
            value = int(line[element], 16)
            values.append((value*60.0)/1000) #convert to avg kW, assuming 1000 blinks/kWh
            timestamps_hr.append(timestamp_min/60.0)

            #Calculate smoothed curves
            i += 1
            start = i - smoothingtime1
            if start >= 0:
                smoothed_1.append(sum(values[start:i])/smoothingtime1)

            start = i - smoothingtime2
            if start >= 0:
                smoothed_2.append(sum(values[start:i])/smoothingtime2)

            start = i - smoothingtime3
            if start >= 0:
                smoothed_3.append(sum(values[start:i])/smoothingtime3)


    plt.plot(timestamps_hr, values)
    plt.plot(timestamps_hr[smoothingtime1-1:], smoothed_1)
    plt.plot(timestamps_hr[smoothingtime2-1:], smoothed_2)
    plt.plot(timestamps_hr[smoothingtime3-1:], smoothed_3)
    plt.ylabel('kW')
    plt.xlabel('Time')
    plt.show()
