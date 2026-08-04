#pragma once
#include "analyzerbase.h"

class DorroughAnalyzer : public AnalyzerBase {
  Q_OBJECT
public:
  static const char *kName;
  Q_INVOKABLE explicit DorroughAnalyzer(QWidget *parent);
  void analyze(QPainter &p, const Scope &scope, bool new_frame) override;
  void transform(Scope &s) override;
private:
  float m_persist = -60.0f; // dB
  float m_peak = -60.0f;
  float m_peakHold = -60.0f;
  int m_peakHoldCounter = 0;
};
