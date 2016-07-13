import sys
import os
from copy import copy

import numpy as np
import ROOT as rt
from scipy.interpolate import interp1d

def main():

    # full_scan_file = 'data/bundles/full_scan_048.root'
    # full_scan_file = 'data/bundles/full_scan_048.root'

    # Grab the full scan for the base.
    full_scan_file = sys.argv[1]

    # And the runs to splice in.
    splice_files = []

    for fname in sys.argv[2:]:
        if fname.endswith('.root'):
            splice_files.append(fname)
        else:
            splice_files.append('data/crunched/run_%05i.root' % int(fname))

    print splice_files

    f = rt.TFile(full_scan_file)
    t = f.Get('t_sync')
    data = np.empty([29, t.GetEntries()])

    bad_laser_points = []

    if '047' in full_scan_file:
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

            data[0, n] = (data[0, n]) % 360

            t.GetEntry(n)

        else:
            data[0, n] = (t.phi_2 - 1.36) % 360

        for i in xrange(28):

            data[i + 1, n] = t.freq[i]

    # Now splice in subsequent runs.
    for datafile in splice_files:

        f = rt.TFile(datafile)
        t = f.Get('t_sync')
        new_data = np.empty([29, t.GetEntries()])

        for n in xrange(t.GetEntries()):

            t.GetEntry(n)

            if n in bad_laser_points:
                t.GetEntry(n - 1)
                new_data[0, n] = 0.5 * (t.phi_2 - 1.36)

                t.GetEntry(n + 1)
                new_data[0, n] += 0.5 * (t.phi_2 - 1.36)

                new_data[0, n] = new_data[0, n] % 360

                t.GetEntry(n)

            else:
                new_data[0, n] = (t.phi_2 - 1.36) % 360

            for i in xrange(28):

                new_data[i + 1, n] = t.freq[i]

        # Replace the old data.
        phi_min = new_data[0, 0]
        phi_max = new_data[0, t.GetEntries() - 1]

        old_data = copy(data)

        # Make phi for convenience.
        phi = old_data[0, :]

        # Find where the new data will replace the old.
        dphi = (phi_max - phi_min) % 360.0
        idx = np.where((phi - phi_min) % 360.0 > dphi)[0]
        data = np.empty([29, t.GetEntries() + len(idx)])

        # Calculate values to negate drift
        b_i = interp1d(phi, old_data[1, :])(phi_min)
        b_offset = new_data[1, 0] - b_i

        new_data[1:, :] -= b_offset

        data[:, :t.GetEntries()] = new_data
        data[:, t.GetEntries():] = old_data[:, idx]

        data = data[:, data[0, :].argsort()]

    output = 'data/auxilliary/csv/spliced_'
    output += os.path.basename(full_scan_file).replace('root', 'csv')
    np.savetxt(output, data.T, fmt='%.6f')

    return 0

if __name__ == '__main__':
    sys.exit(main())
