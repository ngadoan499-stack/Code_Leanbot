#include <Leanbot.h>
#include <Arduino_APDS9960.h>

// ============================================================================
// 0. KHU VỰC NHẬP ĐỀ THI NHANH (CHỈ SỬA ĐÚNG DÒNG NÀY KHI BỐC THĂM ĐỀ)
//    Danh sách mã hợp lệ: O_A1..O_A4, O_B1..O_B4, O_C1..O_C4, O_D1..O_D4,
//                         O_E1..O_E4, O_F1..O_F4, O_G1..O_G4, O_H1..O_H4
// ============================================================================
enum ViTri : uint8_t {
  O_A1, O_A2, O_A3, O_A4,
  O_B1, O_B2, O_B3, O_B4,
  O_C1, O_C2, O_C3, O_C4,
  O_D1, O_D2, O_D3, O_D4,
  O_E1, O_E2, O_E3, O_E4,
  O_F1, O_F2, O_F3, O_F4,
  O_G1, O_G2, O_G3, O_G4,
  O_H1, O_H2, O_H3, O_H4
};

// >>> NHẬP ĐÚNG THỨ TỰ CÁC Ô ĐƯỢC BỐC THĂM TẠI ĐÂY <<<
const ViTri DE_THI[] = { O_H1, O_F3, O_C1, O_E2 };

// Bật = true khi test màu trên máy tính qua Serial Monitor, khi THI ĐẤU để = false
const bool BAT_DEBUG_SERIAL = false;

// ============================================================================
// 1. CẤU HÌNH TỐC ĐỘ, BÙ GÓC & THÔNG SỐ SA BÀN SYNAPSE CITY V3
// ============================================================================
const int V_NHANH          = 1400;  // Chạy thẳng đoạn dài (an toàn chống trượt bước)
const int V_LINE           = 1250;  // Dò line tốc độ cao ổn định trên bạt thi
const int V_LUI_KHOI       = 900;   // Lùi thoát khối êm, không văng đuôi
const int V_XOAY           = 900;   // Xoay góc nhanh
const int V_CHAM           = 520;   // Phanh mềm / đẩy 7cm / khóa vạch

const int O_LUOI_MM        = 305;   // Độ dài 1 cạnh ô lưới (mm)
const int KC_TAM_BANH_MM   = 35;    // Khoảng cách từ mắt IR tới trục bánh (mm)
const int KC_GOC_BO_MM     = 265;   // Quãng đường đi tới 3 góc bo tròn A1, F3, D2 (mm)
const int KC_S3_SANG_S1_MM = 85;    // Khoảng cách từ (2,3)/S3 sang trái tới S1 (mm)
const int KC_CRL_LEN_22_MM = 105;   // Từ vạch x=2 ngang CRL chạy lên giao điểm (2,2) (mm)

const int BU_GOC_NHO       = 3;     // Trừ 3° cho góc <= 60° (45° -> 42°)
const int BU_GOC_LON       = 5;     // Trừ 5° cho góc > 60°  (90°->85°, 135°->130°, 180°->175°)
const int GOC_QUET_LINE    = 38;    // Giới hạn góc quét bắt vạch (chống quay lố)

const int CANG_MO          = 0;     // Mở rộng càng
const int CANG_DAY         = 35;    // Góc chữ U giữ thẳng khối khi đẩy 7cm
const int CANG_KEP         = 90;    // Kẹp chặt khối trung hòa / khối nguy hại

const int KC_VAO_KHOI      = 8;     // Từ giao điểm chéo vào sát mặt khối (cm)
const int KC_VAO_GOC_BO    = 4;     // Từ mép cong vào sát mặt khối A1, F3, D2 (cm)
const int KC_DAY_GIU_DIEM  = 7;     // Quét màu xong đẩy thẳng 7cm giữ trọn điểm
const int KC_DAT_SAU       = 10;    // Độ sâu đặt khối trung hòa sau khối đã đẩy (cm)
const int KC_DAT_SAU_BO    = 6;     // Độ sâu đặt khối trung hòa ở 3 góc bo (cm)

