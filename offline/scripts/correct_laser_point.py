#!/bin/python

import os
import sys
import time
import tempfile
import shutil
import subprocess
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
    key = 'Laser Tracker Point = STRING : [32]'

    if correct_point == 'p1':
        print "Capitalizing point to P1."
        correct_point = 'P1'

    if correct_point == 'p2':
        print "Capitalizing point to P2."
        correct_point = 'P2'

    if correct_point not in ('P1', 'P2'):
        print 'Unrecognized point setting to no point, \'N\''
        correct_point = 'N'

    odb_file = resource_dir + '/run%05i.odb' % run_number

    fh, abs_path = tempfile.mkstemp()

    with open(odb_file) as f:

        with open(abs_path, 'w') as tmpfile:

            for line in f:

                if line.find(key) == 0:
                    tmpfile.write('%s %s\n' % (key, correct_point))

                else:
                    tmpfile.write(line)

    os.close(fh)
    os.remove(odb_file)
    shutil.move(abs_path, odb_file)

    subprocess.call(['python', 'scripts/reset_run_data.py', str(run_number)])


if __name__ == '__main__':
    main()

