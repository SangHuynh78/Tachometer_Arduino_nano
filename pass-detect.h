// =====================================================================
//  pass-detect.h - Phát hiện vật đi ngang qua 1 cảm biến khoảng cách
//                  và xác định thời điểm vật đi qua TÂM cảm biến
//
//  Mỗi lần đo, gọi pd_update(&p, thời_điểm_ms, khoảng_cách_cm).
//  Khi vật đã đi qua hết, pd_update trả về 1 và p.res chứa kết quả:
//    tIn   : lần đầu thấy vật          tOut : lần cuối thấy vật
//    tMid  : (tIn + tOut) / 2          -> tâm theo "giữa vùng thấy vật"
//    tMin  : lúc khoảng cách nhỏ nhất  -> tâm theo "điểm gần nhất"
//            (nếu có nhiều mẫu cùng nhỏ nhất, lấy giữa của chúng)
//  Mảng buf[] lưu lại chuỗi mẫu (kể cả vài mẫu -1 trước/sau) để in ra xem.
// =====================================================================
#ifndef PASS_DETECT_H
#define PASS_DETECT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PD_MAX_SAMPLES   48     // số mẫu tối đa lưu lại (~2 s); giữ nhỏ để đủ RAM khi bật LCD
#define PD_PRE_SAMPLES   4      // số mẫu "không có vật" giữ lại trước khi vật vào
#define PD_MIN_HITS      2      // ít hơn số mẫu có vật này -> coi là nhiễu
#define PD_MAX_EVENT_MS  5000   // vật đứng yên quá lâu -> kết thúc cưỡng bức
#define PD_MIN_TOL_CM    0.0f   // 0 = t_min đúng là thời điểm của mẫu nhỏ nhất
#define PD_NO_ECHO_MM    (-10)  // mã lưu cho mẫu -1 (không có vật)

typedef struct {
  uint16_t dt;      // ms tính từ tStart
  int16_t  d_mm;    // khoảng cách x10 (mm), PD_NO_ECHO_MM nếu không có vật
} PdSample;

typedef struct {
  uint32_t tIn, tOut, tMid, tMin;   // ms (theo millis())
  float    dMin;                    // cm
  uint16_t hits;                    // số mẫu có vật
  uint8_t  truncated;               // 1 = quá PD_MAX_SAMPLES, buf bị cắt bớt
} PdResult;

typedef struct {
  // cấu hình
  char     name;
  float    detectMaxCm;   // chỉ tính là "có vật" khi gần hơn giá trị này
  uint8_t  endMisses;     // số mẫu trống liên tiếp để kết luận vật đã đi qua

  // chuỗi mẫu của lần đi qua gần nhất
  uint32_t tStart;
  PdSample buf[PD_MAX_SAMPLES];
  uint8_t  n;

  // kết quả lần đi qua gần nhất
  PdResult res;

  // trạng thái nội bộ
  uint8_t  active, misses;
  uint32_t preT[PD_PRE_SAMPLES];
  int16_t  preD[PD_PRE_SAMPLES];
  uint8_t  preHead, preCount;
  uint32_t tMinFirst, tMinLast;
} PassDetector;

void    pd_init(PassDetector *p, char name, float detectMaxCm, uint8_t endMisses);

// Đưa 1 mẫu đo vào. cm < 0 nghĩa là không có tiếng vọng.
// Trả về 1 đúng lúc một lần đi qua vừa kết thúc (kết quả trong p->res).
uint8_t pd_update(PassDetector *p, uint32_t tMs, float cm);

#ifdef __cplusplus
}
#endif

#endif // PASS_DETECT_H
