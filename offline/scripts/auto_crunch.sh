#!/bin/bash
source /home/newg2/.bashrc

cd $(dirname $(readlink -f $0))/..
python scripts/crunch_runs.py

# end script