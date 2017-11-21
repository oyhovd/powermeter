import sys
import os
import csv
import re
import math
import matplotlib.pyplot as plt

smoothingtime1 = 20
smoothingtime2 = 60
with open(sys.argv[1], 'r') as csvfile:
    plt.figure(1)
    lines = list(csv.reader(csvfile, delimiter=' '))
    values = []
    smoothed_15mins = []
    smoothed_hr = []
    timestamps_hr = []
    timestamp_min = 0
    i = 0
    for line in lines:
        for element in range(1,5):
            timestamp_min += 1
            if line[element] == 'FFFFFFFF':
                continue
            if line[element] == '00000000':
                continue

            value = int(line[element], 16)
            values.append((value*60.0)/1000) #convert to avg kW
            timestamps_hr.append(timestamp_min/60.0)

            #+1 since i is included with +1 below
            start = i - smoothingtime1 + 1
            if i < (smoothingtime1-1):
                start = 0

            smoothed_15mins.append(sum(values[start:i+1])/smoothingtime1)

            #+1 since i is included with +1 below
            start = i - smoothingtime2 + 1
            if i < (smoothingtime2-1):
                start = 0

            smoothed_hr.append(sum(values[start:i+1])/smoothingtime2)
            i += 1

    plt.plot(timestamps_hr, values)
    plt.plot(timestamps_hr, smoothed_15mins)
    plt.plot(timestamps_hr, smoothed_hr)
    plt.legend()
    plt.ylabel('kW')
    plt.xlabel('Time')
    plt.show()
