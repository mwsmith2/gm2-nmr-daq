import sys
import json
import subprocess
import hashlib
import zmq
import numpy as np
import copy
import glob
import midas


def main():

    # Create the ZMQ sockets that talk to the job scheduler.
    ctx = zmq.Context()
    job_sck = ctx.socket(zmq.PUSH)
    job_sck.connect('tcp://127.0.1.1:44444')

    req_sck = ctx.socket(zmq.REQ)
    req_sck.connect('tcp://127.0.1.1:44446')

    # Grab the data directory from the experiment's ODB.
    odb = midas.ODB('gm2-nmr')
#    datadir = odb.get_value('/Logger/Data dir').rstrip() + '/'
    datadir = '/home/newg2/data/'

    if len(sys.argv) < 2:
        print "Insufficient arguments."
        print "usage: python update_full_scans.py <command> [runs ...]"
        sys.exit(1)

    if sys.argv[1] == 'add':
        print "Adding new full scan."

        run_num = 1
        runfile_glob = datadir + '/bundles/run_list_full_scan*%03i.txt'

        while len(glob.glob(runfile_glob % run_num)):
            run_num += 1

        if len(sys.argv) < 3:
            print 'Cannot create a full scan with no runs.'
            sys.exit(1)

        runs = []
        for i in range(2, len(sys.argv)):
            runs.append(int(sys.argv[i]))

        print 'Next full scan is run number %i.' % run_num
        print 'Writing the run list.'

        runfile = datadir + '/bundles/run_list_full_scan_%03i.txt'
        runfile = runfile % run_num

        with open(runfile, 'w') as f:
            runlist = [str(run) for run in runs]
            f.write(' '.join(runlist) + '\n')

        print 'The run list is complete. To process the full scan now, type:'
        print ''
        print 'python scripts/full_scan_helper.py update %i' % run_num
        print ''

    elif sys.argv[1] == 'update':
        print "Rebundling full scans."

        runfiles = []

        # If runs were supplied, check those.
        if len(sys.argv) > 2:
            for i in range(2, len(sys.argv)):
                run = int(sys.argv[i])
                runfile = datadir + 'bundles/run_list_full_scan_%03i.txt' % run
                runfiles.append(runfile)

        # If not, check all of them.
        else:
            runfiles = glob.glob(datadir + 'bundles/run_list*.txt')

        runfiles.sort()
        runfiles.reverse()

        for runfile in runfiles:
            scan_idx = int(runfile[-7:-4])
            runlist = np.genfromtxt(runfile, dtype=np.int).tolist()

            # Submit all the jobs that the full scan depends on.
            for run in runlist:
                job_sck.send_json({'type': 'normal', 'run': run})

            print 'Ensuring all prerequisite jobs have run.'

            while len(runlist) != 0:

                tmplist = copy.copy(runlist)
                print '%i job completions unverified.' % len(tmplist)
                print tmplist

                for run in runlist:
                    req = {}
                    req['type'] = 'check'
                    req['body'] = {}
                    req['body']['type'] = 'normal'
                    req['body']['msg'] = {'run': run, 'type':'normal'}
                    req_sck.send_json(req)
                    msg = req_sck.recv_json()

                    if msg['result'] == False:
                        runlist.remove(run)

            job_sck.send_json({'type': 'bundle-full_scan', 'run': scan_idx})


    else:
        print "Unrecognized argument."
        print "usage: python update_full_scans.py <command> [runs ...]"


if __name__ == '__main__':
    main()
