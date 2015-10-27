#!/bin/python

import time
import os
from subprocess import call

import midas

def main():
    
    offline_dir = os.path.dirname(__file__) + '/..'
    os.chdir(offline_dir)

    odb = midas.ODB('gm2-nmr')
    run_num = odb.get_value("Runinfo/Run Number")

    call(['python', 'scripts/crunch_runs.py', run_num])


if __name__ == '__main__':
    main()

