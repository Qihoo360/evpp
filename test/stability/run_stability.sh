
for port in 7081 7082 7083 7084 7085 7086 7087 7088 7089; do
    mkdir -p $port && cd $port
    nohup ../../../build-debug/bin/test_evpp_stability $port &
    cd ..
done

for port in 8081 8082 8083 8084 8085 8086 8087 8088 8089; do
    mkdir -p $port && cd $port
    nohup ../../../build-debug/bin/test_evpp_stability_boost_lockfree $port &
    cd ..
done

for port in 9081 9082 9083 9084 9085 9086 9087 9088 9089; do
    mkdir -p $port && cd $port
    nohup ../../../build-debug/bin/test_evpp_stability_concurrentqueue $port &
    cd ..
done

