#!/bin/bash
source /home/newg2/.bashrc
source /home/newg2/.shrc

cd $(dirname $(readlink -f $0))/..

if [ "$(pidof g2field-crunchd)" ]
then
    echo "g2field-crunchd is running."
else
    echo "Restarting g2field-crunchd."
    python scripts/g2field-crunchd.py
fi

# end script