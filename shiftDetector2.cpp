/*
written by s.yanagida 2014/11/23
compile: $ g++ shiftDetector2.cpp -o shiftDetector
$ g++ --version => 4.2.1
$ g++ --version => 4.9

for 2 ADCs,
to generate gnuplot input with one file.

- Edit the last paragraph to generate specific gnuplot input
- Edit CHANNEL in the case of "Segmentation fault"
*/


#include <iostream>
#include <fstream>

#define Timer_Tag 0x4000
#define Synchronize_Tag 0xffff
#define Both_Adc  0x0000

#define CHANNEL 2048

unsigned short upper(unsigned int x) {
  x = x >> 16;
  return((unsigned short)(x & 0x0000ffff));
}
unsigned short lower(unsigned int x) {
  return((unsigned short)(x & 0x0000ffff));
}


int main(int argc, char const *argv[]) {
  if (argc != 3) {
    std::cout << "Usage: ./readList (raw MPA3 List file) (interval[sec])\n";
    exit(2);
  }

  // List File open
  std::ifstream ifs(argv[1], std::ios::in | std::ios::binary);
  if (ifs.fail()) {
    std::cout << "Couldn't open " << argv[1] << std::endl;
    exit(2);
  }
  std::cout << "Seeking Binary data ...\n";
  char d;
  while (true) {
    std::string buf;
    while (true) {
      ifs.read((char*)&d, sizeof(char));
      buf += d;
      if (strcmp(&d, "\n") == 0) break;
    }
    if (buf == "[LISTDATA]\r\n") break;
  }

  // Interval
  int interval = atoi(argv[2]);
  if (interval <= 0 ) {
    std::cout << "Interval is invalid.\n";
    exit(2);
  }
  std::cout << "Interval : " << interval << "[sec]\n";


  // Process
  unsigned int buf;
  unsigned short header;
  int msec = 0, sections = 0;
  int adc1[CHANNEL];
  int adc2[CHANNEL];
  for (int i = 0; i<CHANNEL; i++) {
    adc1[i] = 0;
    adc2[i] = 0;
  }
  int adc1max = 0, adc2max = 0;
  std::ofstream ofsadc1("ADC1.dat", std::ios::out);
  std::ofstream ofsadc2("ADC2.dat", std::ios::out);
  while (!ifs.eof()) {
    ifs.read((char *)&buf, sizeof(buf));
    header = upper(buf);
    if (header == Timer_Tag) {
      if (lower(buf) == 0xffff) {
        msec++;
        if (msec % (interval*1000) == 0) {
          ofsadc1 << "# Time : " << sections*interval << " - " << (sections+1)*interval << " [sec]" << std::endl;
          ofsadc2 << "# Time : " << sections*interval << " - " << (sections+1)*interval << " [sec]" << std::endl;
          for (int i = 0; i<CHANNEL; i++) {
            ofsadc1 << adc1[i] << std::endl;
            ofsadc2 << adc2[i] << std::endl;
            if (adc1max < adc1[i]) adc1max = adc1[i];
            if (adc2max < adc2[i]) adc2max = adc2[i];
          }
          ofsadc1 << std::endl << std::endl;
          ofsadc2 << std::endl << std::endl;
          for (int i = 0; i<CHANNEL; i++) {
            adc1[i] = 0;
            adc2[i] = 0;
          }
          sections++;
        }
      }
    } else if (header == Both_Adc) {
      ifs.read((char *)&buf, sizeof(buf));
      adc1[upper(buf)]++;
      adc2[lower(buf)]++;
    }
  }
  ifs.close();
  ofsadc1.close();
  ofsadc2.close();
  std::cout << "Divided into " << sections << " sequence." << std::endl;

  // brew gnuplot input
  std::cout << "Generating gnuplot input." << std::endl;
  std::ofstream gnuplotInputOfADC1("plotADC1.gp", std::ios::out);
  gnuplotInputOfADC1
  << "set term gif animate optimize" << std::endl
  << "set output 'ADC1.gif'" << std::endl
  << "do for[i=0:" << sections-1 << ":1] {" << std::endl
  << "  time = sprintf(\"t=%d-%d[sec]\", i*" << interval << ", (i+1)*" << interval << ")" << std::endl
  << "  set title time" << std::endl
  << "  set yrange [:" << adc1max << "]" << std::endl
  << "  plot 'ADC1.dat' index i w l notitle" << std::endl
  << "}" << std::endl;
  gnuplotInputOfADC1.close();
  std::ofstream gnuplotInputOfADC2("plotADC2.gp", std::ios::out);
  gnuplotInputOfADC2
  << "set term gif animate optimize" << std::endl
  << "set output 'ADC2.gif'" << std::endl
  << "do for[i=0:" << sections-1 << ":1] {" << std::endl
  << "  time = sprintf(\"t=%d-%d[sec]\", i*" << interval << ", (i+1)*" << interval << ")" << std::endl
  << "  set title time" << std::endl
  << "  set yrange [:" << adc2max << "]" << std::endl
  << "  plot 'ADC2.dat' index i w l notitle" << std::endl
  << "}" << std::endl;
  gnuplotInputOfADC2.close();
  std::system("gnuplot plotADC1.gp");
  std::system("gnuplot plotADC2.gp");
  return 0;
}
