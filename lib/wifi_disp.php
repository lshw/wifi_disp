<?php
/*程序考虑了api数据缓存，尽快断开客户端链接，如果不需要这些，只需要一行代码就行了
echo "$_GET[temp],720";
或者
echo api("https://lab.ahusmart.com//nCoV/api/overall","confirmedCount").",3600"; //10小时后再上线
*/

ignore_user_abort(true);//客户端断开链接后继续执行php程序。
ob_start();
switch($_GET['type']) {
	case 'wh':
		echo api("https://lab.ahusmart.com//nCoV/api/overall","confirmedCount").",3600"; //10小时后再上线
		break;
	default:
		echo "$_GET[temp],720"; //显示温度，2小时(720*10s)后再上线。
		break;
}
$size=ob_get_length();
header('Connection: close');//不需要持续链接
header("Content-Length: $size");  //明确告诉浏览器数据长度,浏览器接收到此长度数据后就不再接收数据
ob_end_flush();
//save_data();//断开链接后，再保存数据，防止长时间挂住客户端，造成客户端费电。

function api($url,$field) {
	$urlmd5=md5($url.$field);
	$t=filemtime("/tmp/${urlmd5}.txt"); //结果本地缓存半小时
	if($t<time()+1800) {
		$str=file_get_contents("/tmp/${urlmd5}.txt");
	}
	if($str=='') {
		$str=file_get_contents($url);
		$a=json_decode($str,true);

		$str=$a[results][0][$field];
		file_put_contents("/tmp/${urlmd5}.txt",$str);
	}
	return $str;
}
