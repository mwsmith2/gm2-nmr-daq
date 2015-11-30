import sys
import time
import zmq
import json

def main():
    ctx = zmq.Context()
    sck = ctx.socket(zmq.PUSH)
    sck.connect('tcp://localhost:44444')
    time.sleep(0.01)

    info = {}
    info['run'] = 802
    info['cmd'] = 'DONE'
    info['opt'] = ''

    for i in xrange(1, len(sys.argv)):

        try:
            info['run'] = int(sys.argv[i])

        except(ValueError):
            print 'Not a valid run number.'
            continue

        rc = sck.send(json.dumps(info))


if __name__ == '__main__':
    main()

