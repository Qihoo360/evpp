echo -n $(uname -r|awk -F"." '{print $(NF-1) "." $NF}')
