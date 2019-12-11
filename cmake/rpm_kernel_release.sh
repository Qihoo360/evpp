echo -n $(/usr/lib/rpm/redhat/dist.sh --dist | awk -F '.' '{print $2}').$(uname -m)
