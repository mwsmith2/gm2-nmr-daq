import sys
import os

import numpy as np
import ROOT as rt

def main():

    try:
        run_type = 'run'
        run_number = int(sys.argv[1])

    except(ValueError):
        run_type = sys.argv[1]
        run_number = int(sys.argv[2])

    if run_type == 'run':
        datafile = 'data/crunched/run_%05i.root' % run_number

    else:
        datafile = 'data/bundles/%s_%03i.root' % (run_type, run_number)

    f = rt.TFile(datafile)
    t = f.Get('t_sync')
    data = np.empty([26, t.GetEntries()])

    bad_laser_points = []

    if datafile == 'data/bundles/full_scan_047.root':
        bad_laser_points.append(167)
        bad_laser_points.append(1766)
        bad_laser_points.append(5932)

    for n in xrange(t.GetEntries()):

        t.GetEntry(n)

        if n in bad_laser_points:
            t.GetEntry(n - 1)
            data[0, n] = 0.5 * (t.phi_2 - 1.36)

            t.GetEntry(n + 1)
            data[0, n] += 0.5 * (t.phi_2 - 1.36)

            data[0, n] = data[0, n] % 360

            t.GetEntry(n)

        else:
            data[0, n] = (t.phi_2 - 1.36) % 360

        for i in xrange(25):

            data[i + 1, n] = t.freq[i]

    output = 'data/auxilliary/csv/freq_nfix_' + os.path.basename(datafile).replace('root', 'csv')
    np.savetxt(output, data.T, fmt='%.6f')

    return 0

if __name__ == '__main__':
    sys.exit(main())
