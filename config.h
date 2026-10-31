// =====================================================================
//  config.h - Cấu hình chân và thông số cho hệ thống đo tốc độ mini
//  Board: Arduino Nano (ATmega328P)
// =====================================================================
#pragma once

// ---------------------------------------------------------------------
// 1. SƠ ĐỒ CHÂN
// ---------------------------------------------------------------------
// Cảm biến siêu âm HC-SR04 #1 (cổng VÀO - vật đi qua trước)
#define PIN_TRIG1        2
#define PIN_ECHO1        3
// Cảm biến siêu âm HC-SR04 #2 (cổng RA - vật đi qua sau)
#define PIN_TRIG2        4
#define PIN_ECHO2        5

// LCD1602 + module I2C (PCF8574)
//   SDA -> A4, SCL -> A5 (chân I2C phần cứng của Nano, không đổi được)
//   Địa chỉ: tự dò khi khởi động (thử 0x27 rồi 0x3F). Có thể ép cố định
//   bằng cách đặt LCD_I2C_ADDR khác 0, vd 0x27.
#define LCD_I2C_ADDR     0         // 0 = tự dò
#define LCD_COLS         16
#define LCD_ROWS         2

// Module đèn giao thông 4 chân (R, Y, G, GND)
#define PIN_LED_RED      A0
#define PIN_LED_YELLOW   A1
#define PIN_LED_GREEN    A2
#define LED_ACTIVE_HIGH  1         // 1 = module chung GND (phổ biến), 0 = chung VCC

// (Dự phòng cho phát triển: còi D6, nút bấm A3; D7-D13 đang trống)
#define PIN_BUZZER       6
#define USE_BUZZER       0         // đặt 1 nếu gắn còi chip (active buzzer) vào D6

// ---------------------------------------------------------------------
// 2. THÔNG SỐ ĐO
// ---------------------------------------------------------------------
#define SENSOR_SPACING_CM   20.0   // KHOẢNG CÁCH giữa tâm 2 cảm biến (đo thật kỹ!)
#define SPEED_LIMIT_KMH     3.0    // ngưỡng vượt tốc độ (theo đơn vị hiển thị)
#define SCALE_FACTOR        1.0    // xe mô hình 1:64 -> đặt 64 để quy ra tốc độ "thật"

#define MAX_RANGE_CM        100    // tầm đo tối đa (giới hạn thời gian chờ echo)
#define DETECT_MARGIN_CM    5.0    // vật phải gần hơn (nền - margin) mới tính là có vật
#define DETECT_MIN_CM       2.0    // dưới giá trị này coi là nhiễu
#define DEBOUNCE_HITS       2      // số lần đọc liên tiếp để xác nhận có/mất vật
#define CALIB_SAMPLES       20     // số mẫu hiệu chuẩn nền khi khởi động

#define PING_INTERVAL_MS    10     // nghỉ giữa 2 lần phát (chống nhiễu chéo 2 cảm biến)
#define MEASURE_TIMEOUT_MS  3000   // quá thời gian này mà chưa qua cảm biến 2 -> hủy
#define RESULT_HOLD_MS      3000   // thời gian giữ kết quả trên LCD
#define MIN_DT_MS           5      // dt nhỏ hơn -> coi là nhiễu (tốc độ phi lý)

#define SERIAL_BAUD         115200
