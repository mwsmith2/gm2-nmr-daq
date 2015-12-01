#!/bin/bash

# This script ensures that full_scans are up-to-date.

source /home/newg2/.bashrc
source /home/newg2/.shrc

cd $(dirname $(readlink -f $0))/..
python scripts/full_scan_helper.py update

# End script.