// Cấu hình kho trung hòa (tự cộng BUOC_KHO_CM nếu bốc thăm trùng màu xếp nối đuôi)
// Lưu ý: Nếu mỗi kho chỉ có 1 vị trí cố định hoặc không xếp sâu, đặt BUOC_KHO_CM = 0
const int KC_VAO_S3        = 2;     // Tiến tại (2,3) kẹp Xanh lá (cm)
const int KC_VAO_S1        = 2;     // Tiến tại S1 kẹp Xanh dương (cm)
const int KC_VAO_S4        = 2;     // Tiến tại (3,2) kẹp Tím (cm)
const int BUOC_KHO_CM      = 4;     // Cộng thêm 4cm cho khối thứ 2 cùng màu trong kho

// Cấu hình xếp quạt tối đa 4 khối nguy hại trong CRL (tránh húc đổ nhau)
const int CRL_GOC_THA[4]   = { 135, 122, 148, 135 };
const int CRL_DO_SAU[4]    = { 11,  10,  10,  6   };

// Ngưỡng đọc màu APDS9960 (tinh chỉnh lại trên sân nếu ánh sáng quá gắt)
const int NGUONG_CO_KHOI   = 6;     // Tổng R+G+B tối thiểu để xác nhận có khối trước mặt
const int NGUONG_MAU_CHINH = 4;     // Giá trị tối thiểu của kênh màu trội

// Biến trạng thái trận đấu
uint8_t curX = 2;
uint8_t curY = 2;
int16_t curGoc = 0;
uint8_t soKhoiCRL = 0;
uint8_t daLayS3 = 0, daLayS1 = 0, daLayS4 = 0;

int ObjRed, ObjGreen, ObjBlue;

enum MauKhoi : uint8_t {
  MAU_XANH_LA = 0,   // -> Kho S3 (dưới 2,3)
  MAU_XANH_DUONG,    // -> Kho S1 (trái S3)
  MAU_TIM,           // -> Kho S4 (phải 3,2)
  MAU_NGUY_HAI,      // Đỏ / Vàng / Bí ẩn -> Gom về CRL
  MAU_KHONG_CO       // Ô trống / không thấy khối -> Rút an toàn
};

// Bảng tra chuẩn xác 32 vị trí (loai: 0=ô thường, 1=góc bo rẽ phải, 2=góc bo rẽ trái)
struct ThongTinO {
  uint8_t x;
  uint8_t y;
  int16_t goc;
  uint8_t loai;
};

const ThongTinO BANG_32_O[32] = {
  // KHU A (Hàng 0, Cột 0)
  {0, 1,   0, 1}, {0, 1,  45, 0}, {1, 0, 225, 0}, {1, 1, 315, 0},
  // KHU B (Hàng 1, Cột 0)
  {0, 1, 135, 0}, {0, 2,  45, 0}, {1, 1, 225, 0}, {1, 2, 315, 0},
  // KHU C (Hàng 0, Cột 1)
  {1, 0, 135, 0}, {1, 1,  45, 0}, {2, 0, 225, 0}, {2, 1, 315, 0},
  // KHU D (Hàng 2, Cột 0)
  {0, 2, 135, 0}, {0, 2, 180, 2}, {1, 2, 225, 0}, {1, 3, 315, 0},
  // KHU E (Hàng 1, Cột 1)
  {1, 1, 135, 0}, {1, 2,  45, 0}, {2, 1, 225, 0}, {2, 2, 315, 0},
  // KHU F (Hàng 0, Cột 2)
  {2, 0, 135, 0}, {2, 1,  45, 0}, {3, 1,   0, 2}, {3, 1, 315, 0},
  // KHU G (Hàng 2, Cột 1)
  {1, 2, 135, 0}, {1, 3,  45, 0}, {2, 2, 225, 0}, {2, 3, 315, 0},
  // KHU H (Hàng 1, Cột 2)
  {2, 1, 135, 0}, {2, 2,  45, 0}, {3, 1, 225, 0}, {3, 2, 315, 0}
};

