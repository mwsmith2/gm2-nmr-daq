echo " How far back would you like to include in the history plots? "
read hours

#get current date
DD=$(date +%d)
MM=$(date +%m)
YY=$(date +%y)

# this will dump individual variables for the last 48 hours with a minimum time of 3 minutes between readings
# change time to look backwards with the 4th input response
# you can explore the menu the responses navigate by just starting mhist with no arguments

mhist <<< $'0\n1\n0\n48\n180\n' > ~/Applications/midas/online/display/hstfiles/temp1_$YY$MM$DD.txt 2>&1;
mhist <<< $'0\n1\n1\n48\n180\n' > ~/Applications/midas/online/display/hstfiles/temp2_$YY$MM$DD.txt 2>&1;
mhist <<< $'0\n1\n8\n48\n180\n' > ~/Applications/midas/online/display/hstfiles/capac1_$YY$MM$DD.txt 2>&1;
mhist <<< $'0\n1\n9\n48\n180\n' > ~/Applications/midas/online/display/hstfiles/capac2_$YY$MM$DD.txt 2>&1;
mhist <<< $'0\n5\n4\n100\n120\n' > ~/Applications/midas/online/display/hstfiles/freq1_$YY$MM$DD.txt 2>&1;


root -b -q ~/Applications/midas/online/display/onlinedispHists.C++;

./online-display -i ~/Applications/midas/online/display/rootfiles/$YY$MM$DD.root;
