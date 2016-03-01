#!/bin/python

import os
import sys
import time
import tempfile
import shutil
import subprocess
import json
import midas


def main():
    
    file_dir = os.path.realpath(os.path.dirname(__file__))
    resource_dir = os.path.realpath(file_dir + '/../../resources/history/')
    offline_dir = os.path.realpath(file_dir + '/../')
    os.chdir(offline_dir)

    if len(sys.argv) < 3:
        print "Insufficient arguments. usage:"
        print "python scripts/correct_laser_point.py <run-number> <correct-point>"
        sys.exit(1)
    
    run_number = int(sys.argv[1])
    correct_point = sys.argv[2]

    if correct_point == 'p2':
        print "Capitalizing point to P2."
        correct_point = 'P2'

    if correct_point == 'p1':
        print "Capitalizing point to P1."
        correct_point = 'P1'

    if correct_point not in ('P1', 'P2'):
        print 'Unrecognized point setting to no point, \'N\''
        correct_point = 'N'

    with open('../resources/log/run_attributes.json') as f:

        run_attr = json.loads(f.read())

        old_point = run_attr['%05i' % run_number]['laser_point']
        old_swap = run_attr['%05i' % run_number]['laser_swap']

        run_attr['%05i' % run_number]['laser_point'] = correct_point
        correct_swap = False

        if correct_point == 'P1' and old_point == 'P2':
            correct_swap = True

        if correct_point == 'P2' and old_point == 'P1':
            correct_swap = True

        run_attr['%05i' % run_number]['laser_swap'] = correct_swap

    with open('../resources/log/run_attributes.json', 'w') as f:
        f.write(json.dumps(run_attr, indent=2, sort_keys=True))

    with open('../resources/log/attribute_changes.json', 'a') as f:
        args = (old_point, old_swap, correct_point, correct_swap)
        f.write('(%s, %s) -> (%s, %s)\n' % args)

    # This script no longer works.
    # subprocess.call(['python', 'scripts/reset_run_data.py', str(run_number)])
    

if __name__ == '__main__':
    main()

