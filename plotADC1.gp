set term gif animate optimize
set output 'ADC1.gif'
do for[i=0:17:1] {
  time = sprintf("t=%d-%d[sec]", i*100, (i+1)*100)
  set title time
  set yrange [:44]
  plot 'ADC1.dat' index i w l notitle
}
