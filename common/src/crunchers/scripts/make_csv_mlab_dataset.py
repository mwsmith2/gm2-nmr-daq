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
    t = f.Get('t_mlab')
    data = np.empty([2, t.GetEntries()])

    for n in xrange(t.GetEntries()):

        t.GetEntry(n)

        data[0, n] = t.field
        data[1, n] = t.midas_time

    output = 'data/auxilliary/csv/mlab_' + os.path.basename(datafile).replace('root', 'csv')
    np.savetxt(output, data.T, fmt='%.8f')

    return 0

if __name__ == '__main__':
    sys.exit(main())