// ============================================================================
// 2. ĐỌC MÀU VI SAI 3 KÊNH LED (XẢ FRAME CŨ + LỌC TRUNG BÌNH 2 LẦN)
// ============================================================================
void _coreReadClean(int &r, int &g, int &b, CRGB ledColor) {
  LbRGB.fillColor(ledColor);
  LbRGB.show();
  // Bỏ 1 mẫu đọng từ chu kỳ tích phân của màu LED trước đó
  while (!APDS.colorAvailable());
  APDS.readColor(r, g, b);
  // Lấy mẫu sạch của màu LED hiện tại
  while (!APDS.colorAvailable());
  APDS.readColor(r, g, b);
}

void doc_rgb_mot_lan(int &outR, int &outG, int &outB) {
  int rRed, gGreen, bBlue, rr, gg, bb;
  _coreReadClean(rRed,   gg,     bb,    CRGB::Red);
  _coreReadClean(rr,     gGreen, bb,    CRGB::Lime);
  _coreReadClean(rr,     gg,     bBlue, CRGB::Blue);
  _coreReadClean(rr,     gg,     bb,    CRGB::Black);

  outR = max(0, rRed   - rr);
  outG = max(0, gGreen - gg);
  outB = max(0, bBlue  - bb);
}

void doc_rgb() {
  int doSangGoc = LbRGB.getBrightness();
  LbRGB.setBrightness(255);

  int r1, g1, b1, r2, g2, b2;
  doc_rgb_mot_lan(r1, g1, b1);
  doc_rgb_mot_lan(r2, g2, b2);

  LbRGB.setBrightness(doSangGoc);
  ObjRed   = (r1 + r2) / 2;
  ObjGreen = (g1 + g2) / 2;
  ObjBlue  = (b1 + b2) / 2;

  if (BAT_DEBUG_SERIAL) {
    Serial.print("RGB: ");
    Serial.print(ObjRed);   Serial.print(", ");
    Serial.print(ObjGreen); Serial.print(", ");
    Serial.println(ObjBlue);
  }
}

MauKhoi quet_mau() {
  doc_rgb();
  int tong = ObjRed + ObjGreen + ObjBlue;

  // 1. Không thấy khối (ô trống)
  if (tong < NGUONG_CO_KHOI) {
    LbRGB.fillColor(CRGB::Yellow); LbRGB.show();
    return MAU_KHONG_CO;
  }

  // 2. Xanh lá
  if (ObjGreen > ObjRed && ObjGreen > ObjBlue && ObjGreen >= NGUONG_MAU_CHINH) {
    LbRGB.fillColor(CRGB::Green); LbRGB.show();
    return MAU_XANH_LA;
  }

  // 3. Xanh dương hoặc Tím (đã bỏ điều kiện gắt ObjRed > 0 && ObjGreen > 0)
  if (ObjBlue >= ObjRed && ObjBlue >= ObjGreen && ObjBlue >= NGUONG_MAU_CHINH) {
    if (ObjRed > 0 && (long)ObjGreen < 2L * ObjRed) {
      LbRGB.fillColor(CRGB::Purple); LbRGB.show();
      return MAU_TIM;
    } else {
      LbRGB.fillColor(CRGB::Blue); LbRGB.show();
      return MAU_XANH_DUONG;
    }
  }

  // 4. Đỏ / Vàng / Bí ẩn -> Nguy hại
  LbRGB.fillColor(CRGB::Red); LbRGB.show();
  return MAU_NGUY_HAI;
}

// ============================================================================
// 3. CHUYỂN ĐỘNG MƯỢT (SOFT-START + PHANH MỀM + BÙ GÓC + BẮT LINE AN TOÀN)
// ============================================================================
int tinh_goc_bu(int gocChuan) {
  if (gocChuan <= 0)  return 0;
  if (gocChuan <= 15) return max(1, gocChuan - 2);
  if (gocChuan <= 60) return gocChuan - BU_GOC_NHO;
  return gocChuan - BU_GOC_LON;
}

