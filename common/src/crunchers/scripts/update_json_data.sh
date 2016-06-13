#!/bin/bash
source /home/newg2/.bashrc
source /home/newg2/.shrc

cd $(dirname $(readlink -f $0))/..

python scripts/update_json_data.py