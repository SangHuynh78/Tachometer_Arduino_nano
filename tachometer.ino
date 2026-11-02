// =====================================================================
//  MÁY ĐO TỐC ĐỘ MINI
//  Arduino Nano + 2 cảm biến HC-SR04 + LCD1602 I2C + đèn giao thông
//
//  Cách hoạt động:
//    0. Khởi động: ĐO NỀN 2 giây (để trống trước cảm biến) để biết vật đứng yên
//       gần nhất (tường, bàn...). Sau đó chỉ tính vật gần hơn nền 8 cm trở lên.
//    1. Hai cảm biến A, B đặt cách nhau 19.5 cm, đo luân phiên.
//    2. Khi vật đi ngang qua 1 cảm biến, ghi lại lúc vật GẦN NHẤT (t_min, d_min).
//    3. Khi vật đã qua cả A và B:
//       - d_min của A và B lệch nhau > 2 cm  -> "LECH", đèn ĐỎ
//       - ngược lại tính vận tốc = 19.5 cm / (t_min B - t_min A)
//           <= 6 km/h -> đèn XANH,  > 6 km/h -> đèn VÀNG
//    4. Vật chỉ qua 1 cảm biến -> lỗi, tắt đèn.
//
//  Nối dây:
//    Cảm biến A : TRIG -> D2, ECHO -> D3
//    Cảm biến B : TRIG -> D4, ECHO -> D5
//    LCD I2C    : SDA  -> A4, SCL  -> A5
//    Đèn        : Đỏ -> A0, Vàng -> A1, Xanh -> A2
// =====================================================================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "hc-sr04.h"

// ---------------- Các con số cố định ----------------
const float KHOANG_CACH_AB  = 19.5;   // khoảng cách giữa 2 cảm biến (cm)
const float GIOI_HAN_TOC_DO = 6.0;    // km/h
const float LECH_TOI_DA     = 2.0;    // cm
const float XA_NHAT         = 100.0;  // chỉ tính vật gần hơn 100 cm
const float CACH_NEN        = 8.0;    // vật phải gần hơn nền ít nhất 8 cm
const int   NGHI_GIUA_2_LAN = 15;     // ms nghỉ giữa 2 lần đo (tránh A nghe nhầm sóng của B)
const int   SO_LAN_TRONG    = 3;      // không thấy vật 3 lần liên tiếp -> vật đã đi qua
const int   SO_MAU_LOG      = 40;     // số mẫu tối đa ghi lại mỗi lần vật đi qua

const int LED_DO   = A0;
const int LED_VANG = A1;
const int LED_XANH = A2;

LiquidCrystal_I2C lcd(0x27, 16, 2);   // màn hình không hiện chữ -> đổi 0x27 thành 0x3F

// ---------------- Dữ liệu của 1 cảm biến ----------------
struct CamBien {
  char   ten;                 // 'A' hoặc 'B'
  HCSR04 hc;                  // chân TRIG, ECHO
  float  nguong;              // chỉ tính vật gần hơn ngưỡng này (cm), có sau khi đo nền
  bool   dangThayVat;         // đang có vật trước cảm biến
  int    soLanThay;           // số lần thấy vật trong lần đi qua này
  int    soLanTrongLienTiep;  // số lần liên tiếp không thấy vật
  float  dMin;                // khoảng cách nhỏ nhất (cm)
  unsigned long tMin;         // thời điểm khoảng cách nhỏ nhất (ms)
  bool   coKetQua;            // vật đã đi qua xong, chờ xử lý
  unsigned long tKetThuc;     // thời điểm vật đi qua xong
  // Ghi lại các lần đo để in ra Serial
  int    n;
  float  logD[SO_MAU_LOG];
  unsigned long logT[SO_MAU_LOG];
};

CamBien camA, camB;

