#!/usr/bin/python
import os

import glob
import json

def main():
    """Checks for any files to merge with runlog.  
    This is the only program that should write to the runlog"""
    
    runlog_updates = []

    if os.path.exists(lock_file)
    open('runlog.json.lock', 'a').close()

    for f in glob.glob(
