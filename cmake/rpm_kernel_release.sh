#Docker has no kernel
#Check if evpp is in container
INCONTAINE=`cat /proc/1/cgroup  | grep 'docker\|lxc'`
if [ ! -z "$INCONTAINE" ]; then
    echo -n $(cat /etc/redhat-release | awk -F ' ' '{printf("%s.%s.%s\n",$1,$2,$3)}')
else
    echo -n $(uname -r | awk -F"." '{print $(NF-1) "." $NF}')
fi
