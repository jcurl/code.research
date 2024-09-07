OUTPUT_IMG='rpi4_8M.png'

set xlabel "Slice (bytes)"
set ylabel "GB/s"
set xrange [0:*]
set yrange [0:*]
set xtics 0, 32
set term pngcairo size 800,480
set output OUTPUT_IMG
set key noenhanced
plot 'rpi4_8M.txt' with lines title 'RPi4 Linux (RPiOS 5.3)', \
     'rpi4_qnx710_8M.txt' with lines title 'RPi4 QNX 7.1.0', \
     'rpi4_qnx800_8M.txt' with lines title 'RPi4 QNX 8.0.0'
