import sys
import re
import json
import glob

def main():
    attr_file = '/home/newg2/Applications/gm2-nmr/resources/log/run_attributes.json'
    arch_dir = '/home/newg2/Applications/gm2-nmr/resources/history'

    try:
        run_attr = json.loads(open(attr_file).read())

    except:
        run_attr = {}
    
    if len(sys.argv) > 2:
         start = int(sys.argv[1])
         stop = int(sys.argv[2])

    else:
        start = 0
        stop = int(sys.argv[1])

    for run_num in range(start, stop):
        run_files = glob.glob(arch_dir + '/*%05i*' % run_num)
        key = "%05i" % run_num
        run_attr[key] = {}
        
        for f in run_files:

            for line in open(f):

                if 'xml' in f:
                    if '"step_size" type="DOUBLE"' in line:
                        step_size = float(line.split('>')[1].split('<')[0])
                        run_attr[key]['step_size'] = step_size
                            
                    if '"Laser Tracker Point" type="STRING"' in line:

                        laser_point = line.split('>')[1].split('<')[0].upper()
                        laser_point = laser_point.strip()

                        if laser_point != 'P1' and laser_point != 'P2':
                            laser_point = 'N'

                        run_attr[key]['laser_point'] = laser_point

                else:

                    if 'step_size = DOUBLE' in line:
                        step_size = float(line.split(':')[1])
                        run_attr[key]['step_size'] = step_size
                            
                    if 'Laser Tracker Point = STRING' in line:

                        laser_point = line.split(':')[1].split(']')[1].upper()
                        laser_point = laser_point.strip()

                        if laser_point != 'P1' and laser_point != 'P2':
                            laser_point = 'N'

                        run_attr[key]['laser_point'] = laser_point

        try:
            run_attr[key]['step_size']

        except:
            run_attr[key]['step_size'] = -1.0

        try:
            run_attr[key]['laser_point']

        except:
            run_attr[key]['laser_point'] = 'N'

        print run_attr[key]
    
    with open(attr_file, 'w') as f:
        f.write(json.dumps(run_attr, indent=2, sort_keys=True))

if __name__ == '__main__':
    main()
