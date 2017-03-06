#!/bin/bash

# Run this script at the directory which holds it.

cd ../

SUBDIRS="evpp evpp/http evpp/httpc evpp/udp test examples/echo/httpecho examples/echo/udpecho examples/echo/tcpecho examples/pingpong/client examples/pingpong/server apps/evmc/test apps/evmc apps/evnsq apps/evnsq/test "
FILETYPES="*.cc *.h"
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
    for t in ${FILETYPES}
    do
        for file in $d/$t
        do
            echo ">>>>>>>>>> format file: $file "
            if test -f $file
            then
               ${ASTYLE} $file 
               rm -f ${file}.orig
            fi
        done
    done
done

