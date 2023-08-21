#ifndef __HT16C21_H__
#define __HT16C21_H__
#include <Wire.h>
#define HT1621 0x38
char ram_buf[10];
void send_ram();
void load_ram();
void charge_off();
void charge_on();
extern bool power_in;
void ht16c21_cmd(uint8_t cmd, uint8_t dat) {
  Wire.beginTransmission(HT1621);
  Wire.write(byte(cmd));
  Wire.write(byte(dat));
  if (Wire.endTransmission() != 0) {
    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);
    delay(80);
    for (uint8_t i = 0; i < 10; i++) {
      digitalWrite(5, HIGH);
      delay(1);
      digitalWrite(5, LOW);
      delay(1);
    }
    delay(80);
    Wire.begin(4, 5);
    Wire.beginTransmission(HT1621);
    Wire.write(byte(cmd));
    Wire.write(byte(dat));
    Wire.endTransmission();
  }
}
void ht16c21_setup() {
  Wire.begin(4, 5);
  load_ram();
  ht16c21_cmd(0x84, 3); //1621文档20页 系统模式命令 开关ht1621时钟/显示  0-关闭  3-开启
  ht16c21_cmd(0x8A, B00110001); //LCD电压 后4位，16种电压 0000-关闭
  ht16c21_cmd(0x82, 0); //LCD电压 后4位，16种电压 0000-关闭
}
void disp(char *str) {
  uint8_t dot, str0[22];
  const uint8_t convert[5 * 8] = { //高位是ram地址(0,3,4,7,8未用)，低位是ram字节的位，a-g是8字的7个段吗， h是小数点，
    //a     b     c     d     e    f      g     h
    0x51, 0x53, 0x52, 0x20, 0x22, 0x21, 0x23, 0x94, //第1位数字的段对应
    0x95, 0x97, 0x96, 0x64, 0x66, 0x65, 0x67, 0x50, //第2位数字的段对应
    0x61, 0x63, 0x62, 0x90, 0x92, 0x91, 0x93, 0x60, //第3位数字的段对应
    0x25, 0x27, 0x26, 0x54, 0x56, 0x55, 0x57, 0x24, //第4位数字的段对应
    0x11, 0x13, 0x12, 0x14, 0x16, 0x15, 0x17, 0x10  //第5位数字...
  };
  uint8_t dispbyte, ram, i, dian;
  if (nvram.pcb_ver == 1) {
    charge_off();
    Serial.begin(115200);
    Serial.println();
  }
  Serial.printf_P(PSTR("disp [%s]\r\n"), str);
  if (nvram.pcb_ver == 1 && power_in) {
    Serial.flush();
    Serial.end();
    charge_on();
  }
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

  ram_buf[1] = 0;
  ram_buf[2] = 0;
  ram_buf[5] = 0;
  ram_buf[6] = 0;
  ram_buf[9] = 0;
  //0,3,4,7,8 未用
  for (i = 0; i < 5; i++) { //处理5个数字
    switch (str0[i]) {
      case '0':
      case 'O':
      case 'o': dispbyte = B11111100; break;
      case '1': dispbyte = B01100000; break;
      case '2': dispbyte = B11011010; break;
      case '3': dispbyte = B11110010; break;
      case '4': dispbyte = B01100110; break;
      case 's':
      case 'S':
      case '5': dispbyte = B10110110; break;
      case '6': dispbyte = B10111110; break;
      case '7': dispbyte = B11100000; break;
      case '8': dispbyte = B11111110; break;
      case '9': dispbyte = B11110110; break;
      case 'A':
      case 'a': dispbyte = B11101110; break;
      case 'C':
      case 'c': dispbyte = B10011100; break;
      case 'E':
      case 'e': dispbyte = B10011110; break;
      case 'F':
      case 'f': dispbyte = B10001110; break;
      case 'H':
      case 'h': dispbyte = B01101110; break;
      case 'L':
      case 'l': dispbyte = B00011100; break;
      case 'P':
      case 'p': dispbyte = B11001110; break;
      case 'T':
      case 't': dispbyte = B00011110; break;
      case 'U':
      case 'u': dispbyte = B01111100; break;
      case ' ': dispbyte = B00000000; break;
      case '-': dispbyte = B00000010; break;
      default:
        dispbyte = B00000000;
    }
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
  //if (dot & 0x10)
  //  ram_buf[5] |= 1; //x2
  //0,3,4,7,8 未用
  send_ram();
}
void load_ram() {
  Wire.beginTransmission(HT1621);//读
  Wire.write(byte(0x80));//
  Wire.write(byte(0)); //read ram
  Wire.endTransmission();
  Wire.requestFrom(HT1621, 10);
  Serial.print(F("LCD_RAM="));
  for (uint8_t i = 0; i < 10; i++) {
    ram_buf[i] = Wire.read();
    Serial.write(' ');
    if (ram_buf[i] < 0x10) Serial.write('0');
    Serial.print(ram_buf[i], HEX);
  }
  Wire.endTransmission();
  Serial.println();
}
void send_ram() {
  uint8_t i;
  if (power_in) //外接电源，点亮左上角的三角形
    ram_buf[9] |= 0x10;
  else
    ram_buf[9] &= ~0x10;
  if (ram_buf[7] & 1) //需要充电， 点亮左下角的三角形
    ram_buf[5] |= 1; //x2
  else
    ram_buf[5] &= ~1; //x2
  Wire.beginTransmission(HT1621); // transmit to device #8
  Wire.write(byte(0x80));        // sends five bytes
  Wire.write(byte(0));              // sends one byte
  for (i = 0; i < 10; i++)
    Wire.write(ram_buf[i]);
  Wire.endTransmission();    // stop transmitting
}

#endif //__HT16C21_H__
