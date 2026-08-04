/*
   СИСТЕМА ТЕЛЕМЕТРИИ СПЕКТРА
   Центр управления полётами - Байконур
   РОСКОСМОС / АСТРА ЛИНУКС / РУСБИТЕХ - 1961-2025
 */

#include <QPainter>
#include <QResizeEvent>
#include <QtMath>
#include <QDateTime>
#include <QRandomGenerator>
#include "engine/enginebase.h"
#include "sonogramanalyzer.h"
#include "fht.h"

const char *SonogramAnalyzer::kName = QT_TRANSLATE_NOOP("AnalyzerContainer", "Roscosmos");

SonogramAnalyzer::SonogramAnalyzer(QWidget *parent) : AnalyzerBase(parent, 9) {
  m_persist=-60; m_peak=-60; m_hold=-60; m_holdC=0; m_sil=0; m_flicker=0;
}

void SonogramAnalyzer::resizeEvent(QResizeEvent *e) {
  Q_UNUSED(e)
  canvas_ = QPixmap(size());
  canvas_.fill(QColor(5,18,8));
}

void SonogramAnalyzer::transform(Scope &scope) {
  fht_->spectrum(scope.data());
  fht_->scale(scope.data(), 1.0f/2.0f);
}

void SonogramAnalyzer::analyze(QPainter &p, const Scope &s, const bool new_frame) {
  Q_UNUSED(new_frame)
  QRect r = rect();

  // Fondo exterior
  p.fillRect(r, QColor(32,32,30));

  QRect inner = r.adjusted(2,2,-2,-2);

  // Persistencia real
  if(canvas_.size()!= r.size()){
    canvas_ = QPixmap(r.size());
    canvas_.fill(QColor(5,18,8));
  }
  {
    QPainter cp(&canvas_);
    cp.setCompositionMode(QPainter::CompositionMode_SourceOver);
    cp.fillRect(inner, QColor(5,18,8,45));
  }

  QPainter cp(&canvas_);

  // Grid
  cp.setPen(QColor(0,85,0,50));
  for(int x = inner.left(); x < inner.right(); x += 18)
    cp.drawLine(x, inner.top(), x, inner.bottom());
  for(int y = inner.top(); y < inner.bottom(); y += 14)
    cp.drawLine(inner.left(), y, inner.right(), y);

  // Análisis
  float sum=0,pk=0;
  for(float v:s){ sum+=v*v; pk=qMax(pk,v); }
  float rms = qSqrt(sum / qMax(1,int(s.size())));
  float rms_db = 20*log10f(qMax(rms,1e-5f))+42.0f;
  float peak_db = 20*log10f(qMax(pk,1e-5f))+42.0f;

  if(sum < 1e-7f){ m_sil++; if(m_sil>10){ m_persist=-60; m_peak=-60; } }
  else m_sil=0;
  if(rms_db > m_persist) m_persist = m_persist*0.2f + rms_db*0.8f;
  else m_persist = m_persist*0.88f + rms_db*0.12f - 0.15f;

  auto db2led = [](float db){ return qBound(0,int((db+25)*(40.0f/39.0f)),39); };
  int per = db2led(m_persist);

  const int N=48; // más barras para que se vea mejor en ancho
  float W = float(inner.width()-6);
  float gap=1.5f;
  float ledW = (W - gap*(N-1))/N;
  float ledH = float(inner.height())*0.72f;
  float y0 = inner.y() + 12;

  m_flicker = (m_flicker+1)%4;
  float flick = 1.0f + (QRandomGenerator::global()->bounded(8)-4)/100.0f;
  if(m_flicker==0) flick = 0.92f;

  for(int i=0;i<N;i++){
    float norm = float(i)/float(N-1);
    int idx = int(norm * s.size()*0.65f);
    float v = (idx < int(s.size()))? s[idx] : 0;
    float h = qBound(0.0f, v*ledH*3.2f*flick, ledH);
    if(h < 1.0f) continue;
    float x = inner.x()+3 + i*(ledW+gap);
    QRectF rc(x, y0+ledH-h, ledW, h);
    cp.setPen(Qt::NoPen);
    cp.setBrush(QColor(80,255,120,32));
    cp.drawRect(QRectF(x-0.5f, y0+ledH-h-1, ledW+1, h+2));
    cp.setBrush(QColor(85,255,125));
    cp.drawRect(rc);
  }

  // Ruido
  cp.setPen(Qt::NoPen);
  for(int k=0;k<18;k++){
    int rx = QRandomGenerator::global()->bounded(inner.width()) + inner.x();
    int ry = QRandomGenerator::global()->bounded(inner.height()) + inner.y();
    cp.setBrush(QColor(85,255,125,QRandomGenerator::global()->bounded(50)+10));
    cp.drawRect(rx, ry, 1, 1);
  }
  cp.end();

  p.drawPixmap(0,0,canvas_);

  // Scanlines
  p.setPen(QColor(0,0,0,40));
  for(int y = inner.top(); y < inner.bottom(); y += 3)
    p.drawLine(inner.left(), y, inner.right(), y);

  // === TEXTO SIN SUPERPOSICIÓN ===
  bool isTall = r.height() > 90;

  QFont mono6(QStringLiteral("DejaVu Sans Mono"), 6);
  mono6.setBold(true);
  QFont mono5(QStringLiteral("DejaVu Sans Mono"), 5);
  mono5.setBold(false);

  // Top bar - solo 1 línea
  p.setFont(mono6);
  p.setPen(QColor(85,255,125));
  p.drawText(inner.x()+3, inner.y()+1, inner.width()/2, 9, Qt::AlignLeft|Qt::AlignVCenter, QStringLiteral("ЦУП - БАЙКОНУР"));

  // MET arriba derecha
  static QDateTime start = QDateTime::currentDateTime();
  qint64 elapsed = start.secsTo(QDateTime::currentDateTime());
  int hh = int(elapsed/3600)%99;
  int mm = int(elapsed/60)%60;
  int ss = int(elapsed%60);
  QString met = QStringLiteral("МЕТ %1:%2:%3").arg(hh,2,10,QLatin1Char('0')).arg(mm,2,10,QLatin1Char('0')).arg(ss,2,10,QLatin1Char('0'));
  p.setFont(mono5);
  p.setPen(QColor(85,255,125,200));
  p.drawText(inner.x()+inner.width()/2, inner.y()+1, inner.width()/2 -3, 9, Qt::AlignRight|Qt::AlignVCenter, met);

  // Si es alto, mostrá más datos abajo, si no, solo indicadores
  if(isTall){
    p.setFont(mono5);
    p.setPen(QColor(85,255,125,150));
    p.drawText(inner.x()+3, inner.bottom()-10, 100, 8, Qt::AlignLeft|Qt::AlignVCenter, QStringLiteral("ТЕЛЕМЕТРИЯ СПЕКТРА"));
    p.drawText(inner.right()-72, inner.bottom()-10, 70, 8, Qt::AlignRight|Qt::AlignVCenter, QStringLiteral("N45°37' E63°18'"));
  }

  // Indicadores - siempre abajo con espacio
  bool hasSignal = per > 2;
  bool hasSync = per > 5;
  bool hasOrbit = m_sil < 5;

  auto drawDot = [&](int x, int y, bool on, const QString &txt){
    p.setFont(mono5);
    p.setPen(QColor(85,255,125, on?230:70));
    p.drawText(x, y, 32, 7, Qt::AlignLeft|Qt::AlignVCenter, txt);
    p.setBrush(QColor(85,255,125, on?255:30));
    p.setPen(Qt::NoPen);
    p.drawEllipse(x+33, y+1, 4,4);
    if(on){
      p.setBrush(QColor(85,255,125,50));
      p.drawEllipse(x+31, y-1, 8,8);
    }
  };

  int by = inner.bottom()- (isTall? 0 : 0);
  // en modo bajo, los pongo en la misma línea inferior pero separados
  if(!isTall){
    drawDot(inner.x()+3, inner.y()+inner.height()-7, hasOrbit, QStringLiteral("ОРБИТА"));
    drawDot(inner.x()+50, inner.y()+inner.height()-7, hasSignal, QStringLiteral("СИГНАЛ"));
    drawDot(inner.x()+97, inner.y()+inner.height()-7, hasSync, QStringLiteral("СИНХРОН"));
  } else {
    drawDot(inner.x()+3, inner.bottom()-20, hasOrbit, QStringLiteral("ОРБИТА"));
    drawDot(inner.x()+3, inner.bottom()-12, hasSignal, QStringLiteral("СИГНАЛ"));
    drawDot(inner.x()+3, inner.bottom()-4, hasSync, QStringLiteral("СИНХРОН"));
  }
}

void SonogramAnalyzer::demo(QPainter &p) {
  analyze(p, Scope(static_cast<size_t>(fht_->size()), 0), new_frame_);
}
