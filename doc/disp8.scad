fx=0.4; //缝隙 pla要留缝隙， 光敏树脂可以缝隙=0,
x=59.2;
y=35.2;
z=15.5;
bh=2.2;//壁的厚度
reset=[bh+44,5.3,0]; //reset按键的位置
dl=[52,3.5,0]; //download按键的位置
bhz=1; //垂直的壁厚
$fn=32; //圆型结构的精细度 1/32


top();  //顶盖 
translate([0,-42,0]) //y向偏移42mm 
 bottom(); //底盖




//下面是子程序 模块

module top() { //顶盖部分 
 difference(){
  translate([-bh,-bh,-bhz]) cube([x+bh+bh,y+bh+bh,z+bhz]);
  translate([-fx,-fx,0]) cube([x+fx+fx,y+fx+fx,z]);
  translate([2,8.5,-bhz]) cube([53.5,19,z]);
  translate([x,19.5,2.5]) cube([10,8.5,4]); //usb孔
  translate([x+1+fx,19.5-2,2.5-1]) cube([10,8.5+4,4+2]); //usb框
 }
 cube([0.5,bh+y,5.5]);//lcd边条
 cube([3.5,4.5,5.5]);
 cube([x+bh,4.5,2]);
 translate([9.5,0,0]) cube([x-9.5+0.1,2.5,5.5]);
 translate([48,0,0]) cube([7.5,4.5,5.5]);
 translate([0,29.3,0])
 cube([x+bh,y-29.3,2-1]);

 translate([x-1.5,29.3,0]) cube([1.5+bh-1,bh+y-29.3,5.5]);
 translate([37,34,0]) cube([2.3+19+bh,bh+y-34,5.5]);
 translate([0,34,0]) cube([x,bh+y-34,3]);
 translate([0,33,0]) cube([3,bh+y-33,5.5+1.2+0.5]);
  translate([x+fx,19.5+8.5/2-1,0]) cube([1,2,z]); //usb支撑
}

module switch(){  //按钮开关结构
 fx1=0.8;
 bh=2;
 if(fx>0) { //fx>0 是pla打印，按键的缝隙要宽一点
  fx1=1.6;
 }
 r1=4.8+fx1+fx1;
 difference(){
  union(){
   cylinder(r=r1/2,h=bh);
   translate([-15,-r1/2,0]) cube([15,r1,bh]);
  }//union
  cylinder(r=r1/2-fx1,h=bh);
  translate([-15,-r1/2+fx1,0]) cube([15,r1-fx1-fx1,bh]);
 }//diff
}

module bottom(){ //底盖部分
 z1=5+1.5; //厚度
 bh=2;      //壁厚
 difference(){
  cube([x,y,z1]);
  translate([1,bh,1.5]) cube([x-1-1.6,y-bh-bh,z1]);
  translate([2,30,0]) cube([18,3.1,z1]); //com1
  translate([1,0,z1-3.7]) cube([23,bh,z1]);//电池
  translate([1+18,y-bh,1.5+2]) cube([33,bh,z1]);//天线
  translate(dl) {
   cylinder(r=0.7,h=bh);
   translate([-7/2,-4/2,z1-3]) cube([7,4,z1]); //3x6 开关
  }
  translate(reset) switch();
 }
 translate(reset) cylinder(r=3/2,h=z1-4.2);//按键中心柱
}