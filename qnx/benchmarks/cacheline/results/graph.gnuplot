set xlabel "Slice (bytes)"
set ylabel "GB/s"
set xrange [0:*]
set yrange [0:*]
set xtics 0, 32
set term pngcairo size 800,480
set output OUTPUT_IMG
set key noenhanced
plot INPUT_DATA with lines title INPUT_DATA
