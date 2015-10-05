#!/bin/bash

# Load the experiment variables.
source $(dirname $(readlink -f $0))/../../common/.expt-env

for fe in "${EXPT_FE[@]}"; do
    fename="${EXPT_DIR}/common/bin/${fe}"
    scname="${EXPT}.${fe//_/-}"
    screen -dmS $scname
    screen -S $scname -p 0 -rX stuff "${fename} -e $EXPT$(printf \\r)"
done

for fe in "${EXT_FE[@]}"; do
    fe_arr=(${fe//\:/ })
    fename="${fe_arr[1]}"
    scname="${EXPT}.${fe_arr[1]//_/-}"
    #screen -dmS $scname
    cmd="screen -S $scname -p 0 -rX stuff \"${fename} -e $EXPT"
    ssh "${fe_arr[0]}" "\'bash -c \'$cmd\$(printf \\r)\"\'\'"
done

# end script
