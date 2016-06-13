#!/usr/bin/python

import os
import sys
import json
import glob
import time
import copy
import logging as log
import hashlib

from subprocess import call, Popen

import numpy as np
import setproctitle
import zmq
import gevent
import midas

# Create a zmq context, global since you only need one.
context = zmq.Context()
odb = midas.ODB('gm2-nmr')
nworkers = 4
maxcount = 20
datadir = odb.get_value('/Logger/Data dir').rstrip()
offline_dir = os.path.abspath(os.path.dirname(__file__) + '/..')

# Set up the zmq socket for data.
crunch_sck = context.socket(zmq.PULL)
crunch_sck.bind('tcp://127.0.1.1:44444')

# Set up the zmq socket for logging.
logger_sck = context.socket(zmq.PULL)
logger_sck.bind('tcp://127.0.1.1:44445')

# Worker socket to send job results.
worker_sck = context.socket(zmq.PUSH)
worker_sck.connect('tcp://127.0.1.1:44445')

# Socket to query the scheduler.
status_sck = context.socket(zmq.REP)
status_sck.bind('tcp://127.0.1.1:44446')

job_queue = {}
req_queue = []
msg_queue = []
log_queue = []
workers = []


def main():

    # Set the process name, so that we check if it's running.
    setproctitle.setproctitle('gm2-nmr-crunchd')

    # Move into the offline directory.
    os.chdir(offline_dir)

    # Configure the log file.
    if len(sys.argv) > 1:
        log.basicConfig(filename=sys.argv[1],
                        format='[%(asctime)s]: %(levelname)s, %(message)s',
                        level=log.INFO)

    else:
        log.basicConfig(filename='crunchd.log',
                        format='[%(asctime)s]: %(levelname)s, %(message)s',
                        level=log.INFO)

    log.info('gm2-nmr-crunchd has started as process %i' % os.getpid())

    polltime = 20
        
    # Now process jobs as they come.
    while True:
        jobs = []

        jobs.append(gevent.spawn(crunch_sck.poll, timeout=polltime))
        jobs.append(gevent.spawn(logger_sck.poll, timeout=polltime))
        jobs.append(gevent.spawn(status_sck.poll, timeout=polltime))
        gevent.joinall(jobs)
        
        # Put number of messages into an array.
        nevents = [job.value for job in jobs]

        # Get all messages for each socket.
        if nevents[0]:
            while (crunch_sck.poll(timeout=polltime)):
                msg_queue.append(crunch_sck.recv_json())
        
        if nevents[1]:
            while (logger_sck.poll(timeout=polltime)):
                log_queue.append(logger_sck.recv_json())

        if nevents[2]:
            while (status_sck.poll(timeout=polltime)):
                try:
                    req_queue.append(status_sck.recv_json())

                except(zmq.error.ZMQError):
                    continue

        if len(log_queue) == 0 and len(job_queue) == 0 and len(msg_queue) == 0:
            time.sleep(1.0)

        # Start working on some of the jobs.
        log.debug('running parse_jobs()')
        parse_jobs()

        log.debug('running spawn_jobs()')
        spawn_jobs()

        log.debug('running write_reps()')
        write_reps()

        log.debug('running write_logs()')
        write_logs()


def parse_jobs():
    global msg_queue
    global job_queue
    global maxcount

    # Process a set.
    for count, msg in enumerate(msg_queue):

        if msg['type'] == 'standard':

            if msg['cmd'] == 'crunch':
                try:
                    job_queue[msg['run']]

                except(KeyError):
                    job_queue[msg['run']] = []
            
                for jobs in standard_job_set(msg):
                    job_queue[msg['run']].append(jobs)

            elif msg['cmd'] == 'reset':

                for job in standard_job_set(msg):
                    # Update the metadata just in case.
                    msg = {}
                    msg['cmd'] = 'reset'
                    msg['info'] = job['meta']
                    msg['run'] = str(run_num).zfill(5)
                    msg['log'] = {}
                    msg['log'][job['name']] = {'attempted': True}
                    msg['log'][job['name']]['deps'] = job['deps']
                    worker_sck.send(json.dumps(msg))

        if msg['type'].startswith('bundle'):

            try:
                job_queue[msg['run']]

            except(KeyError):
                job_queue[msg['run']] = []
            
            for jobs in bundle_job_set(msg):
                job_queue[msg['run']].append(jobs)

        # Remove the processed message.
        msg_queue.remove(msg)

        # Break afer a max count of messages.
        if count >= maxcount:
            break


