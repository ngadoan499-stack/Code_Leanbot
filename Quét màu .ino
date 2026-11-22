#include <Leanbot.h>
#include <Arduino_APDS9960.h>

int ObjRed, ObjGreen, ObjBlue;
String mau = "";

// Ngưỡng phân biệt nhanh độ sáng (chỉnh theo khoảng cách đặt khối thực tế)
const int NGUONG_DEN   = 4;   // Dưới mức này coi là Đen / không có vật
const int NGUONG_TRANG = 20;  // Độ sáng tối thiểu để nhận diện Trắng
const int NGUONG_LIME  = 35;  // Xanh lá có G >= 35 -> Lime, G < 35 -> Green

// ==================================================
// ĐỌC RGB (Giữ nguyên cách đo vi sai + Timeout 200ms)
// ==================================================
void _coreRead(int &r, int &g, int &b, CRGB ledColor)
{
  LbRGB.fillColor(ledColor);
  LbRGB.show();

  unsigned long startTime = millis();
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

  // Chặn giá trị âm do nhiễu ánh sáng môi trường
  ObjRed   = max(0, rRed - rr);
  ObjGreen = max(0, gGreen - gg);
  ObjBlue  = max(0, bBlue - bb);
}

// ==================================================
// NHẬN DIỆN ĐẦY ĐỦ 12 MÀU
// ==================================================
int _maxRGB() { return max(ObjRed, max(ObjGreen, ObjBlue)); }
int _minRGB() { return min(ObjRed, min(ObjGreen, ObjBlue)); }

// 1. Đen / Tắt đèn / Không có vật
boolean BasicObj_isBlack()
{
  return (_maxRGB() <= NGUONG_DEN);
}

// 2. Trắng (3 kênh đều sáng và cân bằng nhau >= 65%)
boolean BasicObj_isWhite()
{
  int maxC = _maxRGB();
  int minC = _minRGB();
  if (maxC < NGUONG_TRANG) return false;
  return ((long)minC * 100L >= (long)maxC * 65L);
}

// 3. Vàng (Đỏ + Xanh lá đều cao và gần nhau, Xanh dương thấp)
boolean BasicObj_isYellow()
{
  int maxC = _maxRGB();
  if (ObjRed < 10 || ObjGreen < 10) return false;
  if ((long)ObjBlue * 100L >= (long)maxC * 45L) return false;
  return ((long)min(ObjRed, ObjGreen) * 100L >= (long)max(ObjRed, ObjGreen) * 70L);
}

// 4. Xanh ngọc - Cyan (Xanh lá + Xanh dương đều cao và gần nhau, Đỏ thấp)
boolean BasicObj_isCyan()
{
  int maxC = _maxRGB();
  if (ObjGreen < 10 || ObjBlue < 10) return false;
  if ((long)ObjRed * 100L >= (long)maxC * 45L) return false;
  return ((long)min(ObjGreen, ObjBlue) * 100L >= (long)max(ObjGreen, ObjBlue) * 70L);
}

// 5. Tím hồng sáng - Magenta (Đỏ + Xanh dương đều cao và gần nhau, Xanh lá thấp)
boolean BasicObj_isMagenta()
{
  int maxC = _maxRGB();
  if (ObjRed < 10 || ObjBlue < 10) return false;
  if ((long)ObjGreen * 100L >= (long)maxC * 45L) return false;
  return ((long)min(ObjRed, ObjBlue) * 100L >= (long)max(ObjRed, ObjBlue) * 75L);
}

// 6. Cam - Orange (Đỏ trội nhất, Xanh lá ở mức trung bình 30%-70% của Đỏ, Xanh dương thấp)
boolean BasicObj_isOrange()
{
  if (ObjRed <= ObjGreen || ObjRed <= ObjBlue || ObjRed <= NGUONG_DEN) return false;
  if ((long)ObjBlue * 100L >= (long)ObjRed * 35L) return false;
  long gRatio = (long)ObjGreen * 100L;
  return (gRatio >= (long)ObjRed * 30L && gRatio < (long)ObjRed * 70L);
}

// 7. Hồng đậm - DeepPink (Đỏ trội nhất, Xanh dương 30%-75% của Đỏ, Xanh lá thấp)
boolean BasicObj_isDeepPink()
{
  if (ObjRed <= ObjBlue || ObjRed <= ObjGreen || ObjRed <= NGUONG_DEN) return false;
  if ((long)ObjGreen * 100L >= (long)ObjRed * 40L) return false;
  long bRatio = (long)ObjBlue * 100L;
  return (bRatio >= (long)ObjRed * 30L && bRatio < (long)ObjRed * 75L);
}

