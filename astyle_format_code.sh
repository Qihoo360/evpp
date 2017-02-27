
SUBDIRS="evpp test examples apps 3rdparty"

ASTYLE_BIN="3rdparty/astyle/build/gcc/bin/astyle"
ASTYLE="${ASTYLE_BIN} -A2 -HtUwpj -M80 -c -s4 --pad-header --align-pointer=type "

if test -f ${ASTYLE_BIN}
then
    echo "use astyle"
else
    tar zxvf 3rdparty/astyle_2.06_linux.tar.gz -C 3rdparty
    make -j -C 3rdparty/astyle/build/gcc
    if test -f ${ASTYLE_BIN}
    then
        echo "${ASTYLE_BIN} generated successfully."
    else
        echo "ERROR: astyle compiled failed!!!"
        exit 1
    fi
fi

for d in ${SUBDIRS}
do
    echo "astyle format subdir: $d "
    for file in $d/*.cc
    do
        echo ">>>>>>>>>> format file: $file "
        if test -f $file
        then
           ${ASTYLE} $file 
        fi
    done
done

