#!/bin/python

import os
import json
import glob

flist = ['data/rome/.processing_metadata.json']
flist.append('data/shim/.processing_metadata.json')
flist.append('data/crunched/.processing_metadata.json')

for fname in flist:

    with open(fname) as f:
        meta = json.loads(f.read())

    print 'Updating metadata for: %s' % fname
    print 'Making a backup as %s.old' % fname

    count = 1
    while (os.path.isfile(fname + '.old.%i' % count)):
        count += 1
          
    with open(fname + '.old.%i' % count, 'w') as f:
        f.write(json.dumps(meta, indent=2, sort_keys=True))

    for runkey in meta.keys():

        try:
            meta[runkey].keys()

        except(AttributeError):
            del meta[runkey]
            continue
        
        for jobkey in meta[runkey].keys():
            
            for filekey in meta[runkey][jobkey]['deps']:
                
                info = {}
                stat = os.stat(filekey)

                info['time'] = stat.st_mtime

                # Check if this is the old format or new.
                try:
                    info['md5'] = meta[runkey][jobkey]['deps'][filekey]['md5']

                except(KeyError):
                    info['md5'] = meta[runkey][jobkey]['deps'][filekey]

                meta[runkey][jobkey]['deps'][filekey] = info

    print 'Saving metadata file %s.' % (fname)

    with open(fname + '.test', 'w') as f:
        f.write(json.dumps(meta, indent=2, sort_keys=True))

# And take care of the full_scans metadata.
fname = 'data/full_scans/.processing_metadata.json'
with open(fname) as f:
    meta = json.loads(f.read())
    
print 'Updating metadata for: %s' % fname
print 'Making a backup as %s.old' % fname

count = 1
while (os.path.isfile(fname + '.old.%i' % count)):
    count += 1
          
with open(fname + '.old.%i' % count, 'w') as f:
    f.write(json.dumps(meta, indent=2, sort_keys=True))

for runkey in meta.keys():

    try:
        meta[runkey].keys()
        
    except(AttributeError):
        del meta[runkey]
        continue

    if 'runs' in meta[runkey].keys():
        del meta[runkey]
        continue
    
    for jobkey in meta[runkey].keys():
            
        for filekey in meta[runkey][jobkey]['deps']:

            info = {}
            stat = os.stat(filekey)
            
            info['time'] = stat.st_mtime
            
            # Check if this is the old format or new.
            try:
                info['md5'] = meta[runkey][jobkey]['deps'][filekey]['md5']

            except(KeyError, TypeError):
                info['md5'] = meta[runkey][jobkey]['deps'][filekey]

            meta[runkey][jobkey]['deps'][filekey] = info

print 'Saving metadata file %s.' % (fname)

with open(fname + '.test', 'w') as f:
    f.write(json.dumps(meta, indent=2, sort_keys=True))


        
                



