#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VER "1.82"
#define HOSTNAME "disp_"
#define CRC_MAGIC 3
/* ESP32_C3 MAGIC 4 */
#define DEFAULT_URL0 "http://temp.cfido.com:808/wifi_disp.php"
#define DEFAULT_URL1 "http://temp2.wf163.com:808/wifi_disp.php"

#ifndef GIT_VER
#define GIT_VER "test"
#endif
#define DOWNLOAD_KEY 0
#ifndef BUILD_SET
#define BUILD_SET "default"
#endif

#endif //_CONFIG_H_
