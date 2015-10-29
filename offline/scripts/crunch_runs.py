#!/usr/bin/python

import os
import sys
import json
import glob
import subprocess
import hashlib


def main():

    offline_dir = os.path.abspath(os.path.dirname(__file__) + '/..')
    os.chdir(offline_dir)

    info_file = offline_dir + '/data/crunched/.processing_metadata.json'
    runs = []

    if len(sys.argv) > 1:
        for i in sys.argv[1:]:
            runs.append(int(i))

        runs.sort()

        cmd = ['python', 'scripts/extract_run_attr.py']
        cmd.append(str(run[0]))
        cmd.append(str(run[-1])
        subprocess.call(cmd)

        for run in runs:
            try:
                process_midas(run)
                process_root(run)
                process_shim(run)
            except:
                pass


    else:

        try:
            info = json.loads(open(info_file).read())

        except:
            with open(info_file, 'w') as f:
                f.write(json.dumps({}, indent=2, sort_keys=True))
            
        try:
            last_full_crunch = info['last_full_crunch'] + 1

        except:
            last_full_crunch = 1

        current_run_number = last_full_crunch

        for mf in glob.glob('data/midas/run*.mid'):
            if int(mf[-9:-4]) > current_run_number:
                current_run_number = int(mf[-9:-4])


        print last_full_crunch, current_run_number

        cmd = ['python', 'scripts/extract_run_attr.py']
        cmd.append(str(last_full_crunch))
        cmd.append(str(current_run_number))
        subprocess.call(cmd)

        for run in range(last_full_crunch, current_run_number):
            try:
                process_midas(run)
                os.chdir(offline_dir)

                process_root(run)
                os.chdir(offline_dir)

                process_shim(run)
                os.chdir(offline_dir)
            except:
                pass

            os.chdir(offline_dir)
            info = json.loads(open(info_file).read())
            info['last_full_crunch'] = run

            with open(info_file, 'w') as f:
                f.write(json.dumps(info, indent=2, sort_keys=True))


def process_midas(run_num):
    print "process_midas(%i)" % run_num

    info_file = 'data/rome/.processing_metadata.json'
    run_key = str(run_num)

    if (run_num < 787):
        rome_analyzers = ['sync', 'ctec', 'slowcont', 'tilt']

    else:
        rome_analyzers = ['sync_ext_ltrk', 'ctec', 'slowcont', 'tilt']

    try:
        info = json.loads(open(info_file).read())[run_key]

    except:
        # Initialize the metadata.
        info = {}

    for ra in rome_analyzers:

        try:
           info[ra] == None

        except:
           info[ra] = 0

    cmd_prefix = "./midanalyzer.exe -i romeConfig.xml -r "
    cmd_suffix = " -m offline -p 0 -q"

    for ra in rome_analyzers:

        os.chdir('rome-analyzers/' + ra)

        md5sum = hashlib.md5(open('midanalyzer.exe', 'r').read()).hexdigest()

        if info[ra] == md5sum:
            print "Already processed for %s." % ra
            os.chdir('../../')

        else:
            print "Running rome analyzer %s." % ra
            info[ra] = md5sum
            all_info = json.loads(open(info_file).read())
            all_info[run_key] = info
            with open(info_file, 'w') as f:
                f.write(json.dumps(all_info, indent=2, sort_keys=True))

            subprocess.call(cmd_prefix + str(run_num) + cmd_suffix, shell=True)

            # Clean up unecessary files.
            for rf in glob.glob('*.root'):
                subprocess.call(['rm', rf])
        
            # Back to offline dir
            os.chdir('../../')


def process_root(run_num):
    print "process_root(%i)" % run_num

    info_file = 'data/shim/.processing_metadata.json'
    run_key = str(run_num)
    cmd = 'bin/shim_data_bundler'

    try:
        info = json.loads(open(info_file).read())[run_key]

    except:
        # Initialize the metadata.
        info = {}

    try:
        info[cmd] == None

    except:
        info[cmd] = 0

    input_files = []
    for f in glob.glob('data/rome/%05i.root' % run_num):
        input_files.append(f)

    for f in glob.glob('data/root/%05i.root' % run_num):
        input_files.append(f)

    # Make sure none of the input files have changed.
    for f in input_files:

        md5sum = hashlib.md5(open(f, 'r').read()).hexdigest()
        
        try:
            if info[f] == md5sum:
                pass

            else:
                # Reset the processing
                info[fname] = md5sum        
                info[cmd] = 0

        except:
            info[f] = md5sum

    md5sum = hashlib.md5(open(cmd, 'r').read()).hexdigest()

    if info[cmd] == md5sum:
        print "Already processed with %s." % cmd
        return

    else:
        info[cmd] = md5sum
        all_info = json.loads(open(info_file).read())
        all_info[run_key] = info
        with open(info_file, 'w') as f:
            f.write(json.dumps(all_info, indent=2, sort_keys=True))

        subprocess.call([cmd, str(run_num)])

def process_shim(run_num):
    print "process_shim(%i)" % run_num

    info_file = 'data/crunched/.processing_metadata.json'
    run_key = str(run_num)
    cmd = './bin/recrunch_fids'

    try:
        info = json.loads(open(info_file).read())[run_key]

    except:
        # Initialize the metadata.
        info = {}

    try:
        info[cmd] == None

    except:
        info[cmd] = 0

    # Check to see if the input file has changed.
    fname = 'data/shim/run_%05i.root' % run_num

    md5sum = hashlib.md5(open(fname, 'r').read()).hexdigest()
    try:
        if info[fname] == md5sum:
            pass

        else:
            info[fname] = md5sum        
            info[cmd] = 0

    except:
        info[fname] = md5sum        
 
    md5sum = hashlib.md5(open(cmd, 'r').read()).hexdigest()

    if info[cmd] == md5sum:
        print "Already processed with %s." % cmd
        return

    else:
        info[cmd] = md5sum
        all_info = json.loads(open(info_file).read())
        all_info[run_key] = info
        with open(info_file, 'w') as f:
            f.write(json.dumps(all_info, indent=2, sort_keys=True))

        subprocess.call([cmd, 'data/shim/run_%05i.root' % (run_num)])
    

if __name__ == '__main__':
    main()
