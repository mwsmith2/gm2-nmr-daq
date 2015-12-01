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
    info['type'] = 'normal'

    for i in xrange(1, len(sys.argv)):
        info['run'] += 1
        info['opt'] = sys.argv[i]
        rc = sck.send(json.dumps(info))

    if len(sys.argv) < 2:
        info['opt'] = 'new: Test Message'
        rc = sck.send(json.dumps(info))

if __name__ == '__main__':
    main()

