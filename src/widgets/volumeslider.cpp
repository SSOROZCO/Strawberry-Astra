/***************************************************************************
* ASTRA MOD - Roon Volume Dial - Platinum #D1D1D6 / Gold #C9B896
* Mantiene la clase VolumeSlider original pero pinta anillo Roon
***************************************************************************/

#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QEnterEvent>
#include <QStyle>
#include <QPainter>
#include <QtMath>
#include "volumeslider.h"

using namespace Qt::Literals::StringLiterals;

VolumeSlider::VolumeSlider(QWidget *parent, const uint max)
    : SliderSlider(Qt::Horizontal, parent, static_cast<int>(max)),
      wheel_accumulator_(0),
      anim_enter_(false),
      anim_count_(0),
      timer_anim_(new QTimer(this)),
      pixmap_inset_(QPixmap(1,1)) {

  setFocusPolicy(Qt::NoFocus);
  setFixedSize(64, 64);
  previous_theme_text_color_ = palette().color(QPalette::WindowText);
  previous_theme_highlight_color_ = palette().color(QPalette::Highlight);
  QObject::connect(timer_anim_, &QTimer::timeout, this, &VolumeSlider::slotAnimTimer);
}

void VolumeSlider::SetEnabled(const bool enabled) {
  QSlider::setEnabled(enabled);
  QSlider::setVisible(enabled);
}

void VolumeSlider::HandleWheel(const int delta) {
  const int scroll_state = wheel_accumulator_ + delta;
  const int steps = scroll_state / WHEEL_ROTATION_PER_STEP;
  wheel_accumulator_ = scroll_state % WHEEL_ROTATION_PER_STEP;
  if (steps!=0) {
    wheeling_ = true;
    QSlider::setValue(SliderSlider::value() + steps);
    Q_EMIT SliderReleased(value());
    wheeling_ = false;
  }
}

void VolumeSlider::paintEvent(QPaintEvent *e) {
  Q_UNUSED(e)
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);
  QRectF r = rect().adjusted(10, 10, -10, -10);
  QPointF c = r.center();
  double rad = r.width()/2.0;

  QPen base(QColor(0x2A, 0x2B, 0x31), 1.8);
  base.setCapStyle(Qt::RoundCap);
  p.setPen(base);
  p.drawArc(r, 135*16, 270*16);

  QColor accent(0xC9, 0xB8, 0x96); // dorado desaturado
  QPen prog(accent, 1.8);
  prog.setCapStyle(Qt::RoundCap);
  p.setPen(prog);
  double ang = 270.0 * (value() / double(qMax(1, maximum())));
  p.drawArc(r, 135*16, ang*16);

  bool showThumb = anim_count_ > 0 || anim_enter_;
  if (showThumb) {
    double theta = qDegreesToRadians(135 + ang);
    double x = c.x() + rad * qCos(theta);
    double y = c.y() + rad * qSin(theta);
    int sz = anim_enter_? (anim_count_ > 5? 7 : 5) : 4;
    p.setPen(Qt::NoPen);
    p.setBrush(accent);
    p.drawEllipse(QPointF(x, y), sz/2.0, sz/2.0);
  }

  p.setPen(anim_enter_? QColor(0xF5,0xF5,0xF7) : QColor(0x8E,0x8E,0x93));
  QFont f = p.font();
  f.setPointSize(9);
  f.setWeight(QFont::Light);
  p.setFont(f);
  p.drawText(r, Qt::AlignCenter, QString::number(value()) + QStringLiteral("%"));
}

void VolumeSlider::generateGradient() {}
QPixmap VolumeSlider::drawVolumePixmap() const { return QPixmap(1,1); }
void VolumeSlider::drawVolumeSliderHandle() { handle_pixmaps_.clear(); handle_pixmaps_ << QPixmap(1,1); for(int i=1;i<ANIM_MAX;i++) handle_pixmaps_ << QPixmap(1,1); }
void VolumeSlider::slotAnimTimer() {
  if (anim_enter_) { if(++anim_count_ >= ANIM_MAX-1){ anim_count_=ANIM_MAX-1; timer_anim_->stop(); } }
  else { if(--anim_count_ <= 0){ anim_count_=0; timer_anim_->stop(); } }
  update();
}
void VolumeSlider::paletteChange(const QPalette&) {}
void VolumeSlider::enterEvent(QEnterEvent *e){ Q_UNUSED(e) anim_enter_=true; timer_anim_->start(ANIM_INTERVAL); }
void VolumeSlider::leaveEvent(QEvent *e){ Q_UNUSED(e) anim_enter_=false; timer_anim_->start(ANIM_INTERVAL); }
void VolumeSlider::contextMenuEvent(QContextMenuEvent *e){
  QMenu menu;
  const QString sheet = u"QMenu { background-color: #1C1C1E; border: 1px solid #2A2B31; border-radius: 8px; padding: 6px 0px; } QMenu::item { padding: 8px 28px 8px 28px; font-size: 11px; font-weight: 300; color: #8E8E93; letter-spacing: 0.5px; background: transparent; } QMenu::item:selected { background-color: #2C2C2E; color: #F5F5F7; border-radius: 4px; margin: 0px 6px; } QMenu::separator { height: 1px; background: #2A2B31; margin: 6px 12px; }"_s;
  menu.setStyleSheet(sheet);
  menu.setWindowFlags(menu.windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
  menu.setAttribute(Qt::WA_TranslucentBackground);

  QHash<QAction*, int> values;
  menu.setTitle(u"Volume"_s);
  values[menu.addAction(u"100%"_s)] = 100;
  values[menu.addAction(u"80%"_s)] = 80;
  values[menu.addAction(u"60%"_s)] = 60;
  values[menu.addAction(u"40%"_s)] = 40;
  values[menu.addAction(u"20%"_s)] = 20;
  values[menu.addAction(u"0%"_s)] = 0;

  QAction *ret = menu.exec(mapToGlobal(e->pos()));
  if (ret) { QSlider::setValue(values[ret]); Q_EMIT SliderReleased(values[ret]); }
}
void VolumeSlider::slideEvent(QMouseEvent *e){
  QPointF c = QRectF(rect().adjusted(10,10,-10,-10)).center();
  double dx = e->pos().x() - c.x();
  double dy = e->pos().y() - c.y();
  double a = qRadiansToDegrees(qAtan2(dy, dx)) - 135.0;
  if (a < 0) a += 360;
  if (a > 270) a = a > 315 ? 0 : 270;
  QSlider::setValue(int((a / 270.0) * maximum()));
}
void VolumeSlider::mousePressEvent(QMouseEvent *e){ if (e->button()!= Qt::RightButton) { SliderSlider::mousePressEvent(e); slideEvent(e); } }
void VolumeSlider::wheelEvent(QWheelEvent *e){
  int step = e->angleDelta().y() > 0 ? 2 : -2;
  QSlider::setValue(qBound(minimum(), value() + step, maximum()));
  Q_EMIT SliderReleased(value());
}
