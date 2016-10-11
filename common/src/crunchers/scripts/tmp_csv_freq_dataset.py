import sys
import os

import numpy as np
import ROOT as rt

def main():

    datafile = 'data/spliced/full_scan_047.root'

    f = rt.TFile(datafile)
    t = f.Get('t')
    data = np.empty([26, t.GetEntries()])

    bad_laser_points = []

    for n in xrange(t.GetEntries()):

        t.GetEntry(n)

        if n in bad_laser_points:
            t.GetEntry(n - 1)
            data[0, n] = 0.5 * t.phi

            t.GetEntry(n + 1)
            data[0, n] += 0.5 * t.phi

            data[0, n] = data[0, n] % 360

            t.GetEntry(n)

        else:
            data[0, n] = t.phi % 360

        for i in xrange(25):

            data[i + 1, n] = t.freq[i]

    output = 'data/auxilliary/csv/freq_spliced' + os.path.basename(datafile).replace('root', 'csv')
    np.savetxt(output, data.T, fmt='%.6f')

    return 0

if __name__ == '__main__':
    sys.exit(main())
