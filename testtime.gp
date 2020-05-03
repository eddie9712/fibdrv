reset
set xlabel 'size'
set ylabel 'time(ns)'
set title 'fibonacci number'
set term png enhanced font 'Verdana,10'
set output 'output3.png'
set xtic 10
set  key left

plot [:120][:80000]'correct.txt' using 1:2 with points title 'userspace'  ,\
'' using 1:3 with points title 'kernelspace' ,\
'' using 1:4 with points title 'kernel to user' 
