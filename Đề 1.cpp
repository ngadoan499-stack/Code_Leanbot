#include <Leanbot.h>
#include <Arduino_APDS9960.h>

int ObjRed, ObjGreen, ObjBlue;
int distance;
int target;
String mau;
int line;
int speed;
int test;

// ==================================================
// ĐỌC RGB — GIỮ CÁCH ĐO CỦA BẠN
// ==================================================
void _coreRead(int &r, int &g, int &b, CRGB ledColor)
{
  LbRGB.fillColor(ledColor);
  LbRGB.show();

  while (!APDS.colorAvailable())
    ;

  APDS.readColor(r, g, b);
}

void Obj_readRGB()
{
  int rRed, gGreen, bBlue;
  int rr, gg, bb;

  int originalBrightness = LbRGB.getBrightness();
  LbRGB.setBrightness(255);

  _coreRead(rRed, gg, bb, CRGB::Red);
  _coreRead(rr, gGreen, bb, CRGB::Lime);
  _coreRead(rr, gg, bBlue, CRGB::Blue);
  _coreRead(rr, gg, bb, CRGB::Black);

  LbRGB.setBrightness(originalBrightness);

  ObjRed = rRed - rr;
  ObjGreen = gGreen - gg;
  ObjBlue = bBlue - bb;
}

// ==================================================
// NHẬN DIỆN MÀU
// ==================================================
boolean BasicObj_isRed()
{
  if (ObjRed <= ObjGreen)
    return false;
  if (ObjRed <= ObjBlue)
    return false;
  if (ObjRed <= 3)
    return false;

  return true;
}

boolean BasicObj_isGreen()
{
  if (ObjGreen <= ObjRed)
    return false;
  if (ObjGreen <= ObjBlue)
    return false;
  if (ObjGreen <= 3)
    return false;

  return true;
}

boolean BasicObj_isPurple()
{
  if (ObjBlue <= ObjRed)
    return false;
  if (ObjBlue <= ObjGreen)
    return false;
  if (ObjBlue <= 3)
    return false;

  if (ObjRed <= 0)
    return false;
  if (ObjGreen <= 0)
    return false;

  if ((long)ObjGreen >= 2L * ObjRed)
    return false;

  return true;
}

boolean BasicObj_isBlue()
{
  if (ObjBlue <= ObjRed)
    return false;
  if (ObjBlue <= ObjGreen)
    return false;
  if (ObjBlue <= 3)
    return false;

  if (ObjRed <= 0)
    return false;
  if (ObjGreen <= 0)
    return false;

  if ((long)ObjGreen < 2L * ObjRed)
    return false;

  return true;
}

// ==================================================
// ĐỌC MÀU VÀ HIỂN THỊ LÊN LED
// ==================================================
String Obj_detectColor()
{
  Obj_readRGB();

  if (BasicObj_isGreen())
  {
    LbRGB.fillColor(CRGB::Green);
    LbRGB.show();
    return "Green";
  }
  else if (BasicObj_isPurple())
  {
    LbRGB.fillColor(CRGB::Purple);
    LbRGB.show();
    return "Purple";
  }
  else if (BasicObj_isBlue())
  {
    LbRGB.fillColor(CRGB::Blue);
    LbRGB.show();
    return "Blue";
  }
  else
  {
    LbRGB.fillColor(CRGB::Red);
    LbRGB.show();
    return "Red";
  }
  LbRGB.fillColor(CRGB(50, 50, 50));
  LbRGB.show();
  return "Unknown";
}

// ==================================================
// ĐIỀU HƯỚNG CƠ BẢN
// ==================================================
void re_phai(int goc_phai)
{
  LbMotion.runLRrpm(25, -25);
  LbMotion.waitRotationDeg(goc_phai);
  LbMotion.stopAndWait();
}

void di_thang(int kc_thang)
{
  LbMotion.runLRrpm(30, 30);
  LbMotion.waitDistanceMm(kc_thang * 10);
  LbMotion.stopAndWait();
}

void di_lui(int kc_thang)
{
  LbMotion.runLRrpm(-25, -25);
  LbMotion.waitDistanceMm(kc_thang * 10);
  LbMotion.stopAndWait();
}

void re_trai(int goc_trai)
{
  LbMotion.runLRrpm(-25, 25);
  LbMotion.waitRotationDeg(goc_trai);
  LbMotion.stopAndWait();
}

