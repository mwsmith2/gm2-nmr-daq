import sys
import json
import subprocess 
import hashlib

def main():

    info_file = 'data/full_scans/.processing_metadata.json'

    try:
        info = json.loads(open(info_file).read())

    except:
        with open(info_file, 'w') as f:
            f.write(json.dumps({}))

    if len(sys.argv) < 2:
        print "Insufficient arguments."
        print "usage: python update_full_scans.py <command> [runs ...]"

    if sys.argv[1] == 'add':
        print "Adding new full scan."

        keys = json.loads(open(info_file).read()).keys()
        keys.sort()

        try:
            run_num = int(keys[-1]) + 1

        except:
            run_num = 1

        info = {}
        
        info['./bin/bundle_full_scan'] = 0
        info['runs'] = {}

        for idx in range(2, len(sys.argv)):
            info['runs'][sys.argv[idx]] = 0

        all_info = json.load(open(info_file))
        all_info[str(run_num)] = info

        print all_info
        with open(info_file, 'w') as f:
            f.write(json.dumps(all_info, indent=2, sort_keys=True))

    elif sys.argv[1] == 'update':
        print "Rebundling full scans."

        keys = json.load(open(info_file)).keys()
        keys.sort()
        
        for run in keys:
            process_full_run(int(run))

    else:
        print "Unrecognized argument."
        print "usage: python update_full_scans.py <command> [runs ...]"

def process_full_run(run_num):

    info_file = 'data/full_scans/.processing_metadata.json'
    info = json.load(open(info_file))[str(run_num)]

    prog = './bin/bundle_full_scan'
    reprocess = False
    cmd = []
    # Check all the checksums to see if things have changed.
    md5sum = hashlib.md5(open(prog, 'r').read()).hexdigest()
    cmd.append(prog)

    if info[prog] != md5sum:
        reprocess = True
        info[prog] = md5sum

    # Set the output file and data directory.
    cmd.append('data/full_scans/full_scan_run_%05i.root' % int(run_num))
    cmd.append('data/crunched')

    for run in info['runs']:

        fname = 'data/crunched/run_%05i.root' % int(run)
        md5sum = hashlib.md5(open(fname, 'r').read()).hexdigest()
        cmd.append(str(run))

        if info['runs'][str(run)] != md5sum:

            reprocess = True
            info['runs'][str(run)] = md5sum

    if reprocess:
        subprocess.call(cmd)
        
        all_info = json.load(open(info_file))
        all_info[str(run_num)] = info

        with open(info_file, 'w') as f:
            f.write(json.dumps(all_info))

    else:
        print "Nothing to update for full scan run %i" % int(run)


if __name__ == '__main__':
    main()
