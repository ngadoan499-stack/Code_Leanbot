#include <Leanbot.h>
#include <Arduino_APDS9960.h>

// ============================================================================
// 1. CẤU HÌNH TỐC ĐỘ, BÙ GÓC (TRỪ 2-5 ĐỘ) & THÔNG SỐ SA BÀN V3
// ============================================================================
const int V_NHANH          = 1600;  // Chạy thẳng đoạn dài không tải (steps/s)
const int V_LINE           = 1500;  // Dò line tốc độ cao
const int V_LUI_KHOI       = 950;   // Lùi thoát khối êm, không văng lệch góc
const int V_XOAY           = 950;   // Xoay góc nhanh
const int V_CHAM           = 550;   // Phanh mềm / đẩy 7cm / khóa vạch

const int O_LUOI_MM        = 305;   // Độ dài 1 cạnh ô lưới (mm)
const int KC_TAM_BANH_MM   = 35;    // Khoảng cách từ mắt IR tới trục bánh (mm)
const int KC_GOC_BO_MM     = 265;   // Quãng đường đi tới 3 góc bo tròn A1, F3, D2 (mm)
const int KC_S3_SANG_S1_MM = 85;    // Khoảng cách từ (2,3)/S3 sang trái tới S1 (75-100mm)

const int BU_GOC_NHO       = 3;     // Trừ 3° cho góc <= 60° (45° -> 42°)
const int BU_GOC_LON       = 5;     // Trừ 5° cho góc > 60°  (90°->85°, 135°->130°, 180°->175°)

const int CANG_MO          = 0;     // Mở rộng càng
const int CANG_DAY         = 35;    // Góc chữ U giữ thẳng khối khi đẩy 7cm
const int CANG_KEP         = 90;    // Kẹp chặt khối trung hòa / khối nguy hại

const int KC_VAO_KHOI      = 8;     // Từ giao điểm chéo vào sát mặt khối (cm)
const int KC_VAO_GOC_BO    = 4;     // Từ mép cong vào sát mặt khối A1, F3, D2 (cm)
const int KC_DAY_GIU_DIEM  = 7;     // Quét màu xong đẩy thẳng 7cm giữ trọn điểm
const int KC_DAT_SAU       = 10;    // Độ sâu đặt khối trung hòa sau khối đã đẩy (cm)
const int KC_DAT_SAU_BO    = 6;     // Độ sâu đặt khối trung hòa ở 3 góc bo A1, F3, D2 (cm)

const int KC_VAO_S3        = 2;     // Tiến thêm tại (2,3) để kẹp S3 - Xanh lá (cm)
const int KC_VAO_S1        = 2;     // Tiến thêm tại S1 để kẹp Xanh dương (cm)
const int KC_VAO_S4        = 2;     // Tiến thêm tại (3,2) để kẹp S4 - Tím (cm)
const int KC_VAO_CRL       = 11;    // Độ sâu thả khối đầu tiên trong CRL (cm)

// Biến lưu trạng thái thực tế của robot trên lưới 4x4
uint8_t curX = 2;
uint8_t curY = 2;
int16_t curGoc = 0;
uint8_t soKhoiCRL = 0;

int ObjRed, ObjGreen, ObjBlue;

enum MauKhoi {
  MAU_XANH_LA = 0,   // -> Kho S3 (dưới 2,3)
  MAU_XANH_DUONG,    // -> Kho S1 (trái S3)
  MAU_TIM,           // -> Kho S4 (phải 3,2)
  MAU_NGUY_HAI       // Đỏ / Vàng / Bí ẩn -> Gom về CRL
};

// ============================================================================
// 2. ĐỌC MÀU VI SAI 3 KÊNH LED (GIỮ NGUYÊN LÕI CHUẨN TỪ FILE MẪU CỦA BẠN)
// ============================================================================
void _coreRead(int &r, int &g, int &b, CRGB ledColor) {
  LbRGB.fillColor(ledColor);
  LbRGB.show();
  while (!APDS.colorAvailable());
  APDS.readColor(r, g, b);
}

