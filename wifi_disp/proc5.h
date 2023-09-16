#ifndef __PROC5_H__
#define __PROC5_H__
//wifi定位仿真程序，存储，回放

struct ssid_mac {
  uint8_t rssi;
  uint8_t bssid[6];
  uint8_t channel;
  uint8_t encryptionType;
  bool hidden;
  char ssid[20];
};
bool ssid_comp(ssid_mac mac1, String ssid, uint8_t *bssid) {
  for (uint8_t i = 0; i < 6; i++) if (bssid[i] != mac1.bssid[i]) return false;
  if (String(mac1.ssid) != ssid) return false;
  return true;
}
#define MACS 10
void proc5_setup() {
  File fp;
  int8_t rc;
  struct ssid_mac macs[MACS];
  uint8_t offset = 0;
  uint8_t offset1 = 0;
  _myTicker.attach(1, timer1s);
  init1();
  if (power_in)
    disp(F("P5S-0"));
  else
    disp(F("P5L-0"));
  if (!SPIFFS.begin()) {
    delay(100);
    poweroff(2);
  }
  pinMode(DOWNLOAD_KEY, INPUT_PULLUP);
  ht16c21_cmd(0x88, 0); //0-不闪 1-2hz 2-1hz 3-0.5hz
  if (power_in) { //插电，是采样模式
    String ssid;
    int32_t rssi;
    uint8_t encryptionType;
    uint8_t *bssid;
    int32_t channel;
    bool hidden;
    uint8_t count = 0;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.println(F("wifi定位采集，请按一下download键保存数据"));
    ht16c21_cmd(0x88, 1); //0-不闪 1-2hz 2-1hz 3-0.5hz
    memset(macs, 0, sizeof(macs));
    bool download_key_last;
    while (1) {
      rc = WiFi.scanNetworks();
      for (uint8_t i = 0; i < rc; i++ ) {
        WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);
        for (uint8_t i0 = 0; i0 < MACS; i0++) {
          if (ssid_comp(macs[i0], ssid, bssid))  {
            macs[i0].channel = channel;
            macs[i0].rssi = rssi;
            macs[i0].encryptionType = encryptionType;
            macs[i0].hidden = hidden;
            ssid = "";
            break;
          }
        } //找已经存在的mac/ssid
        if (ssid == "") break;
        uint8_t mac_min = 0;
        if (macs[MACS - 1].ssid[0] != 0) { //满了找个最小的
          count = 9;
          for (uint8_t i0 = 1; i0 < MACS; i0++) {
            if (macs[i0].rssi < macs[mac_min].rssi) {
              mac_min = i0;
            }
          }
        } else {
          for (mac_min = 0; mac_min < MACS; mac_min++) if (macs[mac_min].ssid[0] == 0) break;
          count = mac_min + 1;
        }
        if (macs[mac_min].ssid[0] == 0 || macs[mac_min].rssi < rssi) { //保存数据
          strncpy(macs[mac_min].ssid, ssid.c_str(), sizeof(macs[0].ssid));
          macs[mac_min].encryptionType = encryptionType;
          memcpy( macs[mac_min].bssid, bssid, sizeof(macs[0].bssid));
          macs[mac_min].channel = channel;
          macs[mac_min].rssi = rssi;
          Serial.printf(PSTR("[%d] %02x:%02x:%02x:%02x:%02x:%02x %ddBm %s\r\n"), mac_min, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], rssi, ssid.c_str());
          if (count > 9) count = 9;
          snprintf_P(disp_buf, sizeof(disp_buf), PSTR("P5S-%d"), count);
          disp((char *) disp_buf);
        }
      }
      if (download_key_last != digitalRead(DOWNLOAD_KEY)) { //按键
        delay(100);
        if (download_key_last != digitalRead(DOWNLOAD_KEY)) {//防抖
          if (download_key_last == HIGH) {
            fp = SPIFFS.open("/ssids.txt", "w");
            if (fp) {
              disp(F("8.8.8.8.8."));
              fp.write((uint8_t *) &macs, sizeof(macs));
              fp.close();
              for (uint8_t i = 0; i < 10; i++) {
                yield();
                delay(100);
              }
            }
            memset(macs, 0, sizeof(macs));
            count = 0;
          }
          download_key_last = digitalRead(DOWNLOAD_KEY);
        }//防抖
      }//按键
    } //死循环
  } else { //不插电是仿真模式
    ht16c21_cmd(0x88, 1); //0-不闪 1-2hz 2-1hz 3-0.5hz
    fp = SPIFFS.open("/ssids.txt", "r");
    if (!fp) poweroff(2);
    rc = fp.read((uint8_t *) &macs, sizeof(macs));
    fp.close();
    if (rc <= 0) poweroff(2);
    Serial.printf(PSTR("载入ssids.txt,体积:%d\r\n"), rc);
    WiFi.mode(WIFI_AP_STA); //开AP
    while (1) {
      for (uint8_t i = 0; i < 10; i++) {
        if (macs[i].ssid[0] == 0) continue;
        Serial.printf(PSTR("[%d] %s %02x:%02x:%02x:%02x:%02x:%02x ch:%d\r\n"), i, macs[i].ssid,
                      macs[i].bssid[0],
                      macs[i].bssid[1],
                      macs[i].bssid[2],
                      macs[i].bssid[3],
                      macs[i].bssid[4],
                      macs[i].bssid[5],
                      macs[i].channel
                     );
        WiFi.softAP(String(macs[i].ssid), String(F("80118011")), macs[i].channel, false, 1, 10);
        wifi_set_macaddr(SOFTAP_IF, macs[i].bssid);
        snprintf_P(disp_buf, sizeof(disp_buf), PSTR("P5L-%d"), i);
        disp((char *) disp_buf);
        for (uint8_t i = 0; i < 20; i++) { //2秒换一个ssid
          yield();
          delay(100);
        }
      }
    }
  }
}
#endif //__PROC5_H__
