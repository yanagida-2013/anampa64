#include <iostream>
#include <fstream>

#define Timer_Tag 0x4000
#define Synchronize_Tag 0xffff
#define ADC1  (0x0001)
#define ADC2  (0x0001 << 1)
#define Both_Adc  0x0000
#define Either_Adc  0x8000

// 4バイトの始めの2バイトを取り出す
unsigned short upper(unsigned int x) {
  x = x >> 16;
  return((unsigned short)(x & 0x0000ffff));
}
// 4バイトの終わりの2バイトを取り出す
unsigned short lower(unsigned int x) {
  return((unsigned short)(x & 0x0000ffff));
}


// 2進数表示用の関数（テスト用）
void bit_disp(unsigned int dt) {
  int i, len;
  /* unsigned int のビット数をlenにセット */
  len = sizeof(dt) * CHAR_BIT;
  for (i = len - 1; i >= 0; i--){
  /* ビットの表示 */
    printf("%u", (dt>>i) & 0x0001);
  }
  printf("\n");
}

int main(int argc, char const *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: ./readList (raw MPA3 List file)\n";
    exit(2);
  }
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

  unsigned int buf;
  unsigned short header; //上側の2バイト
  std::cout << "Processing ...\n";
  while (!ifs.eof()) {
    ifs.read((char *)&buf, sizeof(buf));
    header = upper(buf);
    if (header == Timer_Tag) {  // 0x4000
      /*
      1[msec]ごとにタイマータグが書き込まれる。
      一番下からn番目のビットがADCnの信号を示す。
      値が1なら生存、0なら死亡
      */
      /*
      switch (lower(buf)) {
        case 0xffff:  // 1111111111111111, ADC1,2ともに生存
        break;
        case 0xfffe:  // 1111111111111110, ADC1のみ死亡
        break;
        case 0xfffd:  // 1111111111111101, ADC2のみ死亡
        break;
        case 0xfffc:  // 1111111111111100, ADC1,2ともに死亡
        break;
      }
      */
    } else if (header == Synchronize_Tag) {  // 0xffff
      /*
      タイマーイベントのあとに来る可能性があるイベントは、次の２つ。
      - タイマーイベント      : 0x4000fff?
      - シンクロナイズドマーク : 0xffffffff
      シンクロナイズドマークの次にはイベントデータが続く。

      イベントデータは4バイトのシグナルからスタートする。
      - 前2バイトは幾つかのフラグが乗っており、識別するために30番目のビットが0になっている。
      - 後2バイトはTimerTagと同様にADCの生存死亡が表記される。
      */
    } else if (header == Both_Adc) {  // 0x0000
      /*
      両方のADCが来たというフラグ0x0000の場合である。
      データが続くのでここで読み込んでしまう。

      ADCのデータは4バイトである。
      ADCが偶数個である場合は問題なく、8バイトのデータ列に
      (ADC1, ADC2), (ADC3, ADC4)
      と書き込まれていく。
      ADCが奇数個である場合は最後の4バイトにはダミーが入る。
      つまり、ADCを３つ使っている場合は、
      (ADC1, ADC2), (ADC3, dummy)
      となる。

      今回はTOFとPHのデータ２つなので１度だけ4バイト読み込む。
      */
      ifs.read((char *)&buf, sizeof(buf));
      // printf("ph  = %d\n", upper(buf));
      // printf("tof = %d\n", lower(buf));
    } else if (header == Either_Adc) {
      /*
      ADCどちらかしか来ていない場合。
      つまりPHかTOFのどちらかしか来ていない。
      NIMモジュールでcoincidenceを取っているからありえないけど、
      来てしまっているから仕方がないので、4バイト読み込んで進めておく。
      */
      ifs.read((char *)&buf, sizeof(buf));
    } else {
      std::cout << "Invalid header\n";
      printf("%8x\n", buf);
    }
  }
  ifs.close();

  return 0;
}

