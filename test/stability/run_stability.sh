
for port in 7081 7082 7083 7084 ; do
    mkdir -p $port && cd $port
    nohup ../../../build-debug/bin/test_evpp_stability $port &
    cd ..
done

for port in 8081 8082 8083 8084 ; do
    mkdir -p $port && cd $port
    nohup ../../../build-debug/bin/test_evpp_stability_boost_lockfree $port &
    cd ..
done

for port in 9081 9082 9083 9084 ; do
    mkdir -p $port && cd $port
    nohup ../../../build-debug/bin/test_evpp_stability_concurrentqueue $port &
    cd ..
done