// 8. Đỏ - Red (Đỏ trội hoàn toàn so với G và B)
boolean BasicObj_isRed()
{
  if (ObjRed <= ObjGreen || ObjRed <= ObjBlue || ObjRed <= NGUONG_DEN) return false;
  return true;
}

// 9. Xanh lá sáng - Lime (Xanh lá trội nhất và phản xạ sáng mạnh)
boolean BasicObj_isLime()
{
  if (ObjGreen <= ObjRed || ObjGreen <= ObjBlue) return false;
  return (ObjGreen >= NGUONG_LIME);
}

// 10. Xanh lá đậm - Green (Xanh lá trội nhất, độ sáng vừa/thấp hơn Lime)
boolean BasicObj_isGreen()
{
  if (ObjGreen <= ObjRed || ObjGreen <= ObjBlue || ObjGreen <= NGUONG_DEN) return false;
  return (ObjGreen < NGUONG_LIME);
}

// 11. Tím - Purple (Giữ quy tắc G < 2 * R của bạn)
boolean BasicObj_isPurple()
{
  if (ObjBlue <= ObjRed || ObjBlue <= ObjGreen || ObjBlue <= NGUONG_DEN) return false;
  if (ObjRed <= 0 || ObjGreen <= 0) return false;
  return ((long)ObjGreen < 2L * ObjRed);
}

// 12. Xanh dương - Blue (Giữ quy tắc G >= 2 * R của bạn, hỗ trợ thêm trường hợp R = 0)
boolean BasicObj_isBlue()
{
  if (ObjBlue <= ObjRed || ObjBlue <= ObjGreen || ObjBlue <= NGUONG_DEN) return false;
  if (ObjRed == 0) return true;
  return ((long)ObjGreen >= 2L * ObjRed);
}

// ==================================================
// ĐỌC MÀU VÀ HIỂN THỊ LÊN LED
// ==================================================
String Obj_detectColor()
{
  Obj_readRGB();

  // Thứ tự kiểm tra bắt buộc: Đen/Trắng -> Các màu pha -> Các màu đơn sắc
  if (BasicObj_isBlack())
  {
    LbRGB.fillColor(CRGB::Black);
    LbRGB.show();
    return "Black";
  }
  else if (BasicObj_isWhite())
  {
    LbRGB.fillColor(CRGB::White);
    LbRGB.show();
    return "White";
  }
  else if (BasicObj_isYellow())
  {
    LbRGB.fillColor(CRGB::Yellow);
    LbRGB.show();
    return "Yellow";
  }
  else if (BasicObj_isCyan())
  {
    LbRGB.fillColor(CRGB::Cyan);
    LbRGB.show();
    return "Cyan";
  }
  else if (BasicObj_isMagenta())
  {
    LbRGB.fillColor(CRGB::Magenta);
    LbRGB.show();
    return "Magenta";
  }
  else if (BasicObj_isOrange())
  {
    LbRGB.fillColor(CRGB::Orange);
    LbRGB.show();
    return "Orange";
  }
  else if (BasicObj_isDeepPink())
  {
    LbRGB.fillColor(CRGB::DeepPink);
    LbRGB.show();
    return "DeepPink";
  }
  else if (BasicObj_isLime())
  {
    LbRGB.fillColor(CRGB::Lime);
    LbRGB.show();
    return "Lime";
  }
  else if (BasicObj_isGreen())
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
  else if (BasicObj_isRed())
  {
    LbRGB.fillColor(CRGB::Red);
    LbRGB.show();
    return "Red";
  }
  else
  {
    LbRGB.fillColor(CRGB(50, 50, 50));
    LbRGB.show();
    return "Unknown";
  }
}

// ==================================================
// SETUP & LOOP
// ==================================================
void setup()
{
  Serial.begin(9600);
  Leanbot.begin();

  if (!APDS.begin())
  {
    LbRGB.fillColor(CRGB::Red);
    LbRGB.show();
    Serial.println("Loi khoi tao cam bien mau APDS9960!");
  }

  LbDelay(100);
}

void loop()
{
  mau = Obj_detectColor();

  // In thong so ra Serial Monitor de ban de can chinh nguong tren vat mau that
  Serial.print("R="); Serial.print(ObjRed);
  Serial.print(" | G="); Serial.print(ObjGreen);
  Serial.print(" | B="); Serial.print(ObjBlue);
  Serial.print(" => "); Serial.println(mau);

  LbDelay(500);
}
