#!/bin/sh


#check the error code, if there is error happened, we exit
function check_error
{
   declare exitcode=$?
   if [[ exitcode -eq 0 ]]; then
     echo "exitcode=$exitcode"
     return
   fi

   exit 1
}

NSQPATH=../../nsq-0.3.8.linux-amd64.go1.6.2/bin
$NSQPATH/nsqlookupd & nsqlookupd_pid=$!
$NSQPATH/nsqd --lookupd-tcp-address=127.0.0.1:4160 & nsqd_pid=$!
echo "nsqd_pid=$nsqd_pid"
echo "nsqlookupd_pid=$nsqlookupd_pid"

echo 11111111111111111111111111111111111111111
for i in 1 2 3 4 5 6 7 8 9 10; do
../../build/bin/unittest_evnsq_producer_with_auth -c 10
check_error
echo "22222222222222222222222 i=$i"
done

curl 'http://127.0.0.1:4151/stats'
check_error
curl 'http://127.0.0.1:4151/stats?format=json'
check_error
curl 'http://127.0.0.1:4151/stats?format=json'|grep '"message_count":100' | grep '"depth":100'
check_error

for i in 1 2 3 4 5 6 7 8 9 10; do
../../build/bin/unittest_evnsq_producer_with_auth -h "http://127.0.0.1:4161/lookup?topic=test1" -c 10
check_error
done

curl 'http://127.0.0.1:4151/stats'
check_error
curl 'http://127.0.0.1:4151/stats?format=json'
check_error
curl 'http://127.0.0.1:4151/stats?format=json'|grep '"message_count":200' | grep '"depth":200'
check_error


# Test reconnection feature
../../build/bin/unittest_evnsq_producer_with_auth -c 100 & evnsq_producer_pid=$!
sleep 5
kill $nsqd_pid
$NSQPATH/nsqd --lookupd-tcp-address=127.0.0.1:4160 & nsqd_pid=$!
echo "nsqd_pid=$nsqd_pid"
sleep 5
kill $nsqd_pid
$NSQPATH/nsqd --lookupd-tcp-address=127.0.0.1:4160 & nsqd_pid=$!
echo "nsqd_pid=$nsqd_pid"
sleep 20

curl 'http://127.0.0.1:4151/stats?format=json'| ~/jsonformat
curl 'http://127.0.0.1:4151/stats?format=json'| grep '"depth":300'

echo "nsqd_pid=$nsqd_pid"
echo "nsqlookupd_pid=$nsqlookupd_pid"
kill $nsqd_pid
kill $nsqlookupd_pid