void di_thang(int cm, int tocDo = V_NHANH) {
  int mm = cm * 10;
  if (mm <= 0) return;

  if (mm > 65 && tocDo > V_CHAM) {
    // Đề-pa mềm 20mm -> Chạy tốc độ cao -> Phanh mềm 25mm cuối
    LbMotion.runLR(V_CHAM, V_CHAM);
    LbMotion.waitDistanceMm(20);
    LbMotion.runLR(tocDo, tocDo);
    LbMotion.waitDistanceMm(mm - 45);
    LbMotion.runLR(V_CHAM, V_CHAM);
    LbMotion.waitDistanceMm(25);
  } else if (mm > 40 && tocDo > V_CHAM) {
    LbMotion.runLR(tocDo, tocDo);
    LbMotion.waitDistanceMm(mm - 25);
    LbMotion.runLR(V_CHAM, V_CHAM);
    LbMotion.waitDistanceMm(25);
  } else {
    LbMotion.runLR(V_CHAM, V_CHAM);
    LbMotion.waitDistanceMm(mm);
  }
  LbMotion.stopAndWait();
}

void di_lui(int cm, int tocDo = V_LUI_KHOI) {
  int mm = cm * 10;
  if (mm <= 0) return;

  if (mm > 45 && tocDo > V_CHAM) {
    LbMotion.runLR(-tocDo, -tocDo);
    LbMotion.waitDistanceMm(mm - 25);
    LbMotion.runLR(-V_CHAM, -V_CHAM);
    LbMotion.waitDistanceMm(25);
  } else {
    LbMotion.runLR(-V_CHAM, -V_CHAM);
    LbMotion.waitDistanceMm(mm);
  }
  LbMotion.stopAndWait();
}

void re_trai(int gocChuan, int tocDo = V_XOAY) {
  int gocThuc = tinh_goc_bu(gocChuan);
  if (gocThuc <= 0) return;
  if (gocThuc > 22 && tocDo > V_CHAM) {
    LbMotion.runLR(-tocDo, tocDo);
    LbMotion.waitRotationDeg(gocThuc - 15);
    LbMotion.runLR(-V_CHAM, V_CHAM);
    LbMotion.waitRotationDeg(15);
  } else {
    LbMotion.runLR(-V_CHAM, V_CHAM);
    LbMotion.waitRotationDeg(gocThuc);
  }
  LbMotion.stopAndWait();
}

void re_phai(int gocChuan, int tocDo = V_XOAY) {
  int gocThuc = tinh_goc_bu(gocChuan);
  if (gocThuc <= 0) return;
  if (gocThuc > 22 && tocDo > V_CHAM) {
    LbMotion.runLR(tocDo, -tocDo);
    LbMotion.waitRotationDeg(gocThuc - 15);
    LbMotion.runLR(V_CHAM, -V_CHAM);
    LbMotion.waitRotationDeg(15);
  } else {
    LbMotion.runLR(V_CHAM, -V_CHAM);
    LbMotion.waitRotationDeg(gocThuc);
  }
  LbMotion.stopAndWait();
}

void mo_cang() {
  LbGripper.moveTo(CANG_MO);
  LbDelay(100);
}

void kep_cang() {
  LbGripper.moveTo(CANG_KEP);
  LbDelay(130);
}

// Không gọi displayOnRGB ở đây để tránh khóa ngắt làm giật động cơ bước
void bam_line_mot_buoc(int tocDo) {
  LbIRLine.read();
  byte vach = LbIRLine.value();
  switch (vach) {
    case 0b0110:
    case 0b1111: LbMotion.runLR(tocDo, tocDo); break;
    case 0b0100: LbMotion.runLR((tocDo * 78L) / 100, tocDo); break;
    case 0b0010: LbMotion.runLR(tocDo, (tocDo * 78L) / 100); break;
    case 0b1100:
    case 0b1110: LbMotion.runLR((tocDo * 38L) / 100, tocDo); break;
    case 0b0011:
    case 0b0111: LbMotion.runLR(tocDo, (tocDo * 38L) / 100); break;
    case 0b1000: LbMotion.runLR(tocDo / 8, tocDo); break;
    case 0b0001: LbMotion.runLR(tocDo, tocDo / 8); break;
    default:     LbMotion.runLR((tocDo * 85L) / 100, (tocDo * 85L) / 100); break;
  }
}

