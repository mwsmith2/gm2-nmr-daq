#!/bin/bash

# Load the experiment variables.
source /home/newg2/Applications/gm2-nmr/common/.expt-env

for fename in "${EXPT_FE[@]}"; do
    scname="${EXPT}.${fename//_/-}"
    screen -dmS $scname
    screen -S $scname -p 0 -rX stuff "${fename} -e $EXPT$(printf \\r)"
done

# end script
