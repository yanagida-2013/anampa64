set term gif animate optimize
set output 'ADC2.gif'
do for[i=0:17:1] {
  time = sprintf("t=%d-%d[sec]", i*100, (i+1)*100)
  set title time
  set yrange [:61]
  plot 'ADC2.dat' index i w l notitle
}
