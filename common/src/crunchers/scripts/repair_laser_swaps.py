import sys
import re
import simplejson as json
from collections import OrderedDict
import glob

def main():
    resource_dir = '/home/newg2/Applications/gm2-nmr/resources/'
    attr_file = resource_dir + 'log/run_attributes.json'
    user_runlog_file = resource_dir + 'log/user_runlog.json'
    midas_runlog_file = resource_dir + 'log/midas_runlog.json'

    try:
        run_attr = json.loads(open(attr_file).read(),
                              object_pairs_hook=OrderedDict)
    except:
        print "No attributes to check."
        return 1

    try:
        user_runlog = json.loads(open(user_runlog_file).read(),
                              object_pairs_hook=OrderedDict)

    except:
        user_runlog = json.loads('{}', object_pairs_hook=OrderedDict)

    try:
        midas_runlog = json.loads(open(midas_runlog_file).read(),
                              object_pairs_hook=OrderedDict)

    except:
        midas_runlog = json.loads('{}', object_pairs_hook=OrderedDict)
    
    for key in run_attr.keys():

        runkey = 'run_%05i' % int(key)

        if run_attr[key]['laser_swap'] == True:
            user_runlog[runkey] = midas_runlog[runkey]
            user_runlog[runkey]['laser_swap'] = True

    with open(user_runlog_file, 'w') as f:
        f.write(json.dumps(user_runlog, indent=2))

    return 0

if __name__ == '__main__':
    sys.exit(main())
