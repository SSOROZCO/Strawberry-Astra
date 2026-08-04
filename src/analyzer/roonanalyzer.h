#pragma once
#include "analyzerbase.h"

class RoonAnalyzer : public AnalyzerBase {
  Q_OBJECT
public:
  static const char *kName;
  Q_INVOKABLE explicit RoonAnalyzer(QWidget *parent);
  void analyze(QPainter &p, const Scope &scope, bool new_frame) override;
  void transform(Scope &s) override;

protected:
  void paletteChange(const QPalette &p) { Q_UNUSED(p); }

private:
  int m_bands = 28;
  QVector<float> m_heights;
  QVector<float> m_peaks;
};
