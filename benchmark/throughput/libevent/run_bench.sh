#!/bin/sh

# Author: Marc Lehmann
# File from http://data.plan9.de/runbench
# See also http://libev.schmorp.de/bench.html

run2() {
   s1=9999999999
   s2=9999999999
   l1=9999999999
   l2=9999999999
#   for try in 1 2 3 4 5 6; do
#   for try in 1 2; do
   for try in 1; do
      #nice -n-20 
      taskset 2 "$@" | awk 'NR==1 {print} NR!=1 {all += $1; loop += $2;} END { print all/(NR-1), loop/(NR-1)}' >runbench.tmp
      #taskset 2 "$@" >runbench.tmp
      {
         read s1 l1
         read s2 l2
      } <runbench.tmp
      rm runbench.tmp

#      [ $S1 -lt $s1 ] && s1=$S1
#      [ $S2 -lt $s2 ] && s2=$S2
#      [ $L1 -lt $l1 ] && l1=$L1
#      [ $L2 -lt $l2 ] && l2=$L2
   done
}

run() {
   if [ "$3" -gt "$1" ]; then
      echo "- - - -"
   else
      run2 ./bench -n "$@"
      ls1=$s1
      ls2=$s2
      ll1=$l1
      ll2=$l2
      run2 ./pingpong_bench -n "$@"
      ns1=$s1
      ns2=$s2
      nl1=$l1
      nl2=$l2
      run2 ./pingpong_bench_ctladd -n "$@"
   fi

   echo "$ls2 $ll2 $ns2 $nl2 $s2 $l2"
}

plot() {
   gnuplot - <<EOF
set terminal png size 2400,1600 nocrop butt font "/usr/share/fonts/truetype/ttf-bitstream-vera/Vera.ttf" 20
set output "$1.png"

set pointsize 1.2
set logscale x
set multiplot title "$2"
set size 0.5,0.5
set xlabel "file descriptors"
set ylabel "time (in µs)\\n(lower is better)"

set origin 0,0.5
set title "\\ntotal time per iteration\\n100 active clients"
set key left top
set xrange [100:*]
plot \\
   "$1" using 1:2 axis x1y1 with lines lw 5 title "libevent2",	\\
   "$1" using 1:4 axis x1y1 with lines lw 5 title "muduo", \\
   "$1" using 1:6 axis x1y1 with lines lw 5 title "muduo (ctl_add)"

set origin 0,0
set title "1000 active clients"
set key left top
set xrange [1000:*]
plot \\
   "$1" using 1:8 axis x1y1 with lines lw 5 title "libevent2",	\\
   "$1" using 1:10 axis x1y1 with lines lw 5 title "muduo", \\
   "$1" using 1:12 axis x1y1 with lines lw 5 title "muduo (ctl_add)"

set origin 0.49,0.5
set title "\\ntime spent in event processing\\n100 active clients"
set key right bottom
set xrange [100:*]
plot \\
   "$1" using 1:3 axis x1y1 with lines lw 5 title "libevent2",	\\
   "$1" using 1:5 axis x1y1 with lines lw 5 title "muduo", \\
   "$1" using 1:7 axis x1y1 with lines lw 5 title "muduo (ctl_add)"

set origin 0.49,0
set title "1000 active clients"
set key left top
set xrange [1000:*]
plot \\
   "$1" using 1:9 axis x1y1 with lines lw 5 title "libevent2",	\\
   "$1" using 1:11 axis x1y1 with lines lw 5 title "muduo", \\
   "$1" using 1:13 axis x1y1 with lines lw 5 title "muduo (ctl_add)"
EOF

   mogrify -geometry 50% "$1.png"
   optipng "$1.png"
}

dobench() {
   N=$(perl -e 'print join " ", (map int .5+0.1*1000**(1+$_/67.5*1.5), 0..45), "\n"')
   N="100 200 300 400 500 600 750 1000 2000 3000 4000 5000 6000 7500 10000 20000 30000 50000 70000 100000"
   #N="100 200 300 500 700 1000 2000 3000 5000 7000 10000 30000 100000"
   if true; then
      (
         for n in $N; do
            echo $n $(run $n -a 100) $(run $n -a 1000)
         done
      ) >dat.t0
   fi
   if false; then
      (
         for n in $N; do
            echo $n $(run $n -a 100 -t) $(run $n -a 1000 -t)
         done
      ) >dat.t1
   fi
}

doplot() {
   plot dat.t0 "no timeouts"
   #plot dat.t1 "with per-connection idle timeouts"
}

dodist() {
   pod2xhtml <bench.pod >bench.html --noindex --css http://res.tst.eu/pod.css
   chmod 644 dat*png bench.c bench.html
   rsync -avP bench.html bench.c dat*png ruth:/var/www/libev.schmorp.de/.
}

#dobench
doplot
#dodist

