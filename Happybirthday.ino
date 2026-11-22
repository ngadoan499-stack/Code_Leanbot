#include <Leanbot.h>

// ============================================================================
// ĐỊNH NGHĨA TẦN SỐ SÓNG ÂM CÁC NỐT NHẠC (Hz)
// ============================================================================
#define NOT_C5   523   // Đô
#define NOT_D5   587   // Rê
#define NOT_E5   659   // Mi
#define NOT_F5   698   // Pha
#define NOT_G5   784   // Sol
#define NOT_A5   880   // La
#define NOT_AS5  932   // Si giáng (Bb)
#define NOT_C6   1047  // Đô cao

// ============================================================================
// GIAI ĐIỆU & TRƯỜNG ĐỘ BÀI HAPPY BIRTHDAY (25 nốt)
// ============================================================================
const int giaiDieu[] = {
  // Câu 1: Happy Birthday to you
  NOT_C5, NOT_C5, NOT_D5, NOT_C5, NOT_F5, NOT_E5,
  // Câu 2: Happy Birthday to you
  NOT_C5, NOT_C5, NOT_D5, NOT_C5, NOT_G5, NOT_F5,
  // Câu 3: Happy Birthday dear [Name]
  NOT_C5, NOT_C5, NOT_C6, NOT_A5, NOT_F5, NOT_E5, NOT_D5,
  // Câu 4: Happy Birthday to you
  NOT_AS5, NOT_AS5, NOT_A5, NOT_F5, NOT_G5, NOT_F5
};

const int nhipNot[] = {
  // Câu 1
  300, 150, 450, 450, 450, 900,
  // Câu 2
  300, 150, 450, 450, 450, 900,
  // Câu 3
  300, 150, 450, 450, 450, 450, 900,
  // Câu 4
  300, 150, 450, 450, 450, 1000
};

// ============================================================================
// HÀM PHÁT NHẠC HAPPY BIRTHDAY
// ============================================================================
void phat_nhac_happy_birthday(float tocDo = 1.0) {
  int tongSoNot = sizeof(giaiDieu) / sizeof(giaiDieu[0]);

  for (int i = 0; i < tongSoNot; i++) {
    int thoiGianKeu = nhipNot[i] / tocDo;

    // Phát sóng âm đúng tần số và thời lượng của nốt thứ i
    Leanbot.tone(giaiDieu[i], thoiGianKeu);

    // Chờ hết thời lượng nốt + 25% khoảng lặng để tách rõ 2 nốt trùng nhau (Đô - Đô)
    LbDelay(thoiGianKeu * 1.25);
  }
}

// ============================================================================
// CHƯƠNG TRÌNH CHÍNH
// ============================================================================
void setup() {
  Leanbot.begin();

  // Phát ngay 1 lần khi bật nguồn robot
  phat_nhac_happy_birthday(1.0);
}

void loop() {
  // Chạm nút cảm ứng TB1A nếu muốn phát lại bài hát bất cứ lúc nào
  if (LbTouch.onPress(TB1A)) {
    phat_nhac_happy_birthday(1.0);
  }
}