// ---------------------------------------------------------------------
// Đưa 1 lần đo (thời điểm t, khoảng cách d) vào cảm biến
// ---------------------------------------------------------------------
void capNhat(CamBien &cb, unsigned long t, float d) {
  bool thayVat = (d > 0 && d < cb.nguong);

  // Vật bắt đầu xuất hiện -> làm mới dữ liệu
  if (thayVat && !cb.dangThayVat) {
    cb.dangThayVat = true;
    cb.coKetQua    = false;      // bỏ kết quả cũ chưa xử lý (nếu có)
    cb.soLanThay   = 0;
    cb.dMin        = 999;
    cb.n           = 0;
  }

  if (!cb.dangThayVat) return;   // không có vật -> không làm gì

  // Ghi log (-1 = lần đó không thấy vật)
  if (cb.n < SO_MAU_LOG) {
    cb.logD[cb.n] = thayVat ? d : -1;
    cb.logT[cb.n] = t;
    cb.n++;
  }

  if (thayVat) {
    cb.soLanThay++;
    cb.soLanTrongLienTiep = 0;
    if (d < cb.dMin) {           // gần hơn -> lưu lại
      cb.dMin = d;
      cb.tMin = t;
    }
  } else {
    cb.soLanTrongLienTiep++;
    if (cb.soLanTrongLienTiep >= SO_LAN_TRONG) {   // vật đã đi qua
      cb.dangThayVat = false;
      if (cb.soLanThay >= 2) {                     // chỉ thấy 1 lần -> nhiễu, bỏ
        cb.coKetQua = true;
        cb.tKetThuc = t;
      }
    }
  }
}

// ---------------------------------------------------------------------
// In chuỗi khoảng cách + thời điểm của 1 cảm biến
// ---------------------------------------------------------------------
void inLog(CamBien &cb) {
  Serial.print("["); Serial.print(cb.ten); Serial.println("]");
  Serial.print("  d(cm): ");
  for (int i = 0; i < cb.n; i++) { Serial.print(cb.logD[i], 1); Serial.print("\t"); }
  Serial.println();
  Serial.print("  t(ms): ");
  for (int i = 0; i < cb.n; i++) { Serial.print(cb.logT[i]); Serial.print("\t"); }
  Serial.println();
  Serial.print("  t_min = "); Serial.print(cb.tMin);
  Serial.print(" ms, d_min = "); Serial.print(cb.dMin, 1); Serial.println(" cm");
}

void batDen(bool doSang, bool vangSang, bool xanhSang) {
  digitalWrite(LED_DO,   doSang);
  digitalWrite(LED_VANG, vangSang);
  digitalWrite(LED_XANH, xanhSang);
}

// ---------------------------------------------------------------------
// Vật đã qua cả A và B -> so sánh, tính vận tốc, báo đèn và LCD
// ---------------------------------------------------------------------
void tinhKetQua() {
  Serial.println();
  Serial.println("===== VAT DI QUA =====");
  inLog(camA);
  inLog(camB);

  float lech = abs(camA.dMin - camB.dMin);
  Serial.print("Chenh lech d_min = "); Serial.print(lech, 1); Serial.println(" cm");

  if (lech > LECH_TOI_DA) {
    Serial.println(">>> LECH -> khong tinh van toc");
    batDen(true, false, false);                        // ĐỎ
    lcd.clear();
    lcd.print("DO LECH!");
    lcd.setCursor(0, 1);
    lcd.print("A:"); lcd.print(camA.dMin, 1);
    lcd.print(" B:"); lcd.print(camB.dMin, 1);
    return;
  }

  long deltaT = abs((long)camB.tMin - (long)camA.tMin);  // ms
  Serial.print("delta_t = "); Serial.print(deltaT); Serial.println(" ms");
  if (deltaT == 0) {
    Serial.println(">>> delta_t = 0 -> khong tinh duoc van toc");
    batDen(false, false, false);
    return;
  }

  float vanTocCm = KHOANG_CACH_AB / (deltaT / 1000.0);   // cm/s
  float vanTocKm = vanTocCm * 0.036;                       // km/h
  Serial.print(">>> VAN TOC = "); Serial.print(vanTocCm, 1);
  Serial.print(" cm/s = ");       Serial.print(vanTocKm, 2); Serial.println(" km/h");

  bool quaToc = vanTocKm > GIOI_HAN_TOC_DO;
  if (quaToc) Serial.println(">>> QUA TOC DO");
  batDen(false, quaToc, !quaToc);                        // VÀNG hoặc XANH

  lcd.clear();
  lcd.print("V="); lcd.print(vanTocKm, 2); lcd.print(" km/h");
  lcd.setCursor(0, 1);
  if (quaToc) lcd.print("QUA TOC DO!");
  else      { lcd.print("dt="); lcd.print(deltaT); lcd.print(" ms"); }
}