// ==================================================
// HÀM DÒ LINE
// ==================================================
void runFollowLine()
{
  LbIRLine.read();
  line = LbIRLine.value();
  LbIRLine.displayOnRGB(0x00ff00);
  switch (line)
  {
  case 4:
  case 14:
  case 12:
  case 8:
    LbMotion.runLR(speed / 8, speed);
    break;

  case 2:
  case 7:
  case 3:
  case 1:
    LbMotion.runLR(speed, speed / 8);
    break;

  case 9:
  case 13:
  case 11:
  case 10:
  case 5:
    test = 123;
    break;

  default:
    LbMotion.runLR(speed, speed);
  }
}

void followLineCarefully()
{
  for (int count = 0; count < 10; count++)
  {
    runFollowLine();
    if (LbIRLine.isBlackDetected())
    {
      break;
    }
    LbMotion.waitDistanceMm(1);
  }
}

void followLine()
{
  target = LbMotion.getDistanceMm() + distance;
  followLineCarefully();
  while (LbIRLine.isBlackDetected())
  {
    followLineCarefully();
    if ((LbMotion.getDistanceMm() > target))
    {
      if ((LbIRLine.value() == 6))
      {
        break;
      }
    }
    LbMotion.waitDistanceMm(1);
  }
}

// ==================================================
// TRẠM H1
// ==================================================
void den_H1()
{
    di_thang(10);
    re_phai(85);

    speed = 1500;
    distance = 400;
    followLine();
    LbMotion.stopAndWait();

    re_phai(135);
    di_thang(8);

    mau = Obj_detectColor();
    LbDelay(500);
}

void xu_ly_H1()
{
    if (mau == "Blue")
    {
        di_lui(8);
        re_phai(45);
        speed = 1500;
        distance = 300;
        followLine();
        LbMotion.stopAndWait();
        re_trai(85);
        speed = 1500;
        distance = 300;
        followLine();
        LbMotion.stopAndWait();
        LbGripper.moveTo(90);
        re_trai(85);
        speed = 1500;
        distance = 300;
        followLine();
        LbMotion.stopAndWait();
        re_trai(85);
        speed = 1500;
        distance = 300;
        followLine();
        LbMotion.stopAndWait();
        re_trai(135);
        di_thang(15);
        LbGripper.moveTo(0);
        di_lui(15);
        re_trai(45);
    }
    else if (mau == "Green")
    {
        di_lui(8);
        re_phai(45);
        speed = 1500;
        distance = 300;
        followLine();
        LbMotion.stopAndWait();
        re_trai(85);
        speed = 1500;
        distance = 300;
        followLine();
        LbMotion.stopAndWait();
        re_phai(90);
        di_thang(10);
        re_trai(85);
        LbGripper.moveTo(90);
        re_trai(85);
        speed = 1500;
        distance = 400;
        followLine();
        LbMotion.stopAndWait();
        re_trai(85);
        speed = 1500;
        distance = 300;
        followLine();
        LbMotion.stopAndWait();
        re_trai(135);
        di_thang(15);
        LbGripper.moveTo(0);
        di_lui(15);
        re_trai(45);
    }
    else if (mau == "Purple")
    {
        di_lui(8);
        re_phai(45);
        speed = 1500;
        distance = 620;
        followLine();
        LbMotion.stopAndWait();
        LbGripper.moveTo(90);
        re_phai(175);
        speed = 1500;
        distance = 620;
        followLine();
        LbMotion.stopAndWait();
        re_phai(135);
        di_thang(15);
        LbGripper.moveTo(0);
        di_lui(15);
        re_trai(45);
    }
    else
    {
        LbGripper.moveTo(90);
        di_lui(8);
        re_phai(45);
        speed = 1500;
        distance = 300;
        followLine();
        LbMotion.stopAndWait();
        re_trai(42);
        di_thang(10);
        LbGripper.moveTo(0);
        di_lui(10);
        re_trai(130);
        speed = 1500;
        distance = 300;
        followLine();
        LbMotion.stopAndWait();
        re_phai(85);
    }
}

// ==================================================
// TRẠM F3
// ==================================================
void den_F3()
{
  speed = 1500;
  distance = 300;
  followLine();
  LbMotion.stopAndWait();

  re_trai(85);
  speed = 1500;
  distance = 280;
  followLine();
  LbMotion.stopAndWait();
  re_trai(75);
  di_thang(3);

  mau = Obj_detectColor();
  LbDelay(500);
}

