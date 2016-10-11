import sys
import os
import ROOT as rt

def main():
    """Simply print the angular range contained in the run."""

    for f in sys.argv[1:]:

        rootfile = rt.TFile(f)

        if rootfile.GetListOfKeys().Contains('t'):
            t = rootfile.Get('t')
        else:
            t = rootfile.Get('t_sync')

        t.GetEntry(0)
        phi_min = t.phi_2

        t.GetEntry(t.GetEntries() - 1)
        phi_max = t.phi_2

        print os.path.basename(f), phi_min, phi_max

    return 0


if __name__ == '__main__':
    sys.exit(main())