void doc_rgb() {
  int rRed, gGreen, bBlue, rr, gg, bb;
  int doSangGoc = LbRGB.getBrightness();
  LbRGB.setBrightness(255);

  _coreRead(rRed, gg, bb, CRGB::Red);
  _coreRead(rr, gGreen, bb, CRGB::Lime);
  _coreRead(rr, gg, bBlue, CRGB::Blue);
  _coreRead(rr, gg, bb, CRGB::Black);

  LbRGB.setBrightness(doSangGoc);
  ObjRed   = rRed - rr;
  ObjGreen = gGreen - gg;
  ObjBlue  = bBlue - bb;
}

MauKhoi quet_mau() {
  doc_rgb();
  if (ObjGreen > ObjRed && ObjGreen > ObjBlue && ObjGreen > 3) {
    LbRGB.fillColor(CRGB::Green); LbRGB.show();
    return MAU_XANH_LA;
  }
  if (ObjBlue > ObjRed && ObjBlue > ObjGreen && ObjBlue > 3 && ObjRed > 0 && ObjGreen > 0) {
    if ((long)ObjGreen < 2L * ObjRed) {
      LbRGB.fillColor(CRGB::Purple); LbRGB.show();
      return MAU_TIM;
    } else {
      LbRGB.fillColor(CRGB::Blue); LbRGB.show();
      return MAU_XANH_DUONG;
    }
  }
  LbRGB.fillColor(CRGB::Red); LbRGB.show();
  return MAU_NGUY_HAI;
}

// ============================================================================
// 3. BỘ HÀM TIẾNG VIỆT NÂNG CẤP (PHANH MỀM + BÙ GÓC 2-5° + BẮT LINE + KHÓA NGÃ TƯ)
// ============================================================================
int tinh_goc_bu(int gocChuan) {
  if (gocChuan <= 0)  return 0;
  if (gocChuan <= 15) return max(1, gocChuan - 2);  // Trừ 2° góc rất nhỏ
  if (gocChuan <= 60) return gocChuan - BU_GOC_NHO; // Trừ 3° (45° -> 42°)
  return gocChuan - BU_GOC_LON;                     // Trừ 5° (90°->85°, 135°->130°, 180°->175°)
}

