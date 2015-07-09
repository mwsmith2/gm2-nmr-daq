#get current date
DD=$(date +%d)
MM=$(date +%m)
YY=$(date +%y)

echo "looking for ~/Applications/midas/resources/data/$YY$MM$DD.hst"

# -E 10 is mscb (slow)
# -E 1 is laser tracker ("fast")
mhdump -t -n -E 10 ~/Applications/midas/resources/data/$YY$MM$DD.hst > ~/Applications/midas/online/history/mhdmpfiles/$YY$MM$DD.slow.txt 2>&1;
mhdump -t -n -E 1 ~/Applications/midas/resources/data/$YY$MM$DD.hst > ~/Applications/midas/online/history/mhdmpfiles/$YY$MM$DD.fast.txt 2>&1;

root -b -q ~/Applications/midas/online/history/treefill_slow.C++;
root -b -q ~/Applications/midas/online/history/treefill_fast.C++;
hadd -f ../history/rootfiles/$YY$MM$DD.allHists.root ../history/rootfiles/$YY$MM$DD.slowHists.root ../history/rootfiles/$YY$MM$DD.fastHists.root

./online-display -i ~/Applications/midas/online/history/rootfiles/$YY$MM$DD.allHists.root;