def standard_job_set(msg):
    """Generate the set of jobs associated with standard runs."""

    run_num = msg['run']
    jobs = [[], [], [], [], []]
    new_dep = {'time': None, 'md5': None}
        
    # Add ROME jobs first
    cmd_prefix = "./midanalyzer.exe -b -i romeConfig.xml -r "
    cmd_suffix = " -m offline -p 0 -q"
    rome_dir = offline_dir + '/rome-processors'

    job = {}
    job['meta'] = datadir + '/shim/.crunchd_metadata.json'
    job['cmd'] = cmd_prefix + str(run_num) + cmd_suffix
    job['clean'] = 'rm histos*.root run*.root'

    if run_num < 787:
        job['dir'] = rome_dir + '/single-laser'
        job['name'] = 'laser'

    else:
        job['dir'] = rome_dir + '/double-laser'
        job['name'] = 'laser'

    job['deps'] = {}
    job['deps'][job['dir'] + '/midanalyzer.exe'] = new_dep
    jobs[0].append(job)

    job = copy.copy(job)
    job['name'] = 'capacitec'
    job['dir'] = rome_dir + '/capacitec'
    job['deps'] = {}
    job['deps'][job['dir'] + '/midanalyzer.exe'] = new_dep
    jobs[0].append(job)

    job = copy.copy(job)
    job['name'] = 'shim-platform'
    job['dir'] = rome_dir + '/shim-platform'
    job['deps'] = {}
    job['deps'][job['dir'] + '/midanalyzer.exe'] = new_dep
    jobs[0].append(job)
    
    job = copy.copy(job)
    job['name'] = 'metrolab'
    job['dir'] = rome_dir + '/metrolab'
    job['deps'] = {}
    job['deps'][job['dir'] + '/midanalyzer.exe'] = new_dep
    jobs[0].append(job)
    
    job = copy.copy(job)
    job['name'] = 'mscb-cart'
    job['dir'] = rome_dir + '/mscb-cart'
    job['deps'] = {}
    job['deps'][job['dir'] + '/midanalyzer.exe'] = new_dep
    jobs[0].append(job)

    job = copy.copy(job)
    job['name'] = 'mscb-ring'
    job['dir'] = rome_dir + '/mscb-ring'
    job['deps'] = {}
    job['deps'][job['dir'] + '/midanalyzer.exe'] = new_dep
    jobs[0].append(job)

    job = copy.copy(job)
    job['name'] = 'tilt-sensor'
    job['dir'] = rome_dir + '/tilt-sensor'
    job['deps'] = {}
    job['deps'][job['dir'] + '/midanalyzer.exe'] = new_dep
    jobs[0].append(job)

    job = copy.copy(job)
    job['name'] = 'hall-probe'
    job['dir'] = rome_dir + '/hall-probe'
    job['deps'] = {}
    job['deps'][job['dir'] + '/midanalyzer.exe'] = new_dep
    jobs[0].append(job)
    
    # Make sure run attributes are extracted.
    job = {}
    job['name'] = 'extract_run_attr'
    job['dir'] = offline_dir
    job['cmd'] = 'python scripts/extract_run_attr.py %i' % run_num
    job['clean'] = None
    job['meta'] = datadir + '/crunched/.crunchd_metadata.json'
    job['deps'] = {}
    jobs[0].append(job)
    
    # Now the data bundling job.
    job = {}
    job['name'] = 'make-shim-dataset'
    job['cmd'] = 'bin/make_shim_dataset %i' % run_num
    job['clean'] = None
    job['dir'] = offline_dir
    job['meta'] = datadir + '/shim/.crunchd_metadata.json'
    job['deps'] = {}
    job['deps'][offline_dir + '/bin/make_shim_dataset'] = new_dep
    job['deps']['data/shim/*%05i.root' % run_num] = new_dep
    jobs[1].append(job)

    # Finally apply fixes.
    # job = {}
    # job['name'] = 'fix-probe-remap'
    # job['dir'] = offline_dir
    # job['cmd'] = 'bin/fix_run_probe_map '
    # job['cmd'] += 'data/crunched/run_%05i.root ' % run_num
    # job['cmd'] += 'data/crunched/ %i' % run_num
    # job['clean'] = None
    # job['meta'] = datadir + '/crunched/.crunchd_metadata.json'
    # job['deps'] = {}
    # job['deps'][offline_dir + '/bin/recrunch_fids'] = new_dep
    # job['deps'][datadir + '/shim/run_%05i.root' % run_num] = new_dep
    # jobs[2].append(job)

    # Automatically generate extracted dataset
    job = {}
    job['name'] = 'extraction'
    job['dir'] = offline_dir
    job['cmd'] = 'bin/make_extracted_dataset '
    job['cmd'] += 'data/crunched/run_%05i.root' % run_num
    job['clean'] = None
    job['meta'] = datadir + '/extracted/.crunchd_metadata.json'
    job['deps'] = {}
    job['deps'][offline_dir + '/bin/make_extracted_dataset'] = new_dep
    job['deps'][datadir + '/crunched/run_%05i.root' % run_num] = new_dep
    jobs[3].append(job)

    return jobs