void di_thang(int cm, int tocDo = V_NHANH) {
  int mm = cm * 10;
  if (mm <= 0) return;
  if (mm > 40 && tocDo > V_CHAM) {
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
  if (mm > 40 && tocDo > V_CHAM) {
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
  LbDelay(120);
}

void bam_line_mot_buoc(int tocDo) {
  LbIRLine.read();
  byte vach = LbIRLine.value();
  LbIRLine.displayOnRGB(0x00ff00);
  switch (vach) {
    case 0b0110:
    case 0b1111: LbMotion.runLR(tocDo, tocDo); break;
    case 0b0100: LbMotion.runLR(tocDo * 0.78, tocDo); break;
    case 0b0010: LbMotion.runLR(tocDo, tocDo * 0.78); break;
    case 0b1100:
    case 0b1110: LbMotion.runLR(tocDo * 0.35, tocDo); break;
    case 0b0011:
    case 0b0111: LbMotion.runLR(tocDo, tocDo * 0.35); break;
    case 0b1000: LbMotion.runLR(tocDo / 8, tocDo); break;
    case 0b0001: LbMotion.runLR(tocDo, tocDo / 8); break;
    default:     LbMotion.runLR(tocDo * 0.85, tocDo * 0.85); break;
  }
}

long lay_quang_duong(long mocDau) {
  long d = LbMotion.getDistanceMm() - mocDau;
  return (d < 0) ? -d : d;
}

// Bám line đúng quãng đường mm (dùng cho đoạn vào góc bo tròn hoặc sang S1)
void do_line_mm(int mm, int tocDo = V_LINE) {
  if (mm <= 0) return;
  long mocDau = LbMotion.getDistanceMm();
  while (true) {
    long daDi = lay_quang_duong(mocDau);
    if (daDi >= mm) break;
    int vChay = ((mm - daDi) < 40 && tocDo > V_CHAM) ? V_CHAM : tocDo;
    bam_line_mot_buoc(vChay);
    LbMotion.waitDistanceMm(1);
  }
  LbMotion.stopAndWait();
}

// Bám line bỏ qua vạch giữa cạnh & khóa chuẩn giao điểm thật (triệt tiêu sai số cộng dồn)
void do_line_den_giao_diem(int tongMm, int tocDo = V_LINE) {
  if (tongMm <= 0) return;
  long mocDau = LbMotion.getDistanceMm();
  int doanBoQua = max(15, tongMm - 75); // Chạy nhanh vượt qua vạch giữa cạnh

  while (true) {
    long daDi = lay_quang_duong(mocDau);
    int vChay = (daDi >= doanBoQua && tocDo > V_CHAM) ? V_CHAM : tocDo;
    bam_line_mot_buoc(vChay);

    if (daDi >= doanBoQua) {
      byte vach = LbIRLine.value();
      // Khi chạm vạch ngang của giao điểm (hoặc quá ngưỡng an toàn) -> căn tâm bánh lên giao điểm
      if (vach == 0b1111 || vach == 0b1110 || vach == 0b0111) {
        LbMotion.runLR(V_CHAM, V_CHAM);
        LbMotion.waitDistanceMm(KC_TAM_BANH_MM);
        break;
      }
      if (daDi >= tongMm + 15) break;
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

// Xoay tự do (dùng khi chéo 45/135/225/315° vào khối hoặc hướng vào S3/S1/S4/CRL)
void xoay_ve_huong(int gocDich) {
  int lech = tinh_do_lech_goc(curGoc, gocDich);
  if (lech > 0)      re_phai(lech);
  else if (lech < 0) re_trai(-lech);
  curGoc = ((gocDich % 360) + 360) % 360;
}

// Xoay về trục line chính (0/90/180/270°) + TỰ BẮT VẠCH ĐEN xóa sạch sai số góc
void xoay_bat_line(int gocTruc) {
  int lech = tinh_do_lech_goc(curGoc, gocTruc);
  if (lech == 0) return;

  int gocCanQuay = abs(lech);
  if (gocCanQuay <= 20) {
    xoay_ve_huong(gocTruc);
    return;
  }

  int gocNhanh = max(10, tinh_goc_bu(gocCanQuay) - 20);
  if (lech > 0) {
    LbMotion.runLR(V_XOAY, -V_XOAY);
    LbMotion.waitRotationDeg(gocNhanh);
    LbMotion.runLR(V_CHAM, -V_CHAM);
  } else {
    LbMotion.runLR(-V_XOAY, V_XOAY);
    LbMotion.waitRotationDeg(gocNhanh);
    LbMotion.runLR(-V_CHAM, V_CHAM);
  }

  // Quay chậm tới khi 2 mắt giữa (0b0110) khóa thẳng vạch đen mới (giới hạn an toàn 650ms)
  unsigned long t0 = millis();
  while (millis() - t0 < 650) {
    LbIRLine.read();
    if ((LbIRLine.value() & 0b0110) != 0) break;
  }
  LbMotion.stopAndWait();
  curGoc = ((gocTruc % 360) + 360) % 360;
}

// ============================================================================
// 4. BỘ ĐỊNH TUYẾN ĐƯỜNG NGẮN NHẤT (TUYỆT ĐỐI NÉ 4 GÓC BO TRÒN)
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
      // Cả 2 đều an toàn -> chọn trục tốn ít góc xoay nhất
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
// 5. CHIẾN THUẬT: ĐẨY 7CM GIỮ ĐIỂM + LẤY KHO (S3-LÁ, S1-DƯƠNG, S4-TÍM) + CRL
// ============================================================================
void day_7cm_va_lui(int kcVao = KC_VAO_KHOI) {
  LbGripper.moveTo(CANG_DAY);               // Càng chữ U ôm thẳng khối
  LbDelay(70);
  di_thang(KC_DAY_GIU_DIEM, V_CHAM);        // Đẩy thẳng 7cm giữ trọn điểm
  mo_cang();                                // Mở rộng trước khi lùi
  di_lui(kcVao + KC_DAY_GIU_DIEM, V_LUI_KHOI);
}

// Kho S3 (Xanh lá): Thẳng phía Nam giao điểm (2, 3)
void lay_kho_S3_xanh_la() {
  di_ngan_nhat_toi(2, 3);
  xoay_ve_huong(180);
  di_thang(KC_VAO_S3, V_CHAM);
  kep_cang();
  di_lui(KC_VAO_S3, V_LUI_KHOI);
}

// Kho S1 (Xanh dương): Lệch trái 85mm từ (2, 3) dọc theo vạch đáy y=3
void lay_kho_S1_xanh_duong() {
  di_ngan_nhat_toi(2, 3);
  xoay_bat_line(270);
  do_line_mm(KC_S3_SANG_S1_MM, V_CHAM);
  re_trai(90); curGoc = 180;
  di_thang(KC_VAO_S1, V_CHAM);
  kep_cang();
  di_lui(KC_VAO_S1, V_LUI_KHOI);
  xoay_bat_line(90);
  do_line_den_giao_diem(KC_S3_SANG_S1_MM, V_CHAM);
  curX = 2; curY = 3; curGoc = 90;
}

// Kho S4 (Tím): Thẳng phía Đông giao điểm (3, 2)
void lay_kho_S4_tim() {
  di_ngan_nhat_toi(3, 2);
  xoay_ve_huong(90);
  di_thang(KC_VAO_S4, V_CHAM);
  kep_cang();
  di_lui(KC_VAO_S4, V_LUI_KHOI);
}

void tra_khoi_trung_hoa(uint8_t x, uint8_t y, int16_t huongVao, int doSau = KC_DAT_SAU) {
  di_ngan_nhat_toi(x, y);
  xoay_ve_huong(huongVao);
  di_thang(doSau, V_CHAM);
  mo_cang();
  di_lui(doSau, V_LUI_KHOI);
}

void dua_khoi_dang_kep_ve_CRL() {
  di_ngan_nhat_toi(2, 2);
  xoay_ve_huong(135);
  int doSauCRL = max(6, KC_VAO_CRL - (int)(soKhoiCRL * 2));
  di_thang(doSauCRL, V_CHAM);
  mo_cang();
  di_lui(doSauCRL, V_LUI_KHOI);
  soKhoiCRL++;
}

void kep_va_dua_ve_CRL(int kcVao = KC_VAO_KHOI) {
  kep_cang();
  di_lui(kcVao, V_LUI_KHOI);
  dua_khoi_dang_kep_ve_CRL();
}

// ============================================================================
// 6. ĐẦY ĐỦ 32 TRƯỜNG HỢP RIÊNG BIỆT KHỚP BẢN ĐỒ SYNAPSE CITY V3 (A1-H4)
// ============================================================================

// ------------------------- KHU A (Hàng 0, Cột 0) -------------------------
// A1: Góc bo tròn trên-trái -> Neo (0,1), đi lên 265mm, rẽ phải vào A1
void truong_hop_A1() {
  mo_cang();
  di_ngan_nhat_toi(0, 1);
  xoay_bat_line(0);
  do_line_mm(KC_GOC_BO_MM);
  re_phai(80);
  di_thang(KC_VAO_GOC_BO, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);

  if (m == MAU_XANH_LA || m == MAU_XANH_DUONG || m == MAU_TIM) {
    day_7cm_va_lui(KC_VAO_GOC_BO);
    re_phai(105);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = 0; curY = 1; curGoc = 180;

    if (m == MAU_XANH_LA)         lay_kho_S3_xanh_la();
    else if (m == MAU_XANH_DUONG) lay_kho_S1_xanh_duong();
    else                          lay_kho_S4_tim();

    di_ngan_nhat_toi(0, 1);
    xoay_bat_line(0);
    do_line_mm(KC_GOC_BO_MM);
    re_phai(80);
    di_thang(KC_DAT_SAU_BO, V_CHAM);
    mo_cang();
    di_lui(KC_DAT_SAU_BO, V_LUI_KHOI);
    re_phai(105);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = 0; curY = 1; curGoc = 180;
  } else {
    kep_cang();
    di_lui(KC_VAO_GOC_BO, V_LUI_KHOI);
    re_phai(105);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = 0; curY = 1; curGoc = 180;
    dua_khoi_dang_kep_ve_CRL();
  }
}

void truong_hop_A2() {
  mo_cang(); di_ngan_nhat_toi(0, 1); xoay_ve_huong(45); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(0, 1, 45); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(0, 1, 45); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(0, 1, 45); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_A3() {
  mo_cang(); di_ngan_nhat_toi(1, 0); xoay_ve_huong(225); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 0, 225); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 0, 225); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 0, 225); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_A4() {
  mo_cang(); di_ngan_nhat_toi(1, 1); xoay_ve_huong(315); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 1, 315); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 1, 315); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 1, 315); }
  else                          { kep_va_dua_ve_CRL(); }
}

