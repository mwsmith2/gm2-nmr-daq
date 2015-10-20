import os
import sys
import json
import glob
import subprocess
import hashlib

info = {}

def main():

    offline_dir = os.path.dirname(__file__) + '/..'
    os.chdir(offline_dir)
    
    data_info_file = 'data/processing_info.json'
    global info

    try:
        info = json.loads(open(data_info_files))
    
    except:
        info = { 'runs': {}, 'crunched_files': [] }

    print info

    runs = []

    if len(sys.argv) > 1:
        for i in sys.argv[1:]:
            runs.append(int(i))

    else:

        for mf in glob.glob('data/midas/*mid'):
            print mf

            if mf in info['crunched_files']:
                pass

            else:
                run_num = int(mf.split('.')[-2][-5:])
                process_midas(run_num)

        for rf in glob.glob('data/rome/*.root'):
            print rf

            if rf in info['crunched_files']:
                pass

            else:
                run_num = int(rf.split('.')[-2][-5:])
                process_rome(run_num)

    for run in runs:
        process_midas(run)
        process_rome(run)

    print os.getcwd()
    with open(data_info_file, 'w') as f:
        f.write(json.dumps(info))

def process_midas(run_num):
    print "process_midas(%i)" % run_num

    rome_analyzers = ['sync', 'ctec', 'slowcont', 'tilt']
    cmd_prefix = "./midanalyzer.exe -i romeConfig.xml -r "
    cmd_suffix = " -m offline -p 0 -q"

    for ra in rome_analyzers:

        os.chdir('rome-analyzers/' + ra)
        print cmd_prefix + str(run_num) + cmd_suffix
        subprocess.call(cmd_prefix + str(run_num) + cmd_suffix, shell=True)

        for rf in glob.glob('*.root'):
            subprocess.call(['rm', rf])

        os.chdir('../../')
        
    try:
        info[run_num]['rome'] = rome_analyzers
    
    except:
        info[run_num] = { 'rome': rome_analyzers }

    for f in glob.glob('data/midas/*%i.mid' % run_num):
        info['crunched_files'].append(f)


def process_root(run_num):
    print "process_root(%i)" % run_num

    cmd = 'bin/shim_data_bundler'
    subprocess.call([cmd, str(run_num)])

    md5sum = hashlib.md5(open(cmd, 'r').read()).hexdigest()

    try:
        info[run_num]['shim'] = md5sum
    
    except:
        info[run_num] = { 'shim': md5sum }

    for f in glob.glob('data/root/*%i.root' % run_num):
        info['crunched_files'].append(f)

    for f in glob.glob('data/rome/*%i.root' % run_num):
        info['crunched_files'].append(f)

def process_rome(run_num):
    print "process_rome(%i)" % run_num

    cmd = 'bin/update_rome_data'
    subprocess.call([cmd, str(run_num)])

    md5sum = hashlib.md5(open(cmd, 'r').read()).hexdigest()

    try:
        info[run_num]['rome'] = md5sum
    
    except:
        info[run_num] = { 'rome': md5sum }

    for f in glob.glob('data/rome/*%i.root' % run_num):
        info['crunched_files'].append(f)


def process_shim(run_num):
    print "process_shim(%i)" % run_num

    cmd = './bin/recrunch_fids'
    subprocess.call([cmd, 'data/shim/run_%05i.root' % (run_num)])
    
    md5sum = hashlib.md5(open(cmd, 'r').read()).hexdigest()

    try:
        info[run_num]['recrunched'] = md5sum
    
    except:
        info[run_num] = { 'recrunched': md5sum }

    for f in glob.glob('data/shimt/*%i.root' % run_num):
        info['crunched_files'].append(f)

if __name__ == '__main__':
    main()
