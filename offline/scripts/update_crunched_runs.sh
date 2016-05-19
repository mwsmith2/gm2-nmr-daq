#!/bin/bash

# Ensures that crunched runs are up-to-date.  Runs every 10 minutes.

source /home/newg2/.bashrc
source /home/newg2/.shrc

cd $(dirname $(readlink -f $0))/..
python scripts/crunchd_helper.py update

# End script.