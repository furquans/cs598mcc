#!/bin/bash
#Script to get data from a st of hosts, originally for the hadoop cluster
#across instances

#Parameters for the script
KMODNAME=page_hash.ko
KMODSRCDIR=/home/zak/github/cs598mcc/kmod/page_hash
KMODDIR=/home/vagrant/page_hash_run
DESTDIR=/home/zak/cs598mcc/data

KEYFILE=/home/zak/insecure_private_key

#Our rudimentary "is this VM running?" test
function test_running() {
    nova list | grep -q $INSTANCE_NAME
}

HOSTS="vm-cluster-node1 vm-cluster-node2 vm-cluster-node3 vm-cluster-node4 vm-cluster-node5"
#Will blow up if you have more than one instance with $INSTANCE_NAME
for H in $HOSTS; do
    sleep 10
    FILENAME="$H"
    #Okay, we can establish an ssh session - build and load the module
    ssh $H "mkdir -p $KMODDIR"
    rsync -a -e ssh $KMODSRCDIR/* ${H}:${KMODDIR}/
#    ssh $H "sudo apt-get -y install libssl-dev linux-source gcc" 

    ssh $H "cd $KMODDIR && make && sudo make load"
    #Now that the module is loaded, get the hashes
    ssh $H "sudo echo 1 > /proc/cs598/hash && while grep -q 0 /proc/cs598/hash; do sleep 1; done && cd $KMODDIR && sudo ./user_app $FILENAME > /home/vagrant/${FILENAME}"
    scp $H:/home/vagrant/${FILENAME} $DESTDIR
done