// ------------------------- KHU B (Hàng 1, Cột 0) -------------------------
void truong_hop_B1() {
  mo_cang(); di_ngan_nhat_toi(0, 1); xoay_ve_huong(135); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(0, 1, 135); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(0, 1, 135); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(0, 1, 135); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_B2() {
  mo_cang(); di_ngan_nhat_toi(0, 2); xoay_ve_huong(45); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(0, 2, 45); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(0, 2, 45); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(0, 2, 45); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_B3() {
  mo_cang(); di_ngan_nhat_toi(1, 1); xoay_ve_huong(225); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 1, 225); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 1, 225); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 1, 225); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_B4() {
  mo_cang(); di_ngan_nhat_toi(1, 2); xoay_ve_huong(315); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 2, 315); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 2, 315); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 2, 315); }
  else                          { kep_va_dua_ve_CRL(); }
}

// ------------------------- KHU C (Hàng 0, Cột 1) -------------------------
void truong_hop_C1() {
  mo_cang(); di_ngan_nhat_toi(1, 0); xoay_ve_huong(135); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 0, 135); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 0, 135); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 0, 135); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_C2() {
  mo_cang(); di_ngan_nhat_toi(1, 1); xoay_ve_huong(45); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 1, 45); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 1, 45); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 1, 45); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_C3() {
  mo_cang(); di_ngan_nhat_toi(2, 0); xoay_ve_huong(225); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 0, 225); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 0, 225); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 0, 225); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_C4() {
  mo_cang(); di_ngan_nhat_toi(2, 1); xoay_ve_huong(315); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 1, 315); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 1, 315); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 1, 315); }
  else                          { kep_va_dua_ve_CRL(); }
}

