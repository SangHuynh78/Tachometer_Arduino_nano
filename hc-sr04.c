// =====================================================================
//  hc-sr04.c - Thư viện đơn giản cho cảm biến siêu âm HC-SR04
//
//  Nguyên lý: kéo TRIG lên HIGH 10 µs -> cảm biến phát 8 xung 40 kHz,
//  chân ECHO lên HIGH trong khoảng thời gian sóng đi và về.
//  Khoảng cách (cm) = thời gian (µs) / 58   (âm thanh ~343 m/s, chia 2 chiều)
// =====================================================================
#include <Arduino.h>
#include "hc-sr04.h"

void hcsr04_init(HCSR04 *s, uint8_t trigPin, uint8_t echoPin, uint16_t maxCm)
{
  s->trigPin = trigPin;
  s->echoPin = echoPin;
  s->maxCm   = maxCm;

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
}

float hcsr04_read_cm(HCSR04 *s)
{
  // Một số HC-SR04 giữ ECHO ở HIGH khá lâu khi lần trước không có vọng
  // -> chưa sẵn sàng, bỏ qua lần đo này
  if (digitalRead(s->echoPin) == HIGH) {
    return HCSR04_NO_ECHO;
  }

  // Phát xung kích 10 µs
  digitalWrite(s->trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(s->trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(s->trigPin, LOW);

  // 1) Chờ ECHO lên HIGH. Tùy loại module, việc này mất 0,5 - 2,5 ms
  //    sau xung kích -> chờ riêng, không tính vào tầm đo.
  unsigned long t0 = micros();
  while (digitalRead(s->echoPin) == LOW) {
    if (micros() - t0 > HCSR04_START_TIMEOUT_US) {
      return HCSR04_NO_ECHO;        // cảm biến không phản hồi (kiểm tra dây)
    }
  }

  // 2) Đo độ rộng xung ECHO, dừng sớm nếu vượt tầm đo maxCm
  unsigned long start = micros();
  unsigned long limit = (unsigned long)s->maxCm * 58UL;
  while (digitalRead(s->echoPin) == HIGH) {
    if (micros() - start > limit) {
      return HCSR04_NO_ECHO;        // vật xa hơn tầm đo / không có vọng
    }
  }
  return (micros() - start) / 58.0f;
}
