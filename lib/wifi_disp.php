<?php
/*
wifi_disp.php?type=usd_rub 美元到卢布
wifi_disp.php?type=Doller 人民币到美元
wifi_disp.php?type=USD 人民币到美元
wifi_disp.php?type=JPN 人民币到日元
wifi_disp.php?type=BTC 比特币到美元
wifi_disp.php?type=szxxxxxx 深证某股票
wifi_disp.php?type=shxxxxxx 上证某股票
其它要求，请自行修改下面的代码:
*/
ignore_user_abort(false);
ob_start();
register_shutdown_function("toexit"); //退出时执行这个函数

switch ($_GET['type']) {
    case 'usd_rub':
        usd_to_xxx('RUB'); //美元到卢布
        break;
    case 'meiyuan':
    case 'my':
    case '$':
    case 'dollar':
    case 'Dollar':
        cny_to_xxx("USD"); //人民币to美元
        break;
    case 'USD':
    case 'JPY':
    case 'JPY1':
        cny_to_xxx($_GET['type']); //人民币to美元 人民币to 日元
        break;
    case 'BTC':
        usd_to_xxx($_GET['type']); //美元toBTC
        break;
    default:
        $gp_name=strtolower($_GET['type']);
        $gp_name2=substr($gp_name, 0, 2);
        if (($gp_name2 == 'sh' or $gp_name2=='sz') and is_numeric(substr($gp_name, 2))) {
            stock($gp_name);
            break;
        }
        switch ($_GET['sn']) {
            case 'disp_bc2a5a':
                cny('JPY');
                break;
            case 'disp_9468f6':
                usd("BTC");
                break;
            case 'disp_b48016': //大屏
                $h=date('H');
                if($h=='7' or $h=='8')
                  wendu('disp_8cbd7f'); //7:00-8:59,显示另一个室外温度
                else
                  echo "$_GET[temp],360";
                break;
            default:
                $toff=360;
                if ($_GET['shidu'] != '') {
                    printf("%02d-%02d,%d", round($_GET['temp'], 0), round($_GET['shidu'], 0), $toff);
                } else {
                    echo "$_GET[temp],$toff";
                }
                break;
        }
}

function wendu($disp)
{
    if (file_exists("/tmp/disp/$disp.txt")) {
        $wd=explode(' ', file_get_contents("/tmp/disp/$disp.txt"));
        $c=trim($wd[2]);
        $t=strtotime("$wd0 $wd1")+1840-time();
        if ($t<1000) {
            $t=1840;
        }
        $t=$t/10;
        echo "$c,$t";
    } else {
        echo "00000,180";
    }
    return;
}

function usd_to_xxx($type = 'JPY1', $sec = '360')
{
    switch ($type) {
        case 'BTC':
            $disp=round(api("https://api.coincap.io/v2/assets/bitcoin", "data", "priceUsd"), 0);
            break;
        case 'JPY1': //CNY->JPY
            $disp=round(100.0/api("https://open.er-api.com/v6/latest/USD", "rates", "JPY"), 3);
            break;
        default:
            $disp=round(api("https://open.er-api.com/v6/latest/USD", "rates", $type), 3);
    }
    if ($disp < 0.1) {
        $disp = $disp * 100;
    }
    if ($disp < 10) {
            echo " $disp,$sec";
    } else {
        echo "$disp,$sec";
    }
}
function cny_to_xxx($type = 'USD', $sec = '360')
{
    switch ($type) {
        case 'BTC':
            $disp=api("https://api.coincap.io/v2/assets/bitcoin", "data", "priceUsd");
            break;
        case 'USD': //USD->CNY
            $disp=round(1/api("https://open.er-api.com/v6/latest/CNY", "rates", "USD"), 3);
            break;
        case 'JPY1': //CNY->JPY
            $disp=round(100.0/api("https://open.er-api.com/v6/latest/CNY", "rates", "JPY"), 3);
            break;
        default:
            $disp=round(api("https://open.er-api.com/v6/latest/CNY", "rates", $type), 3);
    }
    if ($disp < 0.1) {
        $disp = $disp * 100;
    }
    if ($disp < 10) {
            echo " $disp,$sec";
    } else {
        echo "$disp,$sec";
    }
}

function get_str(& $string)
{
//解决一下字符串的注入问题
    global $_getstr;
    if ($_getstr["_$string"]==1) {
        return $string ; //已经处理过的
    }
    $string=strtr($string, array('..'=>' ','"'=>' ',"'"=>' ',"\t"=>' ','\r'=>' ','\n'=>' '));
/* '１'=>'1','２'=>'2','３'=>'3','４'=>'4','５'=>'5','６'=>'6','７'=>'7','８'=>'8','９'=>'9','０'=>'0','－'=>'-')); */
    $string=addslashes($string);

    $_getstr["_$string"]=1;
    return $string;
}

