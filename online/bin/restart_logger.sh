#!/bin/bash
# The script starts general midas utilites for sis_wfd experiment.

source $(dirname $(readlink -f $0))/../../common/.expt-env

# The script kills general midas utilies.
for session in $(screen -ls | grep -o "[0-9]*\.${EXPT}.mlogger")
do screen -S "${session}" -p 0 -X quit
done

cmd="mlogger -e $EXPT$(printf \\r)"
screen -dmS "${EXPT}.mlogger"
screen -S "${EXPT}.mlogger" -p 0 -rX stuff "$cmd"

unset cmd

# end script

