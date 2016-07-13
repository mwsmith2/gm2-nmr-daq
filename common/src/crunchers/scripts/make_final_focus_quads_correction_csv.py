import sys
import os
from copy import copy

import numpy as np
import ROOT as rt

def beam_quad_field(phi):
        phi= (phi - 335.0 + 180.0) % 360 - 180.0
        return -4 * 61.79e-3 * np.exp(-(phi / 20.0)**2)

def main():

    # Grab the full scan for the base.
    datafile = sys.argv[1]

    ref_data = np.genfromtxt(datafile)
    data = np.empty([2, ref_data.shape[0]])

    for n, entry in enumerate(ref_data):
        print entry
        data[0, n] = entry[0]


    data[1, :] = beam_quad_field(data[0, :])

    output = 'data/auxilliary/csv/beamline_quad_correction_'
    output += os.path.basename(datafile)
    np.savetxt(output, data.T, fmt='%.6f')

    return 0

if __name__ == '__main__':
    sys.exit(main())
