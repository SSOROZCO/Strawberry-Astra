#include "roonanalyzer.h"
#include <QPainter>
#include <QColor>
#include <QtMath>
#include "fht.h"

const char *RoonAnalyzer::kName = QT_TRANSLATE_NOOP("AnalyzerContainer", "Roon");

RoonAnalyzer::RoonAnalyzer(QWidget *parent) : AnalyzerBase(parent) {
  m_bands = 28;
  resizeForBands(m_bands);
  m_heights.fill(0.0f, m_bands);
  m_peaks.fill(0.0f, m_bands);
  setMinimumWidth(300);
}

void RoonAnalyzer::transform(Scope &s) {
  for (uint x = 0; x < s.size(); ++x) s[x] *= 1.8f;
  fht_->spectrum(s.data());
  fht_->scale(s.data(), 1.0f / 25.0f);
  s.resize(m_bands);
}

void RoonAnalyzer::analyze(QPainter &p, const Scope &scope, bool new_frame) {
  Q_UNUSED(new_frame)
  Scope s(m_bands);
  interpolate(scope, s);

  QRect r = rect();
  p.fillRect(r, QColor(0x12, 0x13, 0x16)); // #121316 Roon base

  const float gap = r.width() / float(m_bands * 1.65f);
  const float bar_w = gap * 0.65f;
  const float radius = 2.5f;

  for (int i = 0; i < m_bands; ++i) {
    float v = qBound(0.0f, s[i] * 2.4f, 1.0f);
    v = qPow(v, 0.58f); // log, no linear

    m_heights[i] = m_heights[i] * 0.80f + v * 0.20f;

    float h = m_heights[i] * r.height() * 0.78f;
    float x = r.x() + i * (bar_w + gap) + gap * 0.8f;
    float y = r.bottom() - h - 4;

    p.setPen(Qt::NoPen);
    // base fantasma
    p.setBrush(QColor(0x2A, 0x2B, 0x31, 55));
    p.drawRoundedRect(QRectF(x, r.bottom()-3, bar_w, 3), 1, 1);

    if (h > 2.0f) {
      p.setBrush(QColor(0xC9, 0xB8, 0x96)); // dorado desaturado, como tu volumen
      p.drawRoundedRect(QRectF(x, y, bar_w, h), radius, radius);
    }
  }
}
