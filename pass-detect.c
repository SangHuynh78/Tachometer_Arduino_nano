// =====================================================================
//  pass-detect.c - xem mô tả trong pass-detect.h
// =====================================================================
#include "pass-detect.h"

void pd_init(PassDetector *p, char name, float detectMaxCm, uint8_t endMisses)
{
  p->name        = name;
  p->detectMaxCm = detectMaxCm;
  p->endMisses   = endMisses;
  p->active      = 0;
  p->misses      = 0;
  p->n           = 0;
  p->preHead     = 0;
  p->preCount    = 0;
}

static void push_sample(PassDetector *p, uint32_t tMs, int16_t dmm)
{
  if (p->n < PD_MAX_SAMPLES) {
    p->buf[p->n].dt   = (uint16_t)(tMs - p->tStart);
    p->buf[p->n].d_mm = dmm;
    p->n++;
  } else {
    p->res.truncated = 1;
  }
}

static uint8_t finish(PassDetector *p)
{
  p->active   = 0;
  p->preCount = 0;
  if (p->res.hits < PD_MIN_HITS) return 0;           // nhiễu, bỏ qua

  p->res.tMid = p->res.tIn + (p->res.tOut - p->res.tIn) / 2;
  p->res.tMin = p->tMinFirst + (p->tMinLast - p->tMinFirst) / 2;
  return 1;
}

uint8_t pd_update(PassDetector *p, uint32_t tMs, float cm)
{
  uint8_t detect = (cm >= 2.0f) && (cm < p->detectMaxCm);
  int16_t dmm    = (cm < 0) ? PD_NO_ECHO_MM : (int16_t)(cm * 10.0f + 0.5f);

  if (!p->active) {
    if (!detect) {
      // lưu vào vòng đệm "trước khi có vật"
      p->preT[p->preHead] = tMs;
      p->preD[p->preHead] = dmm;
      p->preHead = (p->preHead + 1) % PD_PRE_SAMPLES;
      if (p->preCount < PD_PRE_SAMPLES) p->preCount++;
      return 0;
    }

    // ---- bắt đầu một lần đi qua ----
    p->active = 1;
    p->misses = 0;
    p->n      = 0;
    p->res.hits = 0;
    p->res.truncated = 0;
    p->res.tIn  = tMs;
    p->res.dMin = 1e9f;

    uint8_t first = (p->preHead + PD_PRE_SAMPLES - p->preCount) % PD_PRE_SAMPLES;
    p->tStart = (p->preCount > 0) ? p->preT[first] : tMs;
    for (uint8_t k = 0; k < p->preCount; k++) {
      uint8_t idx = (first + k) % PD_PRE_SAMPLES;
      push_sample(p, p->preT[idx], p->preD[idx]);
    }
  }

  // ---- đang trong một lần đi qua ----
  push_sample(p, tMs, dmm);

  if (detect) {
    p->misses = 0;
    p->res.hits++;
    p->res.tOut = tMs;

    if (cm < p->res.dMin - PD_MIN_TOL_CM) {          // nhỏ hơn hẳn -> mốc mới
      p->res.dMin  = cm;
      p->tMinFirst = p->tMinLast = tMs;
    } else if (cm <= p->res.dMin + PD_MIN_TOL_CM) {  // xấp xỉ bằng -> kéo dài "đáy"
      if (cm < p->res.dMin) p->res.dMin = cm;
      p->tMinLast = tMs;
    }
  } else {
    p->misses++;
    if (p->misses >= p->endMisses) return finish(p);
  }

  if (tMs - p->res.tIn > PD_MAX_EVENT_MS) return finish(p);
  return 0;
}
