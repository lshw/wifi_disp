#include <FS.h>
#define VER "1.9"
#define HOSTNAME "disp_"
extern "C" {
#include "user_interface.h"
}
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
char disp_buf[22];
uint32_t next_disp = 120; //下次开机
String hostname = HOSTNAME;
float v;
uint8_t proc; //用lcd ram 0 传递过来的变量， 用于通过重启，进行功能切换
//0,1-正常 2-AP 3-OTA  4-http update
#define AP_MODE 2
#define OTA_MODE 3
#define OFF_MODE 4

#include "ota.h"
#include "ds1820.h"
#include "wifi_client.h"
#include "ap_web.h"
#include "ht16c21.h"
#include "http_update.h"

bool power_in = false;
void setup()
{
  uint8_t i;
  Serial.begin(115200);
  Serial.println();
  Serial.println("Software Ver=" VER);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  Serial.println("Hostname: " + hostname);
  delay(100);
  Serial.flush();
  if(!ds_init() && !ds_init()) ds_init();
  ht16c21_setup();
  get_batt();
  Serial.print("电池电压");
  Serial.println(v);
  if(power_in) Serial.println("外接电源");
  if(ram_buf[7]&1) Serial.println("充电中");
  ht16c21_cmd(0x84,3);  //0-关闭  3-开启
  if (!load_ram() && !load_ram() && !load_ram()) {
    ram_buf[0] = 0xff; //读取错误
    ram_buf[7] = 0; // 1 充电， 0 不充电
  }
  proc = ram_buf[0];
  switch (proc) {
    case OFF_MODE: //OFF
      wdt_disable();
      ram_buf[0] = 0;
      disp(" OFF ");
      delay(5000);
      disp("     ");
      ht16c21_cmd(0x84,0x02);//关闭ht16c21
      poweroff(0);
      return;
      break;
    case OTA_MODE:
      wdt_disable();
      ram_buf[7]|=1; //充电
      ram_buf[0] = OFF_MODE;//ota以后，
      disp(" OTA ");
      break;
    case AP_MODE:
      ram_buf[7]|=1; //充电
      ram_buf[0] = OTA_MODE; //ota
      send_ram();
      AP();
      return;
      break;
    default:
      ram_buf[0] = AP_MODE;
      sprintf(disp_buf, " %3.2f ", v);
      disp(disp_buf);
      break;
  }
  send_ram();
  //更新时闪烁
  ht16c21_cmd(0x88, 1); //闪烁

  if (wifi_connect() == false) {
    if(proc == OTA_MODE || proc == OFF_MODE) {
      ram_buf[0]=0;
      send_ram();
      ESP.restart(); 
    }
    ram_buf[9] |= 0x10; //x1
    ram_buf[0] = 0;
    send_ram();
    Serial.print("不能链接到AP\r\n20分钟后再试试\r\n本次上电时长");
    Serial.print(millis());
    Serial.println("ms");
    poweroff(1200);
    return;
  }
  if(temp==85.00 || temp<=-300){
    delay(1000);
    get_temp();
    if(temp==85.00){
      ds.reset();
      ds.select(dsn);
      ds.write(0x44, 1);
      delay(1000);
      get_temp();
    }
  }
  ht16c21_cmd(0x88, 0); //停止闪烁
  if (proc == AP_MODE) return;
  if (proc == OTA_MODE) {
    ota_setup();
    return;
  }
  if (proc == OFF_MODE) {
    if (http_update() == true) {
      ram_buf[0] = 0;
      send_ram();
      disp("HUP O");
      delay(2000);
    }

    ESP.restart();
    return;
  }
  uint16_t httpCode = http_get();
  if (httpCode >= 400) {
    Serial.print("不能链接到web\r\n20分钟后再试试\r\n本次上电时长");
    Serial.print(millis());
    Serial.println("ms");
    poweroff(1200);
    return;
  }
  if (v < 3.6)
    ht16c21_cmd(0x88, 2); //0-不闪 1-2hz 2-1hz 3-0.5hz
  else
    ht16c21_cmd(0x88, 0); //0-不闪 1-2hz 2-1hz 3-0.5hz
  Serial.print("uptime=");
  Serial.print(millis());
  if (next_disp == 0) next_disp = 120;
  Serial.print("ms,sleep=");
  Serial.println(next_disp);
  poweroff(next_disp);
}
bool power_off = false;
void poweroff(uint32_t sec) {
  if(v>4.17){
    ram_buf[7]&=~1;
    send_ram();
  }
  if (ram_buf[7] & 1) {
    digitalWrite(13, LOW);
  } else {
    digitalWrite(13, HIGH);
  }
  if (power_in) { //如果外面接了电， 就进入LIGHT_SLEEP模式 电流0.8ma， 保持充电
    sec = sec / 2;
    wifi_set_sleep_type(LIGHT_SLEEP_T);
    Serial.print("休眠");
    if (sec > 60) {
      Serial.print(sec / 60);
      Serial.print("分钟");
    }
    Serial.print(sec % 60);
    Serial.println("秒");
    if (ram_buf[7] & 1){
      digitalWrite(13,LOW);
      Serial.println("充电中");
    }
    for(uint32_t i=0;i<sec;i++) {
    delay(1000); //空闲时进入LIGHT_SLEEP_T模式
    system_soft_wdt_feed ();
  }
}
  if (ram_buf[7] & 1)
    Serial.println("充电结束");
  Serial.print("关机");
  if (sec > 60) {
    Serial.print(sec / 60);
    Serial.print("分钟");
  }
  Serial.print(sec % 60);
  Serial.println("秒");
  Serial.println("bye!");
  Serial.flush();
  wdt_disable();
  system_deep_sleep_set_option(0);
  digitalWrite(LED_BUILTIN, LOW);
  if(sec==0) ht16c21_cmd(0x84,0x2); //lcd off
  ESP.deepSleep((uint64_t) 1000000 * sec, WAKE_RF_DEFAULT);
  //system_deep_sleep((uint64_t)1000000 * sec);
  power_off = true;
}
float get_batt(){
  float v0;
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH); //不充电
  delay(1);
  get_batt0();
  v0 = v;
  digitalWrite(13, LOW); //充电
  delay(1);
  get_batt0();
  if (v > v0) { //有外接电源
    v0 = v;
    digitalWrite(13, HIGH); //不充电
    delay(1);
    get_batt0();
    if (v0 > v){
      v0 = v;
      digitalWrite(13, LOW); //充电
      delay(1);
      get_batt0();
      if(v > v0){
	v0 = v;
	digitalWrite(13, HIGH); //不充电
	delay(1);
	get_batt0();
	if (v0 > v){
	  if(!power_in) {
	    power_in = true;
	    Serial.println("测得电源插入");
	  }
	}else power_in=false;
      }else power_in=false;
    }else power_in = false;
  }else power_in = false;

  if(v < 3.8)
    ram_buf[7] |= 1;
  else if(v > 4.17)
    ram_buf[7] &= ~1;
  set_ram_check();
  if(ram_buf[7] & 1)
    digitalWrite(13,LOW);
  else
    digitalWrite(13,HIGH);
  return v;
}
float get_batt0() {//锂电池电压
  uint32_t dat = analogRead(A0);
  dat = analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0);
  v = dat / 8 * (499 + 97.6) / 97.6 / 1023 ;
  return v;
}
void loop()
{
  if (power_off) return;
  switch (proc) {
    case OTA_MODE:
      ota_loop();
      break;
    case AP_MODE:
      digitalWrite(13,LOW);
      ap_loop();
      break;
  }
}