// ------------------------- KHU D (Hàng 2, Cột 0) -------------------------
void truong_hop_D1() {
  mo_cang(); di_ngan_nhat_toi(0, 2); xoay_ve_huong(135); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(0, 2, 135); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(0, 2, 135); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(0, 2, 135); }
  else                          { kep_va_dua_ve_CRL(); }
}

// D2: Góc bo tròn dưới-trái -> Neo (0,2), đi xuống 265mm, rẽ trái vào D2
void truong_hop_D2() {
  mo_cang();
  di_ngan_nhat_toi(0, 2);
  xoay_bat_line(180);
  do_line_mm(KC_GOC_BO_MM);
  re_trai(80);
  di_thang(KC_VAO_GOC_BO, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);

  if (m == MAU_XANH_LA || m == MAU_XANH_DUONG || m == MAU_TIM) {
    day_7cm_va_lui(KC_VAO_GOC_BO);
    re_trai(105);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = 0; curY = 2; curGoc = 0;

    if (m == MAU_XANH_LA)         lay_kho_S3_xanh_la();
    else if (m == MAU_XANH_DUONG) lay_kho_S1_xanh_duong();
    else                          lay_kho_S4_tim();

    di_ngan_nhat_toi(0, 2);
    xoay_bat_line(180);
    do_line_mm(KC_GOC_BO_MM);
    re_trai(80);
    di_thang(KC_DAT_SAU_BO, V_CHAM);
    mo_cang();
    di_lui(KC_DAT_SAU_BO, V_LUI_KHOI);
    re_trai(105);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = 0; curY = 2; curGoc = 0;
  } else {
    kep_cang();
    di_lui(KC_VAO_GOC_BO, V_LUI_KHOI);
    re_trai(105);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = 0; curY = 2; curGoc = 0;
    dua_khoi_dang_kep_ve_CRL();
  }
}

