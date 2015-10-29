#!/usr/bin/python

import os
import sys
import json
import glob
import subprocess
import hashlib


def main():

    offline_dir = os.path.dirname(__file__) + '/..'
    os.chdir(offline_dir)

    info_file = 'data/crunched/.processing_metadata.json'    

    runs = []

    for i in sys.argv[1:]:
        runs.append(int(i))

    runs.sort()

    info = json.loads(open(info_file).read())
    info['last_full_crunch'] = runs[0]
    info = open(info_file, 'w').write(json.dumps(info))

    for run in runs:
        info_file = 'data/rome/.processing_metadata.json'

        info = json.loads(open(info_file).read())
        try:
            del info[str(run)]
        except:
            pass

        with open(info_file, 'w') as f:
            f.write(json.dumps(info, indent=2, sort_keys=True))

        info_file = 'data/shim/.processing_metadata.json'

        info = json.loads(open(info_file).read())
        try:
            del info[str(run)]
        except:
            pass

        with open(info_file, 'w') as f:
            f.write(json.dumps(info, indent=2, sort_keys=True))

        info_file = 'data/crunched/.processing_metadata.json'

        info = json.loads(open(info_file).read())
        try:
            del info[str(run)]
        except:
            pass

        with open(info_file, 'w') as f:
            f.write(json.dumps(info, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