// ---------------------------------------------------------------------
// Đo nền: đo mỗi cảm biến trong 2 giây, lấy khoảng cách nhỏ nhất (vật đứng yên).
// Ngưỡng = nền - 8 cm (không quá 100 cm).
// ---------------------------------------------------------------------
void doNen(CamBien &cb) {
  float nen = 999;
  unsigned long batDau = millis();
  while (millis() - batDau < 2000) {
    float d = hcsr04_read_cm(&cb.hc);
    if (d > 0 && d < nen) nen = d;
    delay(30);
  }
  cb.nguong = min(XA_NHAT, nen - CACH_NEN);

  Serial.print("Nen "); Serial.print(cb.ten); Serial.print(": ");
  if (nen < 999) { Serial.print(nen, 1); Serial.print(" cm"); }
  else           Serial.print("trong");
  Serial.print(" -> chi tinh vat gan hon "); Serial.print(cb.nguong, 1); Serial.println(" cm");
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  camA.ten = 'A';
  camB.ten = 'B';
  hcsr04_init(&camA.hc, 2, 3, 110);
  hcsr04_init(&camB.hc, 4, 5, 110);

  pinMode(LED_DO, OUTPUT);
  pinMode(LED_VANG, OUTPUT);
  pinMode(LED_XANH, OUTPUT);
  batDen(false, false, false);

  lcd.init();
  lcd.backlight();
  lcd.print("DO NEN...");
  Serial.println("Dang do nen, de trong truoc cam bien...");
  doNen(camA);
  doNen(camB);

  lcd.clear();
  lcd.print("SAN SANG");
  Serial.println("San sang do");
}

void loop() {
  // ---- 1. Đo luân phiên A, B ----
  static bool luotA = true;
  static unsigned long lanDoTruoc = 0;

  if (millis() - lanDoTruoc >= NGHI_GIUA_2_LAN) {
    CamBien &cb = luotA ? camA : camB;
    luotA = !luotA;

    unsigned long t = millis();
    float d = hcsr04_read_cm(&cb.hc);
    lanDoTruoc = millis();

    if (d != HCSR04_BUSY) {      // cảm biến còn bận thì bỏ qua lần này
      capNhat(cb, t, d);
    }
  }

  // ---- 2. Xử lý kết quả khi không còn vật trước cả 2 cảm biến ----
  //         (in Serial và ghi LCD chậm, nên chỉ làm lúc này để không ảnh hưởng việc đo)
  if (camA.dangThayVat || camB.dangThayVat) return;

  if (camA.coKetQua && camB.coKetQua) {
    tinhKetQua();
    camA.coKetQua = false;
    camB.coKetQua = false;
  }
  // Chỉ 1 cảm biến có kết quả, chờ 2 giây mà cảm biến kia không thấy gì -> lỗi
  else if (camA.coKetQua && millis() - camA.tKetThuc > 2000) {
    Serial.println("Loi: vat chi qua cam bien A");
    batDen(false, false, false);
    camA.coKetQua = false;
  }
  else if (camB.coKetQua && millis() - camB.tKetThuc > 2000) {
    Serial.println("Loi: vat chi qua cam bien B");
    batDen(false, false, false);
    camB.coKetQua = false;
  }
}