void xu_ly_F3()
{
  if (mau == "Purple")
  {
    di_lui(2);
    re_trai(70);
    speed = 1500;
    distance = 260;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(90);
    speed = 1500;
    distance = 630;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(90);
    re_trai(175);
    speed = 1500;
    distance = 630;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    di_thang(13);
    LbGripper.moveTo(0);
    di_lui(13);
    re_phai(40);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
  }
  else if (mau == "Green")
  {
    di_lui(2);
    re_trai(80);
    speed = 1500;
    distance = 280;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(90);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(90);
    speed = 1500;
    distance = 220;
    followLine();
    LbMotion.stopAndWait();
    re_phai(25);
    di_thang(10);
    LbGripper.moveTo(90);
    di_lui(10);
    re_phai(150);
    speed = 1500;
    distance = 220;
    followLine();
    LbMotion.stopAndWait();
    re_phai(90);
    speed = 1500;
    distance = 640;
    followLine();
    LbMotion.stopAndWait();
    re_phai(90);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(90);
    di_thang(12);
    LbGripper.moveTo(0);
    di_lui(13);
    re_phai(70);
    speed = 1500;
    distance = 280;
    followLine();
    LbMotion.stopAndWait();
  }
  else if (mau == "Blue")
  {
    di_lui(2);
    re_phai(75);
    speed = 1500;
    distance = 280;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 260;
    followLine();
    LbMotion.stopAndWait();
    re_trai(15);
    di_thang(10);
    LbGripper.moveTo(90);
    di_lui(10);
    re_trai(160);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 260;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    di_thang(13);
    LbGripper.moveTo(0);
    di_lui(13);
    re_phai(40);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
  }
  else
  {
    LbGripper.moveTo(90);
    di_lui(3);
    re_phai(75);
    speed = 1500;
    distance = 280;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_trai(43);
    di_thang(12);
    LbGripper.moveTo(0);
    di_lui(12);
    re_trai(130);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
  }
}

// ==================================================
// TRẠM C1
// ==================================================
void den_C1()
{
  re_trai(130);
  di_thang(8);

  mau = Obj_detectColor();
  LbDelay(500);
}

void xu_ly_C1()
{
  if (mau == "Purple")
  {
    di_lui(8);
    re_phai(75);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    di_thang(10);
    LbGripper.moveTo(90);
    di_lui(10);
    re_trai(170);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    di_thang(13);
    LbGripper.moveTo(0);
    di_lui(13);
    re_phai(40);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
  }
  else if (mau == "Green")
  {
    di_lui(8);
    re_phai(42);
    speed = 1500;
    distance = 640;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(25);
    di_thang(10);
    LbGripper.moveTo(90);
    di_lui(10);
    re_phai(150);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    di_thang(13);
    LbGripper.moveTo(0);
    di_lui(13);
    re_phai(40);
    speed = 1500;
    distance = 630;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
  }
  else if (mau == "Blue")
  {
    di_lui(8);
    re_phai(42);
    speed = 1500;
    distance = 640;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 260;
    followLine();
    LbMotion.stopAndWait();
    re_trai(15);
    di_thang(10);
    LbGripper.moveTo(90);
    di_lui(10);
    re_trai(160);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    di_thang(13);
    LbGripper.moveTo(0);
    di_lui(13);
    re_phai(40);
    speed = 1500;
    distance = 630;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
  }
  else
  {
    LbGripper.moveTo(90);
    di_lui(8);
    re_phai(42);
    speed = 1500;
    distance = 640;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 320;
    followLine();
    LbMotion.stopAndWait();
    re_phai(43);
    di_thang(12);
    LbGripper.moveTo(0);
    di_lui(12);
    re_phai(130);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
  }
}

// ==================================================
// TRẠM E2
// ==================================================
void den_E2()
{
  re_phai(132);
  di_thang(8);

  mau = Obj_detectColor();
  LbDelay(500);
}