long lay_quang_duong(long mocDau) {
  long d = LbMotion.getDistanceMm() - mocDau;
  return (d < 0) ? -d : d;
}

void do_line_mm(int mm, int tocDo = V_LINE) {
  if (mm <= 0) return;
  long mocDau = LbMotion.getDistanceMm();
  while (true) {
    long daDi = lay_quang_duong(mocDau);
    if (daDi >= mm) break;

    int vChay = tocDo;
    if (tocDo > V_CHAM && (daDi < 25 || (mm - daDi) < 40)) {
      vChay = V_CHAM;
    }
    bam_line_mot_buoc(vChay);
    LbMotion.waitDistanceMm(1);
  }
  LbMotion.stopAndWait();
}

// Khóa ngã tư chuẩn cho cả chặng dài lẫn chặng ngắn (như 85mm từ S1 về 2,3)
void do_line_den_giao_diem(int tongMm, int tocDo = V_LINE) {
  if (tongMm <= 0) return;
  long mocDau = LbMotion.getDistanceMm();

  // Chặng ngắn < 150mm bỏ qua 65% đầu; chặng dài bỏ qua (tongMm - 75)mm
  int doanBoQua = (tongMm < 150) ? ((tongMm * 65) / 100) : (tongMm - 75);

  while (true) {
    long daDi = lay_quang_duong(mocDau);
    int vChay = tocDo;
    if (tocDo > V_CHAM && (daDi < 25 || daDi >= doanBoQua)) {
      vChay = V_CHAM;
    }
    bam_line_mot_buoc(vChay);

    if (daDi >= doanBoQua) {
      byte vach = LbIRLine.value();
      if (vach == 0b1111 || vach == 0b1110 || vach == 0b0111) {
        LbMotion.runLR(V_CHAM, V_CHAM);
        LbMotion.waitDistanceMm(KC_TAM_BANH_MM);
        break;
      }
      if (daDi >= tongMm + 20) break; // Chốt an toàn nếu mất vạch ngang
    }
    LbMotion.waitDistanceMm(1);
  }
  LbMotion.stopAndWait();
}

int tinh_do_lech_goc(int gocHienTai, int gocDich) {
  int d = (gocDich - gocHienTai) % 360;
  if (d > 180)  d -= 360;
  if (d < -180) d += 360;
  return d;
}

void xoay_ve_huong(int gocDich) {
  int lech = tinh_do_lech_goc(curGoc, gocDich);
  if (lech > 0)      re_phai(lech);
  else if (lech < 0) re_trai(-lech);
  curGoc = ((gocDich % 360) + 360) % 360;
}

// Quay theo chiều chỉ định và dừng ngay khi 2 mắt giữa bắt trúng vạch đen (giới hạn cứng theo độ)
void xoay_tim_line_theo_chieu(bool quayPhai, int gocQuayNhanh, int gocQuetMax = GOC_QUET_LINE) {
  if (gocQuayNhanh > 0) {
    if (quayPhai) LbMotion.runLR(V_XOAY, -V_XOAY);
    else          LbMotion.runLR(-V_XOAY, V_XOAY);
    LbMotion.waitRotationDeg(gocQuayNhanh);
  }

  if (quayPhai) LbMotion.runLR(V_CHAM, -V_CHAM);
  else          LbMotion.runLR(-V_CHAM, V_CHAM);

  for (int deg = 0; deg < gocQuetMax; deg++) {
    LbIRLine.read();
    if ((LbIRLine.value() & 0b0110) != 0) break;
    LbMotion.waitRotationDeg(1);
  }
  LbMotion.stopAndWait();
}

