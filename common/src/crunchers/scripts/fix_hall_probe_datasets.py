#!/bin/python

import sys
import os
import sh
import time
import numpy as np
from glob import glob

def main():

    # Move into the data directory.
    os.chdir('/home/newg2/Applications/gm2-nmr/resources')

    for hp_file in glob('data/auxilliary/csv/hall_probe_dataset*'):

        # Change to Timestamp, voltage, azimuthal tilt, radial tilt
        hall_probe_data = np.genfromtxt(hp_file, delimiter=',')

        # Load the history file from that day.
        hstfile = 'history/%s.hst' % hp_file.split('_')[-2][2:]
        hst_dump = sh.mhdump('-n', '-t', '-E', '13', hstfile)
        hst_data = hst_dump.split('\n')[::-1]
        print hst_data[0]

        for i in xrange(hall_probe_data.shape[0]):
            # Find the corresponding entry by timestamp.
            for entry in hst_data:
                if entry.startswith('13 ' + str(int(hall_probe_data[i, 0]))):
                    data = entry.split()
                    hall_probe_data[i, 2] = int(data[3])
                    hall_probe_data[i, 3] = int(data[4])

        np.savetxt(hp_file, hall_probe_data, fmt='%i, %.6e, %i, %i')

    return 0

if __name__ == '__main__':
    sys.exit(main())
