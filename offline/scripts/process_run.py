#!/bin/python

import time
import os
from subprocess import call

import midas

def main():
    
    offline_dir = os.path.dirname(__file__) + '/..'
    os.chdir(offline_dir)

    odb = midas.ODB('gm2-nmr')
    run_num = int(odb.get_value("Runinfo/Run Number"))
    run_state = int(odb.get_value("Runinfo/State"))

    if run_state == 1:
        call(['python', 'scripts/crunch_runs.py', str(run_num)])

    else:
        call(['python', 'scripts/crunch_runs.py', str(run_num - 1)])

if __name__ == '__main__':
    main()