// Xoay về trục line chính (0/90/180/270°) + tự khóa vạch đen chống lệch góc
void xoay_bat_line(int gocTruc) {
  int lech = tinh_do_lech_goc(curGoc, gocTruc);
  if (lech == 0) return;

  int gocCanQuay = abs(lech);
  if (gocCanQuay <= 20) {
    xoay_ve_huong(gocTruc);
    return;
  }

  int gocNhanh = max(10, tinh_goc_bu(gocCanQuay) - 22);
  xoay_tim_line_theo_chieu(lech > 0, gocNhanh, GOC_QUET_LINE);
  curGoc = ((gocTruc % 360) + 360) % 360;
}

// ============================================================================
// 4. ĐỊNH TUYẾN ĐƯỜNG NGẮN NHẤT (NÉ TUYỆT ĐỐI 4 GÓC BO TRÒN)
// ============================================================================
bool la_goc_bo_tron(uint8_t x, uint8_t y) {
  return (x == 0 && y == 0) || (x == 3 && y == 0) ||
         (x == 0 && y == 3) || (x == 3 && y == 3);
}

void di_doc_X(uint8_t dichX) {
  int dx = (int)dichX - (int)curX;
  if (dx == 0) return;
  xoay_bat_line(dx > 0 ? 90 : 270);
  do_line_den_giao_diem(abs(dx) * O_LUOI_MM);
  curX = dichX;
}

void di_doc_Y(uint8_t dichY) {
  int dy = (int)dichY - (int)curY;
  if (dy == 0) return;
  xoay_bat_line(dy > 0 ? 180 : 0);
  do_line_den_giao_diem(abs(dy) * O_LUOI_MM);
  curY = dichY;
}

void di_ngan_nhat_toi(uint8_t dichX, uint8_t dichY) {
  int dx = (int)dichX - (int)curX;
  int dy = (int)dichY - (int)curY;
  if (dx == 0 && dy == 0) return;

  if (dx != 0 && dy != 0) {
    bool xTruocBiBo = la_goc_bo_tron(dichX, curY);
    bool yTruocBiBo = la_goc_bo_tron(curX, dichY);

    if (xTruocBiBo && !yTruocBiBo) {
      di_doc_Y(dichY); di_doc_X(dichX);
    } else if (yTruocBiBo && !xTruocBiBo) {
      di_doc_X(dichX); di_doc_Y(dichY);
    } else {
      int gocX = (dx > 0) ? 90 : 270;
      int gocY = (dy > 0) ? 180 : 0;
      if (abs(tinh_do_lech_goc(curGoc, gocX)) <= abs(tinh_do_lech_goc(curGoc, gocY))) {
        di_doc_X(dichX); di_doc_Y(dichY);
      } else {
        di_doc_Y(dichY); di_doc_X(dichX);
      }
    }
  } else if (dx != 0) {
    di_doc_X(dichX);
  } else {
    di_doc_Y(dichY);
  }
}

// ============================================================================
// 5. CHIẾN THUẬT: ĐẨY 7CM + LẤY KHO (HỖ TRỢ TRÙNG MÀU) + XẾP QUẠT CRL
// ============================================================================
void day_7cm_va_lui(int kcVao = KC_VAO_KHOI) {
  LbGripper.moveTo(CANG_DAY);
  LbDelay(70);
  di_thang(KC_DAY_GIU_DIEM, V_CHAM);
  mo_cang();
  di_lui(kcVao + KC_DAY_GIU_DIEM, V_LUI_KHOI);
}

void lay_kho_S3_xanh_la() {
  int doSau = KC_VAO_S3 + daLayS3 * BUOC_KHO_CM;
  di_ngan_nhat_toi(2, 3);
  xoay_ve_huong(180);
  di_thang(doSau, V_CHAM);
  kep_cang();
  di_lui(doSau, V_LUI_KHOI);
  daLayS3++;
}

