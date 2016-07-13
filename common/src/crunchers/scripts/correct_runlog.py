#!/usr/bin/python

import sys
from collections import OrderedDict
import re
import simplejson as json
import glob
import midas

def main():

    midas_attr_file = '/home/newg2/Applications/gm2-nmr/resources/log/midas_runlog.json'
    user_attr_file = '/home/newg2/Applications/gm2-nmr/resources/log/user_runlog.json'
    arch_dir = '/home/newg2/Applications/gm2-nmr/resources/history'

    # Load the MIDAS runlog.
    try:
        midas_attr = json.loads(open(midas_attr_file).read(),
                              object_pairs_hook=OrderedDict)

    except:
        midas_attr = json.loads('{}', object_pairs_hook=OrderedDict)

    # Load the user corrected runlog.
    try:
        user_attr = json.loads(open(user_attr_file).read(),
                              object_pairs_hook=OrderedDict)

    except:
        user_attr = json.loads('{}', object_pairs_hook=OrderedDict)

    if len(sys.argv) < 4:
        print 'Usage: python correct_runlog.py <key> <value> [runs ...]'
        sys.exit(1)

    # Grab the values to update.
    key = sys.argv[1]
    value = sys.argv[2]

    # Iterate over all runs specified.
    for run_num in sys.argv[3:]:
        run_key = "run_%05i" % int(run_num)

        if run_key in user_attr.keys():
            user_attr[run_key][key] = value

        else:
            user_attr[run_key] = midas_attr[run_key]
            user_attr[run_key][key] = value

        print json.dumps(user_attr[run_key], indent=2)

    with open(user_attr_file, 'w') as f:
        f.write(json.dumps(user_attr, indent=2))


if __name__ == '__main__':
    sys.exit(main())
