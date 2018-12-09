#ifndef __FS_H__
#define __FS_H__
extern String url;
String get_url() {
  File fp;
  if (url.length() != 0)
    return url;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/url.txt", "r");
    if (fp) {
      url = fp.readStringUntil('\n');
      url.trim();
      fp.close();
    } else
      Serial.println("/url.txt open error");
  } else
    Serial.println("SPIFFS begin error");
  Serial.print("载入url设置:");
  Serial.println(url);
  return url;
}
String get_ssid() {
  File fp;
  String ssid;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    if (fp) {
      ssid = fp.readString();
      fp.close();
    } else
      Serial.println("/ssid.txt open error");
  } else
    Serial.println("SPIFFS begin error");
  Serial.print("载入ssid设置:");
  Serial.println(ssid);
  return ssid;
}
#endif //__FS_H__