void xu_ly_E2()
{
  if (mau == "Purple")
  {
    di_lui(8);
    re_phai(42);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(90);
    re_phai(170);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(130);
    di_thang(13);
    LbGripper.moveTo(0);
    di_lui(13);
    re_phai(45);
    speed = 1500;
    distance = 310;
    followLine();
    LbMotion.stopAndWait();
    re_phai(45);
    di_thang(12);
    LbMotion.stopAndWait();
  }
  else if (mau == "Green")
  {
    di_lui(8);
    re_phai(42);
    speed = 1500;
    distance = 260;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 220;
    followLine();
    LbMotion.stopAndWait();
    re_phai(25);
    di_thang(10);
    LbGripper.moveTo(90);
    di_lui(10);
    re_phai(150);
    speed = 1500;
    distance = 220;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(42);
    di_thang(13);
    LbGripper.moveTo(0);
    di_lui(13);
    re_trai(130);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    di_thang(12);
    LbMotion.stopAndWait();
  }
  else if (mau == "Blue")
  {
    di_lui(8);
    re_phai(42);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    speed = 1500;
    distance = 260;
    followLine();
    LbMotion.stopAndWait();
    re_trai(15);
    di_thang(10);
    LbGripper.moveTo(90);
    di_lui(10);
    re_trai(160);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(42);
    di_thang(13);
    LbGripper.moveTo(0);
    di_lui(13);
    re_trai(130);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(85);
    di_thang(12);
    LbMotion.stopAndWait();
  }
  else
  {
    LbGripper.moveTo(90);
    di_lui(8);
    re_phai(42);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_phai(42);
    di_thang(12);
    LbGripper.moveTo(0);
    LbMotion.stopAndWait();
  }
}

// ==================================================
// TRẠM A2
// ==================================================
void den_A2()
{
  speed = 1500;
  distance = 300;
  followLine();
  LbMotion.stopAndWait();

  re_trai(85);
  di_thang(8);

  mau = Obj_detectColor();
  LbDelay(500);
}

void xu_ly_A2()
{
  if (mau == "Blue")
  {
    di_lui(8);
    re_phai(45);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    di_thang(10);
    LbGripper.moveTo(90);
    di_lui(10);
    re_phai(135);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(0);
  }
  else if (mau == "Green")
  {
    di_lui(8);
    re_phai(45);
    speed = 1500;
    distance = 400;
    followLine();
    LbMotion.stopAndWait();
    re_trai(85);
    LbGripper.moveTo(90);
    di_lui(8);
    LbGripper.moveTo(0);
  }
  else if (mau == "Purple")
  {
    di_lui(8);
    re_phai(90);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(90);
  }
  else
  {
    LbGripper.moveTo(90);
    di_lui(8);
    re_phai(90);
    speed = 1500;
    distance = 620;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(0);
  }
}

// ==================================================
// TRẠM B4
// ==================================================
void den_B4()
{
  re_phai(90);
  di_thang(10);
  speed = 1500;
  distance = 300;
  followLine();
  LbMotion.stopAndWait();

  re_trai(85);
  di_thang(8);

  mau = Obj_detectColor();
  LbDelay(500);
}

void xu_ly_B4()
{
  if (mau == "Blue")
  {
    di_lui(8);
    re_phai(85);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(90);
  }
  else if (mau == "Green")
  {
    di_lui(8);
    re_phai(45);
    speed = 1500;
    distance = 350;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(90);
  }
  else if (mau == "Purple")
  {
    di_lui(8);
    re_trai(45);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(90);
  }
  else
  {
    LbGripper.moveTo(90);
    di_lui(8);
    re_phai(90);
    speed = 1500;
    distance = 500;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(0);
  }
}

// ==================================================
// TRẠM D3
// ==================================================
void den_D3()
{
  re_trai(90);
  speed = 1500;
  distance = 300;
  followLine();
  LbMotion.stopAndWait();

  re_phai(85);
  di_thang(8);

  mau = Obj_detectColor();
  LbDelay(500);
}

void xu_ly_D3()
{
  if (mau == "Blue")
  {
    di_lui(8);
    re_trai(45);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(90);
  }
  else if (mau == "Green")
  {
    di_lui(8);
    re_phai(85);
    speed = 1500;
    distance = 400;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(90);
  }
  else if (mau == "Purple")
  {
    di_lui(8);
    re_phai(90);
    speed = 1500;
    distance = 300;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(90);
  }
  else
  {
    LbGripper.moveTo(90);
    di_lui(8);
    re_trai(45);
    speed = 1500;
    distance = 500;
    followLine();
    LbMotion.stopAndWait();
    LbGripper.moveTo(0);
  }
}

// ==================================================
// SETUP & LOOP
// ==================================================
void setup()
{
  Leanbot.begin(); // initialize Leanbot
  LbIRLine.doManualCalibration(TB1A);
  APDS.begin();
  LbDelay(100);
}

void loop()
{
  LbMission.begin(TB1A + TB1B);
    den_H1();
    xu_ly_H1();
    
    den_F3();
    xu_ly_F3();
    
    den_C1();
    xu_ly_C1();
    
    den_E2();
    xu_ly_E2();

    den_A2();
    xu_ly_A2();

    den_B4();
    xu_ly_B4();

    den_D3();
    xu_ly_D3();
  LbMission.end();
}