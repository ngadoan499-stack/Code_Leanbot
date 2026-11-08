#include <Leanbot.h>
#include <Arduino_APDS9960.h>

int ObjRed, ObjGreen, ObjBlue;
String mau = "";

// ==================================================
// ĐỌC RGB (Có cơ chế chống treo / Timeout an toàn)
// ==================================================
void _coreRead(int &r, int &g, int &b, CRGB ledColor)
{
  LbRGB.fillColor(ledColor);
  LbRGB.show();

  unsigned long startTime = millis();
  // Chờ cảm biến sẵn sàng, tối đa 200ms để tránh kẹt vô tận nếu cảm biến lỗi
  while (!APDS.colorAvailable())
  {
    if (millis() - startTime > 200)
    {
      break; 
    }
  }

  APDS.readColor(r, g, b);
}

void Obj_readRGB()
{
  int rRed = 0, gGreen = 0, bBlue = 0;
  int rr = 0, gg = 0, bb = 0;

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
  if (ObjRed <= ObjGreen) return false;
  if (ObjRed <= ObjBlue) return false;
  if (ObjRed <= 3) return false;
  return true;
}

boolean BasicObj_isGreen()
{
  if (ObjGreen <= ObjRed) return false;
  if (ObjGreen <= ObjBlue) return false;
  if (ObjGreen <= 3) return false;
  return true;
}

boolean BasicObj_isPurple()
{
  if (ObjBlue <= ObjRed) return false;
  if (ObjBlue <= ObjGreen) return false;
  if (ObjBlue <= 3) return false;
  if (ObjRed <= 0) return false;
  if (ObjGreen <= 0) return false;
  if ((long)ObjGreen >= 2L * ObjRed) return false;
  return true;
}

boolean BasicObj_isBlue()
{
  if (ObjBlue <= ObjRed) return false;
  if (ObjBlue <= ObjGreen) return false;
  if (ObjBlue <= 3) return false;
  if (ObjRed <= 0) return false;
  if (ObjGreen <= 0) return false;
  if ((long)ObjGreen < 2L * ObjRed) return false;
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
}

// ==================================================
// SETUP & LOOP (Bắt buộc cho Arduino)
// ==================================================
void setup()
{
  Leanbot.begin(); // Khởi tạo Leanbot
  
  // Khởi tạo cảm biến màu APDS9960 an toàn
  if (!APDS.begin()) {
    // Nếu cảm biến lỗi kết nối, đèn LED RGB có thể nháy báo đỏ ở đây nếu cần
  }
  
  LbDelay(100);
}

void loop()
{
  // Thực thi đọc và nhận diện màu liên tục
  mau = Obj_detectColor();
  LbDelay(500);
}