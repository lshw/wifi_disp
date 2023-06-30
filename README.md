# wifi_disp
基于arduino开发环境，硬件使用esp8266做的一个智能硬件，可以arduino用串口更新程序，或者arduino用wifi ota方式更新程序

内带锂电池， 充满电， 可以工作40天

可以用wifi从web服务器获取5个数字，显示在数码管显示屏上，

带了个AP功能，让笔记本和手机可以接上来用内带的web服务器进行初始设置。

在连上外网后，还能进行自动远程http升级固件。  
本身带一个温度探头， 精度0.5度， 数据可以传到远程http服务器上进行存储。  
我现在用它给家里的室温和室外温度进行测试并记录，还用它作为美元汇率显示器。  
这个项目也可以作为esp8266开发的一个入门例子。  
源码在wifi_disp目录下。   
编译好的bin在 https://www.anheng.com.cn/wifi_disp.bin  

lib目录下的脚本，是linux下，不用arduino集成环境， 直接用命令行进行编译或者ota或者线刷的脚本

lora_mini是一个兼容arduino的uno，带lora功能的一个小板子，可以作为lora的探头端  

 V2版硬件,增加lora模块，cfido2019同款, 可以作为lora开发板学习研究lora， 也可以作为lora网关， 让lora网络覆盖一个住宅小区。  目前稳定测试通讯距离可以1.3公里  

这里是taobao链接， 提供全套散件，这个小小的套件， 可以体验学习如下诸多内容: 嵌入式开发，wifi开发, arduino,http开发，制作ap接入点，物联网应用，3D打印外壳，基于单片机dns泛域名解析， mdns发现，基于单片机的web服务器，好多内容， 适合于大中小学生玩耍,随便从以上各点取一个方向，做毕业设计是很好的。 

相关的项目链接:  
微信小程序[惠多气象]:https://github.com/scfido/weather_thermometer  
手机APP:https://github.com/liuxiaoyang98/wifi_disp_app  

V1无lora功能:https://item.taobao.com/item.htm?id=583649099297    
V2有lora功能:https://item.taobao.com/item.htm?id=601842691823  

看看能打到第几关:  

第一关，可以拿它当个温度计，可以Web看历史温度，~~小程序看温度~~

~~第二级，可以自动开通ddns.cfido.com,自己玩一些ddns的东西。~~

第三级，可以自己开发Web服务端，让它周期显示一些自己感兴趣的内容,5位数字

第四级，扩展的TTL串口和I2C，可以自己扩展一些外设

第五级，进行lora无线通讯开发，使用arduino开发环境
模块支持3种通讯模式，2种传统的，和lora模式，频率433-470MHz（433,470是开放频率，可以进行智能家居，物联网等非连续通讯)

第六级，我们要建立开放的基于lora的，低功耗的,低速物联网通讯协议，用无中心的p2p网络实现互联。

编译方法：  
先安装arduino开发环境， 然后增加esp8266支持, 具体就是开发环境下点设置， 增加开发板管理器的url:https://arduino.esp8266.com/stable/package_esp8266com_index.json   
然后在开发板管理器里就可以搜到并安装esp8266的支持了，目前的源码需要选择3.0.2的esp8266支持环境  
在开发板里选ESPDuino(ESP13 Module)  
MMU选16KB cache 48KB IRAM  
就可以编译了。 