void lay_kho_S1_xanh_duong() {
  int doSau = KC_VAO_S1 + daLayS1 * BUOC_KHO_CM;
  di_ngan_nhat_toi(2, 3);
  xoay_bat_line(270);
  do_line_mm(KC_S3_SANG_S1_MM, V_CHAM);
  re_trai(90); curGoc = 180;
  di_thang(doSau, V_CHAM);
  kep_cang();
  di_lui(doSau, V_LUI_KHOI);
  xoay_bat_line(90);
  do_line_den_giao_diem(KC_S3_SANG_S1_MM, V_CHAM);
  curX = 2; curY = 3; curGoc = 90;
  daLayS1++;
}

void lay_kho_S4_tim() {
  int doSau = KC_VAO_S4 + daLayS4 * BUOC_KHO_CM;
  di_ngan_nhat_toi(3, 2);
  xoay_ve_huong(90);
  di_thang(doSau, V_CHAM);
  kep_cang();
  di_lui(doSau, V_LUI_KHOI);
  daLayS4++;
}

void lay_kho_theo_mau(MauKhoi m) {
  if (m == MAU_XANH_LA)         lay_kho_S3_xanh_la();
  else if (m == MAU_XANH_DUONG) lay_kho_S1_xanh_duong();
  else if (m == MAU_TIM)        lay_kho_S4_tim();
}

void tra_khoi_trung_hoa(uint8_t x, uint8_t y, int16_t huongVao, int doSau = KC_DAT_SAU) {
  di_ngan_nhat_toi(x, y);
  xoay_ve_huong(huongVao);
  di_thang(doSau, V_CHAM);
  mo_cang();
  di_lui(doSau, V_LUI_KHOI);
}

void dua_khoi_dang_kep_ve_CRL() {
  uint8_t idx = min((uint8_t)3, soKhoiCRL);
  di_ngan_nhat_toi(2, 2);
  xoay_ve_huong(CRL_GOC_THA[idx]);
  di_thang(CRL_DO_SAU[idx], V_CHAM);
  mo_cang();
  di_lui(CRL_DO_SAU[idx], V_LUI_KHOI);
  soKhoiCRL++;
}

// ============================================================================
// 6. ĐỘNG CƠ XỬ LÝ CHUNG CHO CẢ 32 VỊ TRÍ (29 Ô THƯỜNG + 3 GÓC BO A1, D2, F3)
// ============================================================================
void xu_ly_o_thuong(uint8_t x, uint8_t y, int16_t huongVao) {
  mo_cang();
  di_ngan_nhat_toi(x, y);
  xoay_ve_huong(huongVao);
  di_thang(KC_VAO_KHOI, V_CHAM);

  MauKhoi m = quet_mau();
  LbDelay(50);

  if (m == MAU_XANH_LA || m == MAU_XANH_DUONG || m == MAU_TIM) {
    day_7cm_va_lui(KC_VAO_KHOI);
    lay_kho_theo_mau(m);
    tra_khoi_trung_hoa(x, y, huongVao, KC_DAT_SAU);
  } else if (m == MAU_NGUY_HAI) {
    kep_cang();
    di_lui(KC_VAO_KHOI, V_LUI_KHOI);
    dua_khoi_dang_kep_ve_CRL();
  } else {
    // Ô trống -> lùi lại an toàn, không kẹp không khí
    di_lui(KC_VAO_KHOI, V_LUI_KHOI);
  }
}

