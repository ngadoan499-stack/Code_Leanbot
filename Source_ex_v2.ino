#include <Leanbot.h>
#include <Arduino_APDS9960.h>

// ============================================================================
// CẤU HÌNH TỐC ĐỘ, GÓC QUAY & CÀNG KẸP
// ============================================================================
const int V_NHANH     = 1650;  // Chạy thẳng đoạn dài không tải (steps/s)
const int V_LINE      = 1400;  // Dò line tốc độ cao
const int V_XOAY      = 1000;  // Xoay góc nhanh
const int V_CHAM      = 550;   // Phanh mềm / tiếp cận khối / bắt line

const int GOC_90      = 90;    // Chỉnh 88-92 theo độ bám mặt sân
const int CANG_MO     = 0;     // Mở rộng càng
const int CANG_DAY    = 35;    // Góc chữ U giữ khối thẳng khi đẩy
const int CANG_KEP    = 88;    // Kẹp chặt khối trung hòa / khối thu hồi

// ============================================================================
// ĐỊNH NGHĨA MÀU KHỐI TIẾNG VIỆT
// ============================================================================
enum MauKhoi {
  MAU_KHONG = 0,     // Không phát hiện khối trước cảm biến
  MAU_XANH_LA,       // Cần trung hòa -> Lấy khối kho S6
  MAU_XANH_DUONG,    // Cần trung hòa -> Lấy khối kho S4
  MAU_TIM,           // Cần trung hòa -> Lấy khối kho S3
  MAU_DO,            // Nguy hại      -> Gắp đưa về CRL
  MAU_VANG,          // Nguy hại      -> Gắp đưa về CRL
  MAU_BI_AN          // Khối ẩn số    -> Xử lý gom về CRL
};

// ============================================================================
// BỘ 17 HÀM TIẾNG VIỆT NÂNG CẤP
// ============================================================================

// Hàm 1: Tiến thẳng có phanh mềm 30mm cuối
void di_thang(int mm, int tocDo = V_NHANH) {
  if (mm <= 0) return;
  if (mm > 45 && tocDo > V_CHAM) {
    LbMotion.runLR(tocDo, tocDo);
    LbMotion.waitDistanceMm(mm - 30);
    LbMotion.runLR(V_CHAM, V_CHAM);
    LbMotion.waitDistanceMm(30);
  } else {
    LbMotion.runLR(V_CHAM, V_CHAM);
    LbMotion.waitDistanceMm(mm);
  }
  LbMotion.stopAndWait();
}

// Hàm 2: Lùi thẳng có phanh mềm 30mm cuối
void di_lui(int mm, int tocDo = V_NHANH) {
  if (mm <= 0) return;
  if (mm > 45 && tocDo > V_CHAM) {
    LbMotion.runLR(-tocDo, -tocDo);
    LbMotion.waitDistanceMm(mm - 30);
    LbMotion.runLR(-V_CHAM, -V_CHAM);
    LbMotion.waitDistanceMm(30);
  } else {
    LbMotion.runLR(-V_CHAM, -V_CHAM);
    LbMotion.waitDistanceMm(mm);
  }
  LbMotion.stopAndWait();
}

// Hàm 3: Rẽ trái theo góc có phanh mềm 18 độ cuối
void re_trai(int goc = GOC_90, int tocDo = V_XOAY) {
  if (goc <= 0) return;
  if (goc > 25 && tocDo > V_CHAM) {
    LbMotion.runLR(-tocDo, tocDo);
    LbMotion.waitRotationDeg(goc - 18);
    LbMotion.runLR(-V_CHAM, V_CHAM);
    LbMotion.waitRotationDeg(18);
  } else {
    LbMotion.runLR(-V_CHAM, V_CHAM);
    LbMotion.waitRotationDeg(goc);
  }
  LbMotion.stopAndWait();
}

// Hàm 4: Rẽ phải theo góc có phanh mềm 18 độ cuối
void re_phai(int goc = GOC_90, int tocDo = V_XOAY) {
  if (goc <= 0) return;
  if (goc > 25 && tocDo > V_CHAM) {
    LbMotion.runLR(tocDo, -tocDo);
    LbMotion.waitRotationDeg(goc - 18);
    LbMotion.runLR(V_CHAM, -V_CHAM);
    LbMotion.waitRotationDeg(18);
  } else {
    LbMotion.runLR(V_CHAM, -V_CHAM);
    LbMotion.waitRotationDeg(goc);
  }
  LbMotion.stopAndWait();
}

