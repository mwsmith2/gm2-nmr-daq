#!/usr/bin/python
import os
import sys

from collections import OrderedDict
import simplejson as json

import midas

def main():
    """Loads an update from json buffer in midas."""
    odb = midas.ODB('gm2-nmr')

    key = odb.get_value('Custom/Data/json-buffer/key').rstrip()
    fdir = odb.get_value('Custom/Path').rstrip()
    fname = os.path.join(fdir, odb.get_value('Custom/Data/json-buffer/file').rstrip())
    value = odb.get_value('Custom/Data/json-buffer/value').rstrip()

    if key == '' or value == '':
        return 0

    if os.path.exists(fname):
        json_data = json.loads(open(fname).read(),
                               object_pairs_hook=OrderedDict))
                               

    else:
        json_data = {}

    json_data[key] = json.loads(value, object_pairs_hook=OrderedDict)

    f = open(fname, 'w')
    f.write(json.dumps(json_data, indent=2))
    f.close()

    key = odb.set_value('Custom/Data/json-buffer/key', '')
    fname = odb.set_value('Custom/Data/json-buffer/file', '')
    value = odb.set_value('Custom/Data/json-buffer/value', '')

    return 0

if __name__=='__main__':
    sys.exit(main())
    
