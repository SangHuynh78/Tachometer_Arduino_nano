// =====================================================================
//  BƯỚC 2: Xác định thời điểm vật đi qua tâm từng cảm biến
//  (chỉ xét vật cách cảm biến < DETECT_CM = 100 cm, xa hơn coi như -1)
//
//  - Đo luân phiên A, B; mỗi mẫu gắn thời điểm millis().
//  - Khi vật đi qua hết 1 cảm biến, in ra Serial (115200):
//      chuỗi khoảng cách (-1 = không có vật) + thời điểm từng mẫu
//      t_min  : lúc khoảng cách nhỏ nhất
//      t_giua : giữa lần đầu và lần cuối thấy vật
//  - Nếu cả A và B cùng có kết quả gần nhau, in chênh lệch thời gian
//    (bước sau: vận tốc = khoảng cách 2 cảm biến / chênh lệch này).
//  - LCD: khoảng cách hiện tại + thời điểm qua tâm gần nhất (giây).
//
//  Nối dây: A: TRIG D2, ECHO D3 | B: TRIG D4, ECHO D5 | LCD I2C: A4, A5
// =====================================================================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "hc-sr04.h"
#include "pass-detect.h"

// ---- Cấu hình ----
#define PIN_TRIG_A     2
#define PIN_ECHO_A     3
#define PIN_TRIG_B     4
#define PIN_ECHO_B     5
#define MAX_CM         110    // chỉ chờ vọng trong 110 cm (~6,4 ms)
#define DETECT_CM      100    // chỉ tính vật gần hơn 100 cm; xa hơn -> bỏ qua (ghi -1)
#define END_MISSES     4      // số mẫu trống liên tiếp -> vật đã đi qua
#define PING_GAP_MS    12     // khoảng cách giữa 2 lần phát (A,B luân phiên -> mỗi cảm biến 24 ms/mẫu)
                              // số nhảy lung tung khi không có vật -> tăng lên 15
#define PAIR_WINDOW_MS 5000   // A và B cách nhau quá mức này thì không ghép cặp
#define LCD_ADDR       0      // 0 = tự dò

HCSR04       sensor[2];
PassDetector det[2];
LiquidCrystal_I2C *lcd = nullptr;

float    lastCm[2]     = { HCSR04_NO_ECHO, HCSR04_NO_ECHO };
bool     printPending[2] = { false, false };
bool     unpaired[2]     = { false, false };
uint32_t lastPassMs[2]   = { 0, 0 };

// ---------------------------------------------------------------------
uint8_t findLcdAddress() {
  Wire.begin();
  for (uint8_t a = 0x20; a <= 0x3F; a++) {
    if (LCD_ADDR != 0 && a != LCD_ADDR) continue;
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) return a;
  }
  return 0;
}

// In số căn phải trong ô rộng 'w' ký tự
void printCell(const char *s, uint8_t w) {
  for (uint8_t k = strlen(s); k < w; k++) Serial.print(' ');
  Serial.print(s);
}

// In kết quả một lần đi qua: chỉ thời điểm qua tâm (t_min)
void printPass(PassDetector *p) {
  Serial.print(F("THOI DIEM QUA TAM ")); Serial.print(p->name);
  Serial.print(F(": t_min = ")); Serial.print(p->res.tMin);
  Serial.println(F(" ms"));
}

// Nếu A và B đều có kết quả chưa ghép, in chênh lệch thời gian
void tryPair() {
  if (!unpaired[0] || !unpaired[1]) return;
  PdResult &a = det[0].res, &b = det[1].res;
  long dMin = (long)b.tMin - (long)a.tMin;
  long dMid = (long)b.tMid - (long)a.tMid;
  unpaired[0] = unpaired[1] = false;
  if (labs(dMid) > PAIR_WINDOW_MS) return;

  Serial.println();
  Serial.print(F("=== CAP A-B: huong ")); Serial.println(dMid >= 0 ? F("A -> B") : F("B -> A"));
  Serial.print(F("    chenh lech theo t_min  = ")); Serial.print(labs(dMin)); Serial.println(F(" ms"));
  Serial.print(F("    chenh lech theo t_giua = ")); Serial.print(labs(dMid)); Serial.println(F(" ms"));
}

// LCD: "A:123.4 @ 12.35s"
// Mỗi lần gọi chỉ ghi 1 dòng (~20 ms) để không làm trễ nhịp đo quá lâu
void updateLcd() {
  static uint8_t i = 0;
  if (!lcd) return;
  {
    char line[17], d[8], t[8];
    if (lastCm[i] < 0) strcpy(d, "  ---");
    else               dtostrf(lastCm[i], 5, 1, d);
    dtostrf(lastPassMs[i] / 1000.0, 6, 2, t);
    snprintf(line, sizeof(line), "%c:%s @%ss", det[i].name, d, t);
    lcd->setCursor(0, i);
    lcd->print(line);
  }
  i ^= 1;
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println(F("# Buoc 2: thoi diem vat qua tam cam bien"));

  hcsr04_init(&sensor[0], PIN_TRIG_A, PIN_ECHO_A, MAX_CM);
  hcsr04_init(&sensor[1], PIN_TRIG_B, PIN_ECHO_B, MAX_CM);
  pd_init(&det[0], 'A', DETECT_CM, END_MISSES);
  pd_init(&det[1], 'B', DETECT_CM, END_MISSES);

  uint8_t addr = findLcdAddress();
  if (addr) {
    lcd = new LiquidCrystal_I2C(addr, 16, 2);
    lcd->init();
    lcd->backlight();
    lcd->print("Buoc 2: qua tam");
    delay(800);
  } else {
    Serial.println(F("# KHONG TIM THAY LCD - kiem tra SDA=A4, SCL=A5"));
  }
}

void loop() {
  static uint8_t  turn = 0;
  static uint32_t lastPing = 0, lastLcd = 0;

  // ---- 1. Đo luân phiên, đúng nhịp PING_GAP_MS ----
  uint32_t now = millis();
  if (now - lastPing >= PING_GAP_MS) {
    lastPing = now;
    uint8_t i = turn;
    turn ^= 1;

    float cm = hcsr04_read_cm(&sensor[i]);
    if (cm >= DETECT_CM) cm = HCSR04_NO_ECHO;   // vật xa hơn DETECT_CM -> bỏ qua
    lastCm[i] = cm;
    if (pd_update(&det[i], now, cm)) {
      printPending[i] = true;
      unpaired[i]     = true;
      lastPassMs[i]   = det[i].res.tMin;
    }
  }

  // ---- 2. Chỉ in / cập nhật LCD khi KHÔNG có vật trước cảm biến nào ----
  //         (in Serial và ghi LCD tốn thời gian, làm trễ nhịp lấy mẫu)
  bool busy = det[0].active || det[1].active;
  if (!busy) {
    if (printPending[0] || printPending[1]) {
      // in theo thứ tự thời gian
      uint8_t first = (printPending[0] && printPending[1] &&
                       det[1].res.tIn < det[0].res.tIn) ? 1 : 0;
      for (uint8_t k = 0; k < 2; k++) {
        uint8_t i = first ^ k;
        if (printPending[i]) { printPass(&det[i]); printPending[i] = false; }
      }
    } else if (millis() - lastLcd > 250) {
      lastLcd = millis();
      updateLcd();
    }
  }
}
