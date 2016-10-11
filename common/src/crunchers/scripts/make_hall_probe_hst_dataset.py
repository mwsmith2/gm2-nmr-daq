#!/bin/python

import sys
import os
import sh
import time
import numpy as np

def main():

    # Move into the data directory.
    os.chdir('/home/newg2/Applications/gm2-nmr/resources')

    # Timestamp, voltage, azimuthal tilt, radial tilt
    hall_probe_data = np.zeros([90, 4])

    hstfile = time.strftime('history/%y%m%d.hst')
    hst_dump = sh.mhdump('-n', '-t', '-E', '15', hstfile)
    hst_data = hst_dump.split('\n')[::-1]
    for i in xrange(90):
        data = hst_data[i + 1].split() # Skip in case of empty line
        hall_probe_data[i, 0] = int(data[1])
        hall_probe_data[i, 1] = float(data[3])

    hst_dump = sh.mhdump('-n', '-t', '-E', '13', hstfile)
    hst_data = hst_dump.split('\n')[::-1]
    for i in xrange(90):
        data = hst_data[i + 1].split() # Skip in case of empty line
        hall_probe_data[i, 2] = int(data[3])
        hall_probe_data[i, 3] = int(data[4])

    fname = 'data/auxilliary/csv/hall_probe_dataset_'
    fname += time.strftime('%Y%m%d_%H%M%S.csv')
    np.savetxt(fname, hall_probe_data, fmt='%i, %.6e, %i, %i')

    return 0

if __name__ == '__main__':
    sys.exit(main())
