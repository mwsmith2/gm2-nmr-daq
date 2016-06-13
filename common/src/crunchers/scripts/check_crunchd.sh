#!/bin/bash
source /home/newg2/.bashrc
source /home/newg2/.shrc

cd $(dirname $(readlink -f $0))/..

if [ "$(pidof gm2-nmr-crunchd)" ]
then
    echo "gm2-nmr-crunchd is running."
else
    echo "Restarting gm2-nmr-crunchd."
    python scripts/gm2-nmr-crunchd.py ${1} &
fi

# end script