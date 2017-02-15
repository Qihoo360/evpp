EL=`uname -r | awk -F. '{print $4}'`
ARCH=`uname -r | awk -F. '{print $5}'`
echo -n ${EL}.${ARCH}
