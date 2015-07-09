export ROOTSYS=/usr/local/opt/root
export PATH=/home/cenpa/Applications/midas/resources/bin:/home/cenpa/Packages/gm2midas/midas/linux/bin:/home/cenpa/Packages/gm2midas/midas/utils:/usr/local/opt/root/bin:/opt/rh/devtoolset-2/root/usr/bin:/usr/local/bin:/opt/rh/devtoolset-2/root/usr/bin:/opt/rh/devtoolset-2/root/usr/bin:/usr/lib64/qt-3.3/bin:/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin:/home/cenpa/Workspace/midas/packages/midas/linux64/bin:/home/cenpa/Workspace/midas/packages/roody/bin:/home/cenpa/bin:/home/cenpa/Workspace/midas/packages/midas/linux64/bin:/home/cenpa/Workspace/nmr-daq/online/bin:/home/cenpa/Packages/gm2midas/midas/linux/bin
export LD_LIBRARY_PATH=/home/cenpa/Packages/gm2midas/midas/linux/lib:/usr/local/opt/root/lib:/home/cenpa/Packages/gm2midas/midas/linux/lib

#get current date
DD=$(date +%d)
MM=$(date +%m)
YY=$(date +%y)

# -E 10 is mscb (slow)
# -E 1 is laser tracker ("fast")
mhdump -t -n -E 10 ~/Applications/midas/resources/data/$YY$MM$DD.hst > ~/Applications/midas/online/history/mhdmpfiles/$YY$MM$DD.slow.txt 2>&1;
mhdump -t -n -E 1 ~/Applications/midas/resources/data/$YY$MM$DD.hst > ~/Applications/midas/online/history/mhdmpfiles/$YY$MM$DD.fast.txt 2>&1;

root -b -q ~/Applications/midas/online/history/treefill_slow.C++;
root -b -q ~/Applications/midas/online/history/treefill_fast.C++;
hadd -f ~/Applications/midas/online/history/rootfiles/$YY$MM$DD.allHists.root ~/Applications/midas/online/history/rootfiles/$YY$MM$DD.slowHists.root ~/Applications/midas/online//history/rootfiles/$YY$MM$DD.fastHists.root
