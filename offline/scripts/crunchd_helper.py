import sys
import json
import zmq
import midas

def main():

    # Open the ZMQ socket that talks to the job scheduler.
    ctx = zmq.Context()
    sck = ctx.socket(zmq.PUSH)
    sck.connect('tcp://localhost:44444')

    # Get the data directory
    odb = midas.ODB('gm2-nmr')
    datadir = odb.get_value('/Logger/Data dir').rstrip() + '/'

    # Intialize a standard job submission.
    info = {'run': 1, 'type': 'normal'}

    # Update only new runs.
    if sys.argv[1] == 'update':

        metafile = datadir + 'crunched/.processing_metadata.json'
        crunched_runs = json.load(open(metafile)).keys()
        crunched_runs.sort()

        last_crunch = 1
        while (('%05i' % last_crunch) in crunched_runs):
            last_crunch += 1

        current_run = int(odb.get_value('/Runinfo/Run number'))

        print last_crunch, current_run

        if last_crunch >= current_run - 1:
            print 'Runs are all crunched.'
        
        else:
            for i in xrange(last_crunch, current_run):
                info['run'] = i
                rc = sck.send_json(info)
            
        
    # Update all runs given
    else:
        for i in xrange(1, len(sys.argv)):

            try:
                info['run'] = int(sys.argv[i])
                
            except(ValueError):
                print 'Not a valid run number.'
                continue

            rc = sck.send_json(info)


if __name__ == '__main__':
    main()

