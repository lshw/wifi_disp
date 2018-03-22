#ifndef __HT16C21_H__
#define __HT16C21_H__
#include <Wire.h>
#define HT1621 0x38
char ram_buf[10];
void send_ram();
void ht16c21_cmd(uint8_t cmd, uint8_t dat) {
  Wire.beginTransmission(HT1621);
  Wire.write(byte(cmd));
  Wire.write(byte(dat));
  Wire.endTransmission();
}
void ht16c21_setup() {
  Wire.begin(4, 5);
  ht16c21_cmd(0x84, 3); //1621文档20页 系统模式命令 开关ht1621时钟/显示  0-关闭  3-开启
  ht16c21_cmd(0x8A,B00110001);//LCD电压 后4位，16种电压 0000-关闭
  ht16c21_cmd(0x82,0);//LCD电压 后4位，16种电压 0000-关闭
}
void disp(char *str) {
  uint8_t dot, str0[22];
  const uint8_t _abcdefgh[20] = { //字符到7段码的对应
    B11111100, //0
    B01100000, //1
    B11011010, //2
    B11110010, //3
    B01100110, //4
    B10110110, //5
    B10111110, //6
    B11100000, //7
    B11111110, //8
    B11110110,//9
    B11101110,//11 A
    B11001110,//12 P
    B10011100,//13 C
    B00000000,//14 ' '
    B10011110,//15 'E'
    B10001110, //16 F
    B00000010, //17 '-'
    B01111100,  //18 'U'
    //B11111100 //19 'O';
  };
  const uint8_t convert[5 * 8] = { //高位是ram地址(0,3,4,7,8未用)，低位是ram字节的位，a-g是8字的7个段吗， h是小数点，
    //a     b     c     d     e    f      g     h
    0x51, 0x53, 0x52, 0x20, 0x22, 0x21, 0x23, 0x94, //第1位数字的段对应
    0x95, 0x97, 0x96, 0x64, 0x66, 0x65, 0x67, 0x50, //第2位数字的段对应
    0x61, 0x63, 0x62, 0x90, 0x92, 0x91, 0x93, 0x60, //第3位数字的段对应
    0x25, 0x27, 0x26, 0x54, 0x56, 0x55, 0x57, 0x24, //第4位数字的段对应
    0x11, 0x13, 0x12, 0x14, 0x16, 0x15, 0x17, 0x10  //第5位数字...
  };
  uint8_t dispbyte, ram, i, dian;
  Serial.print("disp [");
  Serial.print(str);
  Serial.println("]");
  i = strlen(str);
  if (i > 10)
    i = 10;
  ram = 0; //字符个数
  for (dian = 0; dian < i; dian++) {
    if (str[dian] != '.') ram++;
  }

  if (ram > 5) ram = 0;
  else ram = 5 - ram; //字符补' '个数， 有效字符起始位置

  memset(str0, ' ', 6);
  dot = 0;
  for (dian = 0; dian < i; dian++) {
    if (str[dian] != '.')  {
      str0[ram] = str[dian];
      str0[ram + 1] = 0;
      ram++;
      if (ram >= 5) break;
    } else {
      dot |= (1 << (4 - ram));
    }
  }

  //ht16c21_cmd(0x86,0); //内部刷新频率  0-80hz 1-160hz

  if (v < 3.6)
    ht16c21_cmd(0x88, 2); //0-不闪 1-2hz 2-1hz 3-0.5hz
  else
    ht16c21_cmd(0x88, 2); //0-不闪 1-2hz 2-1hz 3-0.5hz


  ram_buf[1] = 0;
  ram_buf[2] = 0;
  ram_buf[5] = 0;
  ram_buf[6] = 0;
  ram_buf[9] = 0;
  //0,3,4,7,8 未用
  for (i = 0; i < 5; i++) { //处理5个数字
    //dispbyte=str[i];//从高位开始取每一个数字
    dispbyte = str0[i];
    switch (dispbyte) {
      case 'A':
      case 'a':
        dispbyte = 10;
        break;
      case 'P':
      case 'p':
        dispbyte = 11;
        break;
      case 'C':
      case 'c':
        dispbyte = 12;
        break;
      case '-':
        dispbyte = 16;
        break;
      case ' ':
        dispbyte = 13;
        break;
      case 'E':
      case 'e':
        dispbyte = 14;
        break;
      case 'F':
      case 'f':
        dispbyte = 15;
        break;
      case 'U':
      case 'u':
        dispbyte = 18;
        break;
      default:
        dispbyte = dispbyte & 0xf;
    }
    dispbyte = _abcdefgh[dispbyte]; //转换成abcdefgh段码
    for (uint8_t i1 = 0; i1 < 8; i1++) {
      //处理每一个点，
      if ((dispbyte & (1 << (7 - i1))) != 0) {
        //此点是亮的，
        dian = convert[i * 8 + i1]; //转换map
        ram = dian >> 4; //ram地址是高位,0-9
        ram_buf[ram] |= (1 << (dian & 0xf)); //点的位置0-7
      }
    }
  }
  if (dot & 1)
    ram_buf[1] |= 1; //5p
  if (dot & 2)
    ram_buf[2] |= 0x10; //小数点固定 4p
  if (dot & 4)
    ram_buf[6] |= 1; //5p
  if (dot & 8)
    ram_buf[9] |= 0x10; //x1
  if (dot & 0x10)
    ram_buf[5] |= 1; //x2
  //0,3,4,7,8 未用
  send_ram();
}
void set_ram_check() {
  ram_buf[3] = 'L';
  ram_buf[4] = 'S';
  ram_buf[8] = 0x55 ^ ram_buf[0] ^ ram_buf[1] ^ ram_buf[2] ^ ram_buf[5] ^ ram_buf[6] ^ ram_buf[7] ^ ram_buf[9];
}
bool ram_check() {
  return  ram_buf[3] == 'L'
          && ram_buf[4] == 'S'
          && ram_buf[8] == (0x55 ^ ram_buf[0] ^ ram_buf[1] ^ ram_buf[2] ^ ram_buf[5] ^ ram_buf[6] ^ ram_buf[7] ^ ram_buf[9]);
}
bool load_ram() {
  Wire.beginTransmission(HT1621);//读
  Wire.write(byte(0x80));//
  Wire.write(byte(0)); //read ram
  Wire.endTransmission();
  Wire.requestFrom(HT1621, 10);
  Serial.print("LCD_RAM=");
  for (uint8_t i = 0; i < 10; i++) {
    ram_buf[i] = Wire.read();
    Serial.print(" ");
    if (ram_buf[i] < 0x10) Serial.write('0');
    Serial.print(ram_buf[i], HEX);
  }
  Wire.endTransmission();
  Serial.println();

  //0,3,4,7,8 校验
  return ram_check();
}
void send_ram() {
  uint8_t i;
  set_ram_check();
  Wire.beginTransmission(HT1621); // transmit to device #8
  Wire.write(byte(0x80));        // sends five bytes
  Wire.write(byte(0));              // sends one byte
  for (i = 0; i < 10; i++)
    Wire.write(ram_buf[i]);
  Wire.endTransmission();    // stop transmitting
}

#endif //__HT16C21_H__
