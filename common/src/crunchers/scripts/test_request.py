import sys
import time
import zmq
import json

def main():
    ctx = zmq.Context()
    sck = ctx.socket(zmq.REQ)
    sck.connect('tcp://127.0.1.1:44446')
    time.sleep(0.01)

    info = {}
    info['type'] = 'check'
    info['body'] = {}
    info['body']['type'] = 'normal'
    info['body']['msg'] = {'run': int(sys.argv[1]), 'type':'normal'}

    rc = sck.send(json.dumps(info))
    print json.dumps(sck.recv_json(), indent=2)

if __name__ == '__main__':
    main()

