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

    for run_num in range(800):
        run_files = glob.glob(arch_dir + '/*%05i*' % run_num)
        run_attr[run_num] = {}
        
        for f in run_files:
            print f

            for line in open(f):

                if 'xml' in f:
                    if '"step_size" type="DOUBLE"' in line:
                        step_size = float(line.split('>')[1].split('<')[0])
                        run_attr[run_num]['step_size'] = step_size
                            
                    if '"Laser Tracker Point" type="STRING"' in line:
                        laser_point = line.split('>')[1].split('<')[0].upper()

                        if laser_point != 'P1' or laser_point != 'P2':
                            laser_point = 'N'

                        run_attr[run_num]['laser_point'] = laser_point.strip()

                else:

                    if 'step_size = DOUBLE' in line:
                        step_size = float(line.split(':')[1])
                        run_attr[run_num]['step_size'] = step_size
                            
                    if 'Laser Tracker Point = STRING' in line:
                        laser_point = line.split(':')[1].split(']')[1].upper()

                        if laser_point != 'P1' or laser_point != 'P2':
                            laser_point = 'N'

                        run_attr[run_num]['laser_point'] = laser_point.strip()

        print '\n'
        print run_attr[run_num]
        print '\n\n'

        try:
            run_attr[run_num]['step_size']

        except:
            run_attr[run_num]['step_size'] = -1.0

        try:
            run_attr[run_num]['laser_point']

        except:
            run_attr[run_num]['laser_point'] = 'N'

    
    with open(attr_file, 'w') as f:
        f.write(json.dumps(run_attr, indent=2))

if __name__ == '__main__':
    main()
