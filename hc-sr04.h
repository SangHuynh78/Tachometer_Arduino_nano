// =====================================================================
//  hc-sr04.h - Thư viện đơn giản cho cảm biến siêu âm HC-SR04
//  Viết bằng C, dùng được từ file .ino (C++) nhờ khối extern "C"
// =====================================================================
#ifndef HC_SR04_H
#define HC_SR04_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Giá trị trả về khi không nhận được tiếng vọng (ngoài tầm / lỗi dây)
#define HCSR04_NO_ECHO   (-1.0f)
// Giá trị trả về khi cảm biến CHƯA SẴN SÀNG (còn bận lần đo trước):
// KHÔNG phải "không có vật" -> nên bỏ qua lần đo này, thử lại sau
#define HCSR04_BUSY      (-2.0f)

// Thời gian chờ tối đa để ECHO bắt đầu lên HIGH sau xung kích (µs)
#define HCSR04_START_TIMEOUT_US  5000UL

// Thông tin một cảm biến
typedef struct {
  uint8_t  trigPin;    // chân TRIG
  uint8_t  echoPin;    // chân ECHO
  uint16_t maxCm;      // tầm đo tối đa (cm), dùng để giới hạn thời gian chờ
} HCSR04;

// Khởi tạo cảm biến: gán chân, cấu hình pinMode
void  hcsr04_init(HCSR04 *s, uint8_t trigPin, uint8_t echoPin, uint16_t maxCm);

// Đo 1 lần, trả về khoảng cách (cm), HCSR04_NO_ECHO hoặc HCSR04_BUSY
// Lưu ý: hàm chờ tối đa ~ 5 ms + maxCm * 58 µs
float hcsr04_read_cm(HCSR04 *s);

#ifdef __cplusplus
}
#endif

#endif // HC_SR04_H
