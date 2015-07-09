#this will dump frequencies 

mhist <<< $'0\n5\n4\n48\n180\n' > ~/Applications/midas/online/history/hstfiles/probe1freq.txt 2>&1;
mhist <<< $'0\n5\n5\n48\n180\n' > ~/Applications/midas/online/history/hstfiles/probe1freqErr.txt 2>&1;

