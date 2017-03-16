

for num in 500 1000 10000 30000; do
for loop in 1 2 3 4 5 6 7 8 9 10; do
    echo "Bench index=$loop num=$num" 
    taskset -c 3 ./asio_test.exe socketpair $num 100 $num
done
done
