#ifndef SONOGRAMANALYZER_H
#define SONOGRAMANALYZER_H
#include <QPixmap>
#include <QPainter>
#include "analyzerbase.h"
class SonogramAnalyzer : public AnalyzerBase {
  Q_OBJECT
 public:
  Q_INVOKABLE explicit SonogramAnalyzer(QWidget *parent);
  static const char *kName;
 protected:
  void resizeEvent(QResizeEvent *e) override;
  void analyze(QPainter &p, const Scope &s, const bool new_frame) override;
  void transform(Scope &scope) override;
  void demo(QPainter &p) override;
 private:
  QPixmap canvas_;
  float m_persist = -60.0f;
  float m_peak = -60.0f;
  float m_hold = -60.0f;
  int m_holdC = 0;
  int m_sil = 0;
  int m_flicker = 0;
};
#endif
