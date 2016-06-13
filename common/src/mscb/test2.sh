#!/bin/bash

echo what do you wanna do?

read feedback

while [ "_$feedback" != _q ] ; do  
    case $feedback in 
        1) comm='ps aux';;
        2) comm='ls ${HOME}';;
        3) comm='netstat | grep "^tcp"';;
        *) echo undefined choice && exit 1;;
    esac    

eval $comm

echo "Make another choice or press 'q' for exit"
read feedback

done
