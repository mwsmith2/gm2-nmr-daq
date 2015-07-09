#!/bin/bash
# The script starts general midas utilites for simple_daq experiment.

source /home/newg2/Applications/gm2-nmr/common/.expt-env

source $EXPT_DIR/online/bin/kill_analyzers.sh
source $EXPT_DIR/online/bin/kill_frontends.sh
source $EXPT_DIR/online/bin/kill_midas.sh

# end script