void truong_hop_D3() {
  mo_cang(); di_ngan_nhat_toi(1, 2); xoay_ve_huong(225); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 2, 225); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 2, 225); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 2, 225); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_D4() {
  mo_cang(); di_ngan_nhat_toi(1, 3); xoay_ve_huong(315); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 3, 315); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 3, 315); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 3, 315); }
  else                          { kep_va_dua_ve_CRL(); }
}

// ------------------------- KHU E (Hàng 1, Cột 1) -------------------------
void truong_hop_E1() {
  mo_cang(); di_ngan_nhat_toi(1, 1); xoay_ve_huong(135); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 1, 135); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 1, 135); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 1, 135); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_E2() {
  mo_cang(); di_ngan_nhat_toi(1, 2); xoay_ve_huong(45); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 2, 45); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 2, 45); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 2, 45); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_E3() {
  mo_cang(); di_ngan_nhat_toi(2, 1); xoay_ve_huong(225); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 1, 225); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 1, 225); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 1, 225); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_E4() {
  mo_cang(); di_ngan_nhat_toi(2, 2); xoay_ve_huong(315); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 2, 315); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 2, 315); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 2, 315); }
  else                          { kep_va_dua_ve_CRL(); }
}

// ------------------------- KHU F (Hàng 0, Cột 2) -------------------------
void truong_hop_F1() {
  mo_cang(); di_ngan_nhat_toi(2, 0); xoay_ve_huong(135); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 0, 135); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 0, 135); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 0, 135); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_F2() {
  mo_cang(); di_ngan_nhat_toi(2, 1); xoay_ve_huong(45); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 1, 45); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 1, 45); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 1, 45); }
  else                          { kep_va_dua_ve_CRL(); }
}

// F3: Góc bo tròn trên-phải -> Neo (3,1), đi lên 265mm, rẽ trái vào F3
void truong_hop_F3() {
  mo_cang();
  di_ngan_nhat_toi(3, 1);
  xoay_bat_line(0);
  do_line_mm(KC_GOC_BO_MM);
  re_trai(80);
  di_thang(KC_VAO_GOC_BO, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);

  if (m == MAU_XANH_LA || m == MAU_XANH_DUONG || m == MAU_TIM) {
    day_7cm_va_lui(KC_VAO_GOC_BO);
    re_trai(105);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = 3; curY = 1; curGoc = 180;

    if (m == MAU_XANH_LA)         lay_kho_S3_xanh_la();
    else if (m == MAU_XANH_DUONG) lay_kho_S1_xanh_duong();
    else                          lay_kho_S4_tim();

    di_ngan_nhat_toi(3, 1);
    xoay_bat_line(0);
    do_line_mm(KC_GOC_BO_MM);
    re_trai(80);
    di_thang(KC_DAT_SAU_BO, V_CHAM);
    mo_cang();
    di_lui(KC_DAT_SAU_BO, V_LUI_KHOI);
    re_trai(105);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = 3; curY = 1; curGoc = 180;
  } else {
    kep_cang();
    di_lui(KC_VAO_GOC_BO, V_LUI_KHOI);
    re_trai(105);
    do_line_den_giao_diem(KC_GOC_BO_MM);
    curX = 3; curY = 1; curGoc = 180;
    dua_khoi_dang_kep_ve_CRL();
  }
}