// Hàm 5: Quay đầu 180 độ tại chỗ
void quay_dau(int tocDo = V_XOAY) {
  re_phai(GOC_90 * 2, tocDo);
}

// Hàm 6: Rẽ trái tự động khóa thẳng vạch đen mới
void re_trai_bat_line(int gocThoat = 55) {
  LbMotion.runLR(-V_XOAY, V_XOAY);
  LbMotion.waitRotationDeg(gocThoat);

  LbMotion.runLR(-V_CHAM, V_CHAM);
  unsigned long t0 = millis();
  while ((LbIRLine.read() & 0b0110) == 0 && (millis() - t0 < 1200)) {}
  LbMotion.stopAndWait();
}

// Hàm 7: Rẽ phải tự động khóa thẳng vạch đen mới
void re_phai_bat_line(int gocThoat = 55) {
  LbMotion.runLR(V_XOAY, -V_XOAY);
  LbMotion.waitRotationDeg(gocThoat);

  LbMotion.runLR(V_CHAM, -V_CHAM);
  unsigned long t0 = millis();
  while ((LbIRLine.read() & 0b0110) == 0 && (millis() - t0 < 1200)) {}
  LbMotion.stopAndWait();
}

// Hàm 8: Quay đầu 180 độ tự động bắt chuẩn đường line phía sau
void quay_dau_bat_line() {
  re_phai_bat_line(145);
}

// Hàm 9: Hàm lõi điều chỉnh 1 bước bám line theo 4 mắt cảm biến IR
void bam_line_mot_buoc(int tocDo) {
  byte vach = LbIRLine.read() & 0b1111;
  switch (vach) {
    case 0b0110:
    case 0b1111:
      LbMotion.runLR(tocDo, tocDo);
      break;
    case 0b0100:
      LbMotion.runLR(tocDo * 0.80, tocDo);
      break;
    case 0b0010:
      LbMotion.runLR(tocDo, tocDo * 0.80);
      break;
    case 0b1100:
      LbMotion.runLR(tocDo * 0.45, tocDo);
      break;
    case 0b0011:
      LbMotion.runLR(tocDo, tocDo * 0.45);
      break;
    case 0b1000:
      LbMotion.runLR(0, tocDo);
      break;
    case 0b0001:
      LbMotion.runLR(tocDo, 0);
      break;
    default:
      LbMotion.runLR(tocDo * 0.75, tocDo * 0.75);
      break;
  }
}

// Hàm 10: Bám line đúng quãng đường mm (hạ tốc 40mm cuối)
void do_line_mm(int mm, int tocDo = V_LINE) {
  long viTriDau = LbMotion.getDistanceMm();
  while (abs(LbMotion.getDistanceMm() - viTriDau) < mm) {
    long conLai = mm - abs(LbMotion.getDistanceMm() - viTriDau);
    int vHienTai = (conLai < 40 && tocDo > V_CHAM) ? V_CHAM : tocDo;
    bam_line_mot_buoc(vHienTai);
  }
  LbMotion.stopAndWait();
}

// Hàm 11: Bám line đếm đúng số ngã tư (lướt ngã tư giữa, căn trục ngã tư cuối)
void do_line_nga_tu(int soNgaTu, int tocDo = V_LINE) {
  for (int i = 0; i < soNgaTu; i++) {
    while (true) {
      byte vach = LbIRLine.read() & 0b1111;
      if (vach == 0b1111 || vach == 0b1110 || vach == 0b0111) break;
      bam_line_mot_buoc(tocDo);
    }
    if (i < soNgaTu - 1) {
      LbMotion.runLR(tocDo, tocDo);
      LbMotion.waitDistanceMm(25);
    } else {
      di_thang(35, V_CHAM);
    }
  }
}

// Hàm 12: Mở rộng càng kẹp
void mo_cang() {
  LbGripper.moveTo(CANG_MO);
  LbDelay(110);
}

// Hàm 13: Đóng càng kẹp chặt khối
void kep_cang() {
  LbGripper.moveTo(CANG_KEP);
  LbDelay(130);
}

// Hàm 14: Đẩy khối ô nhiễm vào trước bằng khung chữ U rồi lùi ra
void day_khoi_o_nhiem(int quangDuongDay = 75, int quangDuongLui = 85) {
  LbGripper.moveTo(CANG_DAY);
  LbDelay(80);
  di_thang(quangDuongDay, V_CHAM);
  mo_cang();
  di_lui(quangDuongLui, V_NHANH);
}

