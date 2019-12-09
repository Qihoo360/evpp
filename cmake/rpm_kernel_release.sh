#Docker has no kernel
#Check if evpp is in container
#Suppor Red Linux
INCONTAINE=`cat /proc/1/cgroup  | grep 'docker\|lxc'`
if [ ! -z "$INCONTAINE" ]; then
    arch=`uname -r | awk -F "." '{print $NF}'`
    os_version=`cat /etc/issue| grep release |awk -F ' ' '{print $(1)}'`
    num=`cat /etc/issue | grep release | awk -F ' ' '{print $3}' | awk -F '.' '{print $1}'` 
    version=`awk -v myVar="$os_version" 'BEGIN{info=myVar; if(info ~ /CentOS|Red/){print "el"}}'`
    echo -n ${version}${num}.$arch
else
    echo -n $(uname -r | awk -F"." '{print $(NF-1) "." $NF}')
fi
