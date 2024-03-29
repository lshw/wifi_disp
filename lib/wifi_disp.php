<?php

/* 本段为注释，不会执行
 最基本的,只需要3行代码就行了,显示当前温度，然后数据存到 /tmp/data.csv
echo "$_GET[temp],720";
$t = date('Y-m-d H:i:s'); //2023-07-02 15:52:00
file_put_contents("/tmp/data.csv", "$t,$_GET[temp],$_GET[shidu]%\r\n", FILE_APPEND);
注释结束 */
error_reporting(E_ALL ^ E_NOTICE);
ignore_user_abort(true);//客户端断开链接后继续执行php程序。
ob_start();
switch ($_GET['type']) {
    case 'loadavg': //服务器端的平均负载
        $loadavg = file_get_contents("/proc/loadavg");
        $loadavg = explode(' ', file_get_contents('/proc/loadavg'));
        echo "$loadavg[0],360";//探头显示loadavg,并且1小时后再次上线
        break;
    case 'USD_CNY':
        $hl = api("https://open.er-api.com/v6/latest/USD", "rates", "CNY");
        $hl = round($hl, 3);
        echo "$hl,360";
        break;
    default:
        switch ($_GET['sn']) {
            case 'disp_4d1230':
                $hl = api("https://open.er-api.com/v6/latest/USD", "rates", "CNY");
                $hl = round($hl, 3);
                echo "$hl,360";
                break;
            default:
                if ($_GET['shidu'] != '') {
                    printf("%02d-%02d,360", round($_GET['temp'], 0), round($_GET['shidu'], 0));
                } else {
                    echo "$_GET[temp],720"; //显示温度，2小时(720*10s)后再上线。
                }
                break;
        }
        break;
}
$size=ob_get_length();
header('Connection: close');//不需要持续链接
header("Content-Length: $size");  //明确告诉浏览器数据长度,浏览器接收到此长度数据后就不再接收数据
ob_end_flush();
//断开链接后，再保存数据，让探头尽快休眠，节省探头电量。
date_default_timezone_set('Asia/ShangHai');
$t=date('Y-m-d H:i:s'); //2023-07-02 15:52:00
file_put_contents("/tmp/data.csv", "$t,$_GET[sn],$_GET[temp],$_GET[shidu]%\r\n", FILE_APPEND);
exit();

function api($url, $field, $field2 = '')
{
//可以通过其它网站的api获取json格式的数据， 展示到探头上。第一个参数是url ,
//返回值: $ar[$field] 或者$ar[$field][$field2]
    $urlmd5 = md5($url);
    $cache_file="/tmp/$urlmd5.txt";
    $a=array();
    if (file_exists($cache_file) and filemtime($cache_file) > time() - 1800) { //结果本地缓存半>小时
        $str = file_get_contents("/tmp/${urlmd5}.txt");
    } else {
        $str = file_get_contents($url);
        file_put_contents("/tmp/${urlmd5}.txt", $str);
    }
        $a = json_decode($str, true);
    if (empty($a)) {
        return;
    }
    if ($field2 != '') {
            return $a[$field][$field2];
    } else {
        return $a[$field];
    }
}