void xu_ly_goc_bo(uint8_t neoX, uint8_t neoY, int16_t huongDi, bool rePhaiVao) {
  int16_t huongVe = (huongDi + 180) % 360;

  mo_cang();
  di_ngan_nhat_toi(neoX, neoY);
  xoay_bat_line(huongDi);
  do_line_mm(KC_GOC_BO_MM);

  if (rePhaiVao) re_phai(80);
  else           re_trai(80);
  di_thang(KC_VAO_GOC_BO, V_CHAM);

  MauKhoi m = quet_mau();
  LbDelay(50);

  if (m == MAU_XANH_LA || m == MAU_XANH_DUONG || m == MAU_TIM) {
    day_7cm_va_lui(KC_VAO_GOC_BO);
    // Quay 82° nhanh rồi quét chậm tìm vạch biên để bám về giao điểm neo
    xoay_tim_line_theo_chieu(rePhaiVao, 82, 42);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = neoX; curY = neoY; curGoc = huongVe;

    lay_kho_theo_mau(m);

    // Quay lại góc bo đặt khối trung hòa
    di_ngan_nhat_toi(neoX, neoY);
    xoay_bat_line(huongDi);
    do_line_mm(KC_GOC_BO_MM);
    if (rePhaiVao) re_phai(80);
    else           re_trai(80);
    di_thang(KC_DAT_SAU_BO, V_CHAM);
    mo_cang();
    di_lui(KC_DAT_SAU_BO, V_LUI_KHOI);

    xoay_tim_line_theo_chieu(rePhaiVao, 82, 42);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = neoX; curY = neoY; curGoc = huongVe;
  } else if (m == MAU_NGUY_HAI) {
    kep_cang();
    di_lui(KC_VAO_GOC_BO, V_LUI_KHOI);
    xoay_tim_line_theo_chieu(rePhaiVao, 82, 42);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = neoX; curY = neoY; curGoc = huongVe;
    dua_khoi_dang_kep_ve_CRL();
  } else {
    di_lui(KC_VAO_GOC_BO, V_LUI_KHOI);
    xoay_tim_line_theo_chieu(rePhaiVao, 82, 42);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = neoX; curY = neoY; curGoc = huongVe;
  }
}

void thuc_hien_o(ViTri vt) {
  ThongTinO o = BANG_32_O[vt];
  if (o.loai == 0) {
    xu_ly_o_thuong(o.x, o.y, o.goc);
  } else {
    xu_ly_goc_bo(o.x, o.y, o.goc, (o.loai == 1));
  }
}

// ============================================================================
// 7. XUẤT PHÁT TỰ BẮT VẠCH TỪ CRL & VỀ ĐÍCH KHÔNG HÚC KHỐI
// ============================================================================
void xuat_phat_tu_CRL() {
  // Đặt robot trong CRL hướng sang trái (270°): tự chạy chạm vạch x=2 -> rẽ lên (2,2)
  long mocDau = LbMotion.getDistanceMm();
  LbMotion.runLR(V_CHAM, V_CHAM);

  while (lay_quang_duong(mocDau) < 160) {
    LbIRLine.read();
    if ((LbIRLine.value() & 0b0110) != 0) {
      LbMotion.waitDistanceMm(KC_TAM_BANH_MM);
      break;
    }
    LbMotion.waitDistanceMm(1);
  }
  LbMotion.stopAndWait();

  curGoc = 270;
  xoay_bat_line(0);
  do_line_den_giao_diem(KC_CRL_LEN_22_MM, V_CHAM);

  curX = 2; curY = 2; curGoc = 0;
  soKhoiCRL = 0;
  daLayS3 = 0; daLayS1 = 0; daLayS4 = 0;
}

void ve_dich_CRL() {
  di_ngan_nhat_toi(2, 2);
  xoay_ve_huong(135);
  // Nếu trong CRL đã có khối nguy hại thì tiến nông hơn để đỗ gọn mà không đẩy văng khối
  int doSauVeDich = (soKhoiCRL == 0) ? 9 : 6;
  di_thang(doSauVeDich, V_CHAM);
}

// ============================================================================
// 8. SETUP & LOOP CHÍNH
// ============================================================================
void setup() {
  Leanbot.begin();
  if (BAT_DEBUG_SERIAL) Serial.begin(115200);
  LbIRLine.doManualCalibration(TB1A);
  APDS.begin();
  mo_cang();
  LbDelay(100);
}

void loop() {
  LbMission.begin(TB1A + TB1B);

  xuat_phat_tu_CRL();

  const uint8_t soLuongO = sizeof(DE_THI) / sizeof(DE_THI[0]);
  for (uint8_t i = 0; i < soLuongO; i++) {
    thuc_hien_o(DE_THI[i]);
  }

  ve_dich_CRL();

  Leanbot.tone(2200, 250);
  LbMission.end();
}