function toexit()
{

    global $datadir, $datafile, $real_file_dir_base, $ah_office;
    header('Connection: close');
    header('Content-Encoding: none');
    ignore_user_abort(false);
    if (function_exists("fastcgi_finish_request")) {
        fastcgi_finish_request();
    }
    $size=ob_get_length();
    header("Content-Length: $size");  //告诉浏览器数据长度,浏览器接收到此长度数据后就不再接收数据
    ob_end_flush();
    ob_flush();
    flush();
/*

//保存温度湿度到log文件
    if (get_str($_GET['sn'])!='' && get_str($_GET['ssid'])!='') {
      check_mkdir("data");
      $datafile="data/".date("Y_m_d").".log";
      file_put_contents($datafile, trim(date('H:i:s')."\t$_GET[batt]\t$_GET[temp]\t$_GET[charge]\t$_GET[power]\t$_GET[rssi]\t$_GET[shidu]")."\r\n", FILE_APPEND);
    }
*/
}

function api($url, $field, $fieldx = '')
{
 //可以通过其它网站的api获取json格式的数据， 展示到探头上。
    $urlmd5 = md5($url);
    $cache_file="/tmp/$urlmd5.txt";
    $a=array();
    if (file_exists($cache_file) and filemtime($cache_file) > time() - 1800) { //结果本地缓存半小时
        $str = file_get_contents("/tmp/${urlmd5}.txt");
    } else {
        $str = file_get_contents($url);
        if($str == '') {
        $str = file_get_contents($url);
        }
        if($str != '') {
        file_put_contents("/tmp/${urlmd5}.txt", $str);
        }
    }
        $a = json_decode($str, true);
    if (empty($a)) {
        return;
    }
    if ($fieldx != '') {
        return $a[$field][$fieldx];
    } else {
        return $a[$field];
    }
}
 
function check_mkdir($dbname)
{
    if (file_exists($dbname)) {
        return;
    }
    mkdir($dbname);
    chmod($dbname, 0777);
}

function stock($gp_name)
{
    $cache_file="/tmp/gp_${gp_name}.txt";
    $week=date('w');
    //根据开市时间，计算股票下次更新时间
    $t8=strtotime(date('Y-m-d 08:00:00'));
    $t15=strtotime(date('Y-m-d 15:00:00'));
    if (time() < $t8) { //未开市
        $next=$t8 - time() + 30;
        if ($week==6) {
            $next +=3600*24*2;
        } else if ($week == 0) {
            $next += 3600*24;
        }
    } else if (time() < $t15) { //开市中
        if ($week ==6) {
            $next=$t8+3600*24*2 -time()+ 30; //第三天开市
        } else if ($week == 0) {
            $next=$t8+3600*24 -time()+ 30; //第二天开市
        } else {
            if (time()+600 < $t15) { //休市在半小时以后
                 $next=600;
            } else {
                $next = $t15 - time() +60; //休市20秒后更新数据
            }
        }
    } else {
        //休市后
        $next=$t8+3600*24 -time()+ 30; //第二天开市
        if ($week == 5) {
            $next+=3600*24*2;// 周五
        } else if ($week == 6) {
            $next+=3600*24;
        }
    }
        $h=date('H');
    if ($h < 8) {
        $next=$t8-strtotime(date('Y-m-d 00:00:00'));
    }
        //$next 是下次上线需要延迟的秒数， 已经根据周末和开市时间做了调整。
    $next=round($next/10);
        //根据休市时间，计算股票更新时间
    if ($week == 6) {
        $last=strtotime(date('Y-m-d 15:00:00', time() - 3600*24));
    } else if ($week ==0) {
        $last=strtotime(date('Y-m-d 15:00:00', time() - 3600*24 *2));
    } else {
        $last=strtotime(date('Y-m-d 15:00:00', time()));
        if ($last > time()) {
            $last = time();
        }
    }

    //$last是股票更新时间
    if (file_exists($cache_file)) { //存在cache文件
        $ctime=filemtime($cache_file); //结果本地缓存半小时
        if ($ctime+600 > $last) {
            echo(file_get_contents($cache_file).",$next");
            return;
        }
    }
         $qt=file_get_contents("http://qt.gtimg.cn/q=$gp_name");
         $a=explode('~', $qt);
    if (count($a) <5) {
         $qt=file_get_contents("http://qt.gtimg.cn/q=$gp_name");
         $a=explode('~', $qt);
    }
    if (count($a) <5) {
        $msg='88888';
    } else {
        $msg=$a[3];
    }
    echo "$msg,$next";
        file_put_contents($cache_file, $msg);
        chmod($cache_file,0666);
        touch($cache_file, $last, $last);
}
