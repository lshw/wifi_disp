<?php
ignore_user_abort(true);
ob_start();
switch($_GET['type']) {
	case 'wh':
		echo api("https://lab.ahusmart.com//nCoV/api/overall","confirmedCount").",7200";
		break;
	case 'meiyuan':
	case 'Dollar':
	case 'USD':
	case 'JPY':
		break;
	default:
		echo "$_GET[temp],1800"; //显示温度，半小时后再上线
		break;
}

header('Connection: close');
$size=ob_get_length();
header("Content-Length: $size");  //告诉浏览器数据长度,浏览器接收到此长度数据后就不再接收数据
ob_end_flush();
//save_data();//断开链接后，再保存数据，防止长时间挂住客户端，造成客户端费电

function api($url,$field) {
	$urlmd5=md5($url.$field);
	$t=filemtime("/tmp/${urlmd5}.txt");
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