def bundle_job_set(msg):

    bundle_num = msg['run']
    bundle_type = msg['type'].replace('bundle-', '')

    bundle_info = (bundle_type, bundle_num)

    jobs = [[]]
    new_dep = {'time': None, 'md5': None}

    job = {}
    job['name'] = 'make-%s' % msg['type']
    job['cmd'] = 'bin/make_bundled_dataset %s %i' % bundle_info
    job['meta'] = datadir + '/bundles/.crunchd_metadata.json'
    job['dir'] = offline_dir
    job['clean'] = None
    job['deps'] = {}

    runfile = datadir + '/bundles/run_list_%s_%03i.txt' % bundle_info
    runs = np.genfromtxt(runfile, dtype=np.int)

    for run in runs:
        job['deps'][datadir + '/crunched/run_%05i.root' % run] = new_dep

    job['deps'][offline_dir + '/bin/make_bundled_dataset'] = new_dep
    jobs[0].append(job)
        
    return jobs


def check_job(run_num, job):

    run_key = str(run_num).zfill(5)
            
    try:
        f = open(job['meta'], 'r')
        f.close()

    except(IOError):
        log.info('%s did not exist, creating it' % job['meta'])
        f = open(job['meta'], 'w')
        f.write(json.dumps({}))
        f.close()

    # Fill out dependency globs
    for fkey in job['deps'].keys():

        if '*' in fkey:
            del job['deps'][fkey]

            log.debug('expanding glob %s' % fkey)
            for f in glob.glob(fkey):
                job['deps'][f] = {'time': None, 'md5': None}

    with open(job['meta']) as f:

        try:
            metadata = json.load(f)[run_key][job['name']]

        except(KeyError):
            log.info('failed to load job metadata for run %s' % run_key)

            for fkey in job['deps'].keys():

                # Add the modification time stamp
                s = os.stat(fkey)
                job['deps'][fkey]['time'] = s.st_mtime

                # Compute the check sum.
                with open(fkey) as f:
                    md5sum = hashlib.md5(f.read()).hexdigest()
            
                    job['deps'][fkey]['md5'] = md5sum

            return True

    for fkey in job['deps'].keys():

        # Open a handle for filestats
        s = os.stat(fkey)

        # Make sure the dependency isn't new.
        try:
            metadata['deps'][fkey]

        except(KeyError):
            log.info('new dependency, %s, reanalyzing' % fkey)
            metadata['attempted'] = False

            # Add the modification time stamp
            job['deps'][fkey]['time'] = s.st_mtime

            # Compute the check sum.
            with open(fkey) as f:
                md5sum = hashlib.md5(f.read()).hexdigest()
            
            job['deps'][fkey]['md5'] = md5sum
            continue

        # Next check the timestamp.
        if s.st_mtime != metadata['deps'][fkey]['time']:
            log.info('current file: %s' % fkey)
            log.info('last modification time changed. Computing checksum')
            job['deps'][fkey]['time'] = s.st_mtime

            # Compute the check sum to see if the file really changed.
            with open(fkey) as f:
                md5sum = hashlib.md5(f.read()).hexdigest()
            
            job['deps'][fkey]['md5'] = md5sum

            if metadata['deps'][fkey]['md5'] != md5sum:
                log.info('Checksum did not match, reanalyzing')
                metadata['attempted'] = False

            else:
                metadata['attempted'] = True

        else:
            # The files haven't changed.
            job['deps'][fkey] = metadata['deps'][fkey]

    if metadata['attempted']:
        log.debug('already ran %s for run %s' % (job['name'], run_key))

        # Update the metadata just in case.
        msg = {}
        msg['cmd'] = 'write'
        msg['info'] = job['meta']
        msg['run'] = str(run_num).zfill(5)
        msg['log'] = {}
        msg['log'][job['name']] = {'attempted': True}
        msg['log'][job['name']]['deps'] = job['deps']
        worker_sck.send(json.dumps(msg))

        return False

    else:
        return True