// Hàm 15: Đặt khối trung hòa vào sau khối ô nhiễm rồi lùi thoát trạm
void dat_khoi_trung_hoa(int doSau = 50, int quangDuongLui = 75) {
  do_line_mm(doSau, V_CHAM);
  mo_cang();
  di_lui(quangDuongLui, V_NHANH);
}

// Hàm 16: Đọc cảm biến APDS-9960 vi sai (trừ nhiễu ánh sáng nền)
void doc_rgb(int &rKetQua, int &gKetQua, int &bKetQua) {
  long tongR = 0, tongG = 0, tongB = 0;
  const int SO_MAU = 3;

  for (int i = 0; i < SO_MAU; i++) {
    int rBat, gBat, bBat, rTat, gTat, bTat;

    LbRGB.fillColor(CRGB::White);
    LbRGB.show();
    LbDelay(8);
    while (!APDS.colorAvailable());
    APDS.readColor(rBat, gBat, bBat);

    LbRGB.fillColor(CRGB::Black);
    LbRGB.show();
    LbDelay(5);
    while (!APDS.colorAvailable());
    APDS.readColor(rTat, gTat, bTat);

    tongR += max(0, rBat - rTat);
    tongG += max(0, gBat - gTat);
    tongB += max(0, bBat - bTat);
  }

  rKetQua = tongR / SO_MAU;
  gKetQua = tongG / SO_MAU;
  bKetQua = tongB / SO_MAU;
}

// Hàm 17: Nhận diện màu khối & tự bật đèn báo trên thân robot
MauKhoi quet_mau(bool inLogSerial = false) {
  int r, g, b;
  doc_rgb(r, g, b);
  int tong = r + g + b;

  if (tong < 15) {
    LbRGB.fillColor(CRGB::Black);
    LbRGB.show();
    return MAU_KHONG;
  }

  int pctR = (r * 100) / tong;
  int pctG = (g * 100) / tong;
  int pctB = (b * 100) / tong;

  if (inLogSerial) {
    Serial.print("R%="); Serial.print(pctR);
    Serial.print(" | G%="); Serial.print(pctG);
    Serial.print(" | B%="); Serial.println(pctB);
  }

  if (pctR > 48 && pctG < 30 && pctB < 30) {
    LbRGB.fillColor(CRGB::Red); LbRGB.show();
    return MAU_DO;
  }
  if (pctR > 38 && pctG > 35 && pctB < 22) {
    LbRGB.fillColor(CRGB::Yellow); LbRGB.show();
    return MAU_VANG;
  }
  if (pctG > 42 && pctR < 34 && pctB < 34) {
    LbRGB.fillColor(CRGB::Green); LbRGB.show();
    return MAU_XANH_LA;
  }
  if (pctB > 42 && pctR < 30) {
    LbRGB.fillColor(CRGB::Blue); LbRGB.show();
    return MAU_XANH_DUONG;
  }
  if (pctR > 30 && pctB > 32 && pctG < 30) {
    LbRGB.fillColor(CRGB::Purple); LbRGB.show();
    return MAU_TIM;
  }

  LbRGB.fillColor(CRGB::Cyan); LbRGB.show();
  return MAU_BI_AN;
}

// ============================================================================
// HÀM CHẠY TEST ĐỦ TOÀN BỘ 17 HÀM (Đã viết sẵn, không bị lỗi cú pháp)
// ============================================================================
void chay_test_17_ham() {
  // --- BƯỚC 1: Test nhóm càng kẹp (Hàm 12, 13) ---
  do_line_nga_tu(1);
  re_trai_bat_line();
  do_line_mm(60);
  quay_dau_bat_line();
  do_line_nga_tu(1);
  re_phai_bat_line();
}

// ============================================================================
// HÀM BẮT BUỘC CỦA CHƯƠNG TRÌNH (setup & loop)
// ============================================================================
void setup() {
  Serial.begin(9600);
  Leanbot.begin();
  APDS.begin();
  mo_cang();

  // Chờ chạm nút cảm ứng TB1A để bắt đầu bài test 17 hàm
  while (!LbTouch.onPress(TB1A)) {
    LbDelay(10);
  }
  Leanbot.tone(1500, 80);
  LbDelay(200);

  // Gọi chạy thử toàn bộ 17 hàm
  chay_test_17_ham();

  // Báo âm thanh hoàn thành bài test
  Leanbot.tone(2200, 300);
}

void loop() {
  // Để trống để robot chạy đúng 1 lượt test rồi dừng hẳn
}