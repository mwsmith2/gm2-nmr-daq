# A program that keeps track of data for you

import os
import sys
import argparse
import hashlib
import json
import uuid

from glob import glob

data = {}
mgr_file = 'test.json'

def main():

    mgr_file = 'test.json'

    try:
        data = json.loads(open(mgr_file).read())

    except:
        data = {}

    init_data(data)

    parser = argparse.ArgumentParser()

    parser.add_argument('cmd', nargs='+', metavar='command', help='add')

    args = parser.parse_args()

    print args

    if args.cmd[0] == 'dataset':
        dataset_add(args.cmd[1:], data)


def init_data(data):
    try:
        data['datasets']

    except:
        data['datasets'] = {}


def save_data(data):
    with open(mgr_file, 'w') as f:
        f.write(json.dumps(data, sort_keys=True, indent=4))

def dataset_add(args, data):

    print "Adding dataset"

    # Make sure we got the name and the path
    if len(args) < 2:
        print "Needs <dataset-name> <dataset-path>"
        sys.exit(1)

    # Load the dataset data
    data['datasets'][args[0]] = {}
    ds = data['datasets'][args[0]]

    ds['path'] = os.path.realpath(args[1])

    datafiles = glob(ds['path'] + '/*')

    ds['files'] = {}

    print ds
    print datafiles

    for datafile in datafiles:
        
        fname = os.path.basename(datafile)
        ds['files'][fname] = {}

        md5sum = hashlib.md5(open(datafile).read()).hexdigest()
        ds['files'][fname]['md5'] = md5sum

    save_data(data)

    
if __name__=='__main__':
    main()
    
    

