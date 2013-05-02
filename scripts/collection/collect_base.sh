#!/bin/bash
#Script to get data on base images so that we can see how much they differ 
#across instances

#Parameters for the script
COUNT=20
KMODNAME=page_hash.ko
KMODSRCDIR=/home/zak/github/cs598mcc/kmod/page_hash
KMODDIR=/home/ubuntu/page_hash_run
DESTDIR=/home/zak/cs598mcc/data

#Openstack parameters
INSTANCE_NAME=samevm
IMAGE=kern_dev
KEY=zak
FLAVOR=1.small

#Our rudimentary "is this VM running?" test
function test_running() {
    nova list | grep -q $INSTANCE_NAME
}

#Will blow up if you have more than one instance with $INSTANCE_NAME
for i in `seq -w $COUNT`; do
    FILENAME="${INSTANCE_NAME}_run${i}"
    test_running && nova delete $INSTANCE_NAME
    while test_running; do
        sleep 20
        echo "Waiting for instance to shut down..."
    done 
    
    #Old instance is now stopped, start the new one
    nova boot --key-name $KEY --flavor $FLAVOR --image $IMAGE --poll $INSTANCE_NAME
    #Give some time for the OS to spin up
    sleep 10
    #expecting line like: | UUID | NAME   | STATE | novanetwork=IP1, IP2 |
    IP=`nova list | grep $INSTANCE_NAME | awk -F\| '{print $5}' | cut -d' ' -f3` 
    DONE=1
    while [ $DONE != 0 ]; do
        sleep 1
        ssh -oConnectTimeout=5 $IP "uptime" > /dev/null
        DONE=$?
    done

    #Okay, we can establish an ssh session - build and load the module
    ssh $IP "mkdir -p $KMODDIR"
    rsync -a -e ssh $KMODSRCDIR/* ${IP}:${KMODDIR}/
    ssh $IP "sudo apt-get -y install libssl-dev linux-source"
    ssh $IP "cd $KMODDIR && make && sudo make load"
    #Now that the module is loaded, get the hashes
    ssh $IP "sudo echo 1 > /proc/cs598/hash && while grep -q 0 /proc/cs598/hash; do sleep 1; done && cd $KMODDIR && sudo ./user_app $FILENAME > /home/ubuntu/${FILENAME}"
    scp $IP:/home/ubuntu/${FILENAME} $DESTDIR
done