def check_job_set(run_num, job_set):
    
    for jobs in job_set:

        for job in jobs:

            if check_job(run_num, job) == True:
                return True

            else:
                pass

    return False


def spawn_jobs():
    global job_queue
    global workers

    for worker in workers:
        if worker[0].poll() is not None:
            job_queue[worker[1]][0].remove(worker[2])
            workers.remove(worker)
            # Log when workers change
            log.info([(w[1], w[2]['name']) for w in workers])

    if len(workers) >= nworkers:
        return
    
    for run in job_queue.keys():
            
        if len(job_queue[run][0]) == 0:
            job_queue[run].pop(0)

        if len(job_queue[run]) == 0:
            del job_queue[run]
            continue
        
        if len(workers) > nworkers:
            return

        for job in job_queue[run][0]:

            # Make sure the job isn't running already.
            if job in [worker[2] for worker in workers]:
                continue

            logdir = odb.get_value('/Logger/Log dir').rstrip()
            fdump = open(logdir + '/crunchd.dump', 'a')

            log.info("Starting job %s for run %05i." % (job['name'], int(run)))
            
            if check_job(run, job) == True:
                
                if job['clean'] == None:
                    cmd = job['cmd']

                else:
                    cmd = ' && '.join([job['cmd'], job['clean']])
          
                worker = Popen(cmd,
                               cwd=job['dir'],
                               env=os.environ,
                               stdout=fdump,
                               stderr=fdump,
                               close_fds=True,
                               shell=True)

                workers.append((worker, run, job))

                # Log when workers change
                log.info([(w[1], w[2]['name']) for w in workers])

            else:
                job_queue[run][0].remove(job)

            msg = {}
            msg['cmd'] = 'write'
            msg['info'] = job['meta']
            msg['run'] = str(run).zfill(5)
            msg['log'] = {}
            msg['log'][job['name']] = {'attempted': True}
            msg['log'][job['name']]['deps'] = job['deps']
            
            worker_sck.send(json.dumps(msg))

            if len(workers) >= nworkers:
                return


def write_reps():
    """Write replies back for queries to on status socket."""
    global maxcount

    # Process the next set.
    for count, req in enumerate(req_queue):
        
        rep = {}

        if req['type'] == 'check':
            
            if req['body']['type'] == 'standard':
                job_set = standard_job_set(req['body']['msg'])
                run_num = req['body']['msg']['run']
                rep['result'] = check_job_set(run_num, job_set)
            
            else:
                rep['result'] = False

        if req['type'] == 'var':
            
            if req['body'] == 'nworkers':
                rep['result'] = nworkers
            
            elif req['body'] == 'njobs':
                rep['result'] = len(job_queue)
            
            else:
                rep['result'] = None

        if req['type'] == 'status':

            status = ['gm2-nmr-crunchd is running as process %i' % os.getpid()]
            jobs = ' '.join(['(%s, %s)' % (w[1], w[2]['name']) for w in workers])
            status.append('    running jobs: %s' % jobs)
            status.append('    queue has %i jobs' % len(job_queue))
            
            req['result'] = '\n'.join(status)

        try:
            status_sck.send_json(rep)

        except(zmq.error.ZMQError):
            pass

        req_queue.remove(req)

        if count > maxcount:
            break
            

def write_logs():
    """Write data to the log file for crunchd."""
    global log_queue
    global maxcount

    # Process the first set
    for count, msg in enumerate(log_queue):

        loginfo = {}
        print msg
        for entry in msg['log'].keys():

            loginfo[entry] = {}

            for key in msg['log'][entry].keys():
                loginfo[entry][key] = msg['log'][entry][key]
        
        with open(msg['info'], 'r') as f:
            metadata = json.load(f)
            
        try:
            metadata[msg['run']]

        except(KeyError):
            metadata[msg['run']] = {}

        if msg['cmd'] == 'write':
            for key in loginfo.keys():
                metadata[msg['run']][key] = loginfo[key]

        elif msg['cmd'] == 'reset':
            metadata[msg['run']] = {}
            
        with open(msg['info'], 'w') as f:
            f.write(json.dumps(metadata, indent=2, sort_keys=True))

        log_queue.remove(msg)

        if count > maxcount:
            break


if __name__ == '__main__':
    main()