void truong_hop_F4() {
  mo_cang(); di_ngan_nhat_toi(3, 1); xoay_ve_huong(315); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(3, 1, 315); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(3, 1, 315); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(3, 1, 315); }
  else                          { kep_va_dua_ve_CRL(); }
}

// ------------------------- KHU G (Hàng 2, Cột 1) -------------------------
void truong_hop_G1() {
  mo_cang(); di_ngan_nhat_toi(1, 2); xoay_ve_huong(135); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 2, 135); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 2, 135); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 2, 135); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_G2() {
  mo_cang(); di_ngan_nhat_toi(1, 3); xoay_ve_huong(45); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(1, 3, 45); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(1, 3, 45); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(1, 3, 45); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_G3() {
  mo_cang(); di_ngan_nhat_toi(2, 2); xoay_ve_huong(225); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 2, 225); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 2, 225); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 2, 225); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_G4() {
  mo_cang(); di_ngan_nhat_toi(2, 3); xoay_ve_huong(315); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 3, 315); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 3, 315); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 3, 315); }
  else                          { kep_va_dua_ve_CRL(); }
}

// ------------------------- KHU H (Hàng 1, Cột 2) -------------------------
void truong_hop_H1() {
  mo_cang(); di_ngan_nhat_toi(2, 1); xoay_ve_huong(135); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 1, 135); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 1, 135); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 1, 135); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_H2() {
  mo_cang(); di_ngan_nhat_toi(2, 2); xoay_ve_huong(45); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(2, 2, 45); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(2, 2, 45); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(2, 2, 45); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_H3() {
  mo_cang(); di_ngan_nhat_toi(3, 1); xoay_ve_huong(225); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(3, 1, 225); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(3, 1, 225); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(3, 1, 225); }
  else                          { kep_va_dua_ve_CRL(); }
}

void truong_hop_H4() {
  mo_cang(); di_ngan_nhat_toi(3, 2); xoay_ve_huong(315); di_thang(KC_VAO_KHOI, V_CHAM);
  MauKhoi m = quet_mau(); LbDelay(60);
  if (m == MAU_XANH_LA)         { day_7cm_va_lui(); lay_kho_S3_xanh_la();    tra_khoi_trung_hoa(3, 2, 315); }
  else if (m == MAU_XANH_DUONG) { day_7cm_va_lui(); lay_kho_S1_xanh_duong(); tra_khoi_trung_hoa(3, 2, 315); }
  else if (m == MAU_TIM)        { day_7cm_va_lui(); lay_kho_S4_tim();        tra_khoi_trung_hoa(3, 2, 315); }
  else                          { kep_va_dua_ve_CRL(); }
}

// ============================================================================
// 7. XUẤT PHÁT TỪ CRL & VỀ ĐÍCH TRONG CRL
// ============================================================================
void xuat_phat_tu_CRL() {
  // Đặt robot trong CRL hướng sang trái giống bài mẫu: đi 10cm ra vạch x=2 -> rẽ phải lên (2,2)
  di_thang(10);
  re_phai(90);
  do_line_den_giao_diem(100);
  curX = 2; curY = 2; curGoc = 0;
  soKhoiCRL = 0;
}

void ve_dich_CRL() {
  di_ngan_nhat_toi(2, 2);
  xoay_ve_huong(135);
  di_thang(9, V_CHAM);
}

// ============================================================================
// 8. HÀM BẮT BUỘC (setup & loop)
// ============================================================================
void setup() {
  Leanbot.begin();
  LbIRLine.doManualCalibration(TB1A);
  APDS.begin();
  mo_cang();
  LbDelay(100);
}

void loop() {
  LbMission.begin(TB1A + TB1B);

  xuat_phat_tu_CRL();

  // === GHI ĐÚNG THỨ TỰ CÁC TRƯỜNG HỢP TRONG 32 HÀM (A1..H4) TẠI ĐÂY ===
  truong_hop_H1();
  truong_hop_F3();
  truong_hop_C1();
  truong_hop_E2();

  ve_dich_CRL();

  Leanbot.tone(2200, 250);
  LbMission.end();
}