# Hệ thống đo tốc độ mini (Arduino Nano)

## 1. Linh kiện
| Linh kiện | SL | Ghi chú |
|---|---|---|
| Arduino Nano (ATmega328P) | 1 | Chọn đúng Processor "ATmega328P (Old Bootloader)" nếu nạp lỗi |
| HC-SR04 | 2 | Cảm biến A (vào), B (ra) |
| LCD1602 + module I2C (PCF8574) | 1 | Chỉ 4 dây: GND, VCC, SDA, SCL |
| Module đèn giao thông 4 chân | 1 | R, Y, G, GND (thường đã có điện trở) |

## 2. Sơ đồ chân

| Thiết bị | Chân thiết bị | Chân Nano |
|---|---|---|
| HC-SR04 A | VCC / GND | 5V / GND |
|  | TRIG | **D2** |
|  | ECHO | **D3** |
| HC-SR04 B | VCC / GND | 5V / GND |
|  | TRIG | **D4** |
|  | ECHO | **D5** |
| LCD1602 I2C | GND | GND |
|  | VCC | 5V |
|  | SDA | **A4** |
|  | SCL | **A5** |
| Đèn giao thông | R / Y / G | **A0 / A1 / A2** |
|  | GND | GND |
| Dự phòng | Còi chip | D6 (`USE_BUZZER 1`) |
| Dự phòng | Nút bấm | A3 |

Chân còn trống: D7–D13, A3 (D6 để dành cho còi), A6, A7 (A6/A7 chỉ đọc analog). D0/D1 dành cho Serial.

### Lưu ý về LCD I2C
- **A4/A5 là chân I2C cố định** của Nano, không đổi sang chân khác được. (Một số Nano clone có thêm chân SDA/SCL riêng, nhưng thực chất vẫn nối chung với A4/A5.)
- **Thư viện:** trong Arduino IDE vào *Library Manager* → tìm **"LiquidCrystal I2C"** → cài bản của **Frank de Brabander**.
- **Địa chỉ:** code tự dò địa chỉ khi khởi động (0x27, 0x3F, rồi cả dải 0x20–0x3F) và in kết quả ra Serial Monitor. Muốn đặt cố định thì sửa `LCD_I2C_ADDR` trong `config.h`.
- **Màn hình sáng nhưng không có chữ / toàn ô vuông:** vặn **biến trở xanh** phía sau module I2C để chỉnh độ tương phản.
- **Không thấy LCD:** đèn vàng nháy 3 lần lúc khởi động và Serial báo lỗi; hệ thống vẫn đo và in kết quả qua Serial.
- Jumper "LED" trên module dùng để bật/tắt đèn nền; cần cắm jumper này.

## 3. Lắp đặt cơ khí
```
          ←──── SENSOR_SPACING_CM (vd 20 cm) ────→
   [HC-SR04 A]                              [HC-SR04 B]
       ║  tia chiếu ngang qua làn                ║
  ═════╬═════════════════════════════════════════╬═════  làn xe
       ║            🚗 →                         ║
  ─────────────────── vách chắn đối diện ──────────────
```
- Hai cảm biến **song song**, cùng chiếu vuông góc với làn, cùng độ cao thân xe.
- Đo khoảng cách **tâm-tâm** 2 cảm biến thật chính xác → nhập vào `SENSOR_SPACING_CM`.
- Đặt vách chắn phẳng phía đối diện (cách 15–40 cm) để nền ổn định.
- Khi khởi động, để làn **trống** ~2 giây cho hệ thống tự hiệu chuẩn.

## 4. Cách hoạt động
1. Khởi động: test đèn → hiệu chuẩn nền → **XANH** (sẵn sàng).
2. Xe cắt tia cảm biến thứ nhất → **VÀNG**, ghi t1.
3. Xe cắt tia cảm biến thứ hai → ghi t2, v = khoảng cách / (t2 − t1).
4. LCD hiện tốc độ, hướng, thời gian; vượt `SPEED_LIMIT_KMH` → **ĐỎ nhấp nháy**.
5. Sau `RESULT_HOLD_MS` và khi làn trống → trở về sẵn sàng.

Serial Monitor (115200): in CSV `count,dir,dt_ms,cm_s,kmh,over`; lệnh `c` hiệu chuẩn lại, `r` reset, `s` xem trạng thái cảm biến (dùng để chỉnh ngưỡng).

## 5. Giới hạn độ chính xác
- Mỗi cảm biến được đọc mỗi ~20 ms (phát luân phiên để chống nhiễu chéo) ⇒ sai số thời gian ~±20 ms.
- Với khoảng cách 20 cm: xe 1 m/s (dt = 200 ms) sai số ~±10%; xe càng nhanh sai số càng lớn.
- Tăng `SENSOR_SPACING_CM` (30–50 cm) là cách rẻ nhất để chính xác hơn.

## 6. Hướng phát triển
**Giai đoạn 1 – Hoàn thiện cơ bản (bản hiện tại)**
- [x] Đo 2 chiều, chống dội, timeout, tự hiệu chuẩn nền
- [x] Đèn trạng thái, cảnh báo vượt tốc, xuất CSV qua Serial
- [ ] Kiểm chứng: thả xe/vật trên dốc cố định, so với quay video 240fps

**Giai đoạn 2 – Tiện dụng**
- Nút bấm A3 (hoặc D7–D9): đổi đơn vị (km/h ↔ cm/s ↔ m/s), xem Max, đặt ngưỡng
- Lưu ngưỡng tốc độ, khoảng cách, tỉ lệ mô hình vào **EEPROM**
- Còi báo vượt tốc (D6), đếm lượt và tốc độ trung bình

**Giai đoạn 3 – Chính xác hơn**
- Thay HC-SR04 bằng **cảm biến hồng ngoại cắt tia / laser + quang trở** dùng ngắt ngoài D2/D3 → sai số còn vài µs
- Hoặc dùng thư viện NewPing (timer) để đo không chặn, tăng tần số lấy mẫu
- Bù nhiệt độ tốc độ âm thanh (thêm DHT11/DS18B20) nếu dùng siêu âm để đo khoảng cách

**Giai đoạn 4 – Kết nối**
- Ghi log thẻ microSD (SPI: D10–D13, đang trống nhờ LCD dùng I2C)
- Thay Nano bằng ESP32 / thêm ESP-01 → web dashboard, gửi dữ liệu lên Google Sheets
- Camera ESP32-CAM chụp ảnh xe vượt tốc ("camera phạt nguội" mini)
