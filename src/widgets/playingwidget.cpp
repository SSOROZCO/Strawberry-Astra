/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 * Copyright 2013-2021, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>

#include <QRandomGenerator>
#include <QtGlobal>
#include <QWidget>
#include <QList>
#include <QByteArray>
#include <QString>
#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QMenu>
#include <QMovie>
#include <QPainter>
#include <QPainterPath>
#include <QTextDocument>
#include <QTimeLine>
#include <QAction>
#include <QActionGroup>
#include <QSettings>
#include <QtEvents>

#include "core/settings.h"
#include "utilities/imageutils.h"
#include "covermanager/albumcoverchoicecontroller.h"
#include "playingwidget.h"

using namespace Qt::Literals::StringLiterals;
using std::make_unique;

namespace {

constexpr char kSettingsGroup[] = "PlayingWidget";
constexpr char kMode[] = "mode";
constexpr char kFitCoverWidth[] = "fit_cover_width";
constexpr char kAboveStatusBar[] = "above_status_bar";

constexpr int kPadding = 12;
constexpr int kMaxCoverSize = 320; // más grande, estilo Roon
constexpr int kBottomOffset = 20;
constexpr int kTopBorder = 8;

}  // namespace

PlayingWidget::PlayingWidget(QWidget *parent)
    : QWidget(parent),
      album_cover_choice_controller_(nullptr),
      mode_(Mode::LargeSongDetails),
      menu_(new QMenu(this)),
      above_statusbar_action_(nullptr),
      fit_cover_width_action_(nullptr),
      enabled_(false),
      visible_(false),
      playing_(false),
      active_(false),
      small_ideal_height_(0),
      total_height_(0),
      desired_height_(0),
      fit_width_(false),
      timeline_show_hide_(new QTimeLine(500, this)),
      timeline_fade_(new QTimeLine(1000, this)),
      details_(new QTextDocument(this)),
      pixmap_previous_track_opacity_(0.0),
      downloading_covers_(false) {

  SetHeight(0);
  Settings s;
  s.beginGroup(kSettingsGroup);
  mode_ = static_cast<Mode>(s.value(kMode, static_cast<int>(Mode::LargeSongDetails)).toInt());
  fit_width_ = s.value(kFitCoverWidth, true).toBool(); // siempre fit
  s.endGroup();

  setAcceptDrops(true);

  QActionGroup *mode_group = new QActionGroup(this);
  CreateModeAction(Mode::SmallSongDetails, tr("Small album cover"), mode_group);
  CreateModeAction(Mode::LargeSongDetails, tr("Large album cover"), mode_group);
  menu_->addActions(mode_group->actions());

  fit_cover_width_action_ = menu_->addAction(tr("Fit cover to width"));
  fit_cover_width_action_->setCheckable(true);
  fit_cover_width_action_->setEnabled(true);
  QObject::connect(fit_cover_width_action_, &QAction::toggled, this, &PlayingWidget::FitCoverWidth);
  fit_cover_width_action_->setChecked(fit_width_);
  menu_->addSeparator();

  QObject::connect(timeline_show_hide_, &QTimeLine::frameChanged, this, &PlayingWidget::SetHeight);
  QObject::connect(timeline_fade_, &QTimeLine::valueChanged, this, &PlayingWidget::FadePreviousTrack);
  timeline_fade_->setDirection(QTimeLine::Backward);

  details_->setUndoRedoEnabled(false);
  if (mode_ == Mode::LargeSongDetails) {
    details_->setDefaultStyleSheet(u"p { font-family: 'JetBrains Mono'; font-size: 11px; color: #8A8C94; } b { color: #E7E8EC; font-size: 14px; letter-spacing: 0.5px; }"_s);
    details_->setHtml(u"<p align=left><b></b><br/><br/></p>"_s);
  }
  UpdateHeight();
}

void PlayingWidget::Init(AlbumCoverChoiceController *album_cover_choice_controller) {
  album_cover_choice_controller_ = album_cover_choice_controller;
  QList<QAction*> cover_actions = album_cover_choice_controller_->GetAllActions();
  menu_->addActions(cover_actions);
  menu_->addSeparator();
  menu_->addAction(album_cover_choice_controller_->search_cover_auto_action());
  menu_->addSeparator();
  above_statusbar_action_ = menu_->addAction(tr("Show above status bar"));
  above_statusbar_action_->setCheckable(true);
  Settings s;
  s.beginGroup(kSettingsGroup);
  above_statusbar_action_->setChecked(s.value(kAboveStatusBar, false).toBool());
  s.endGroup();
  QObject::connect(above_statusbar_action_, &QAction::toggled, this, &PlayingWidget::ShowAboveStatusBar);
  QObject::connect(album_cover_choice_controller_, &AlbumCoverChoiceController::AutomaticCoverSearchDone, this, &PlayingWidget::AutomaticCoverSearchDone);
}

void PlayingWidget::SetEnabled(const bool enabled) {
  if (enabled == enabled_) return;
  if (enabled) SetEnabled();
  else SetDisabled();
}
void PlayingWidget::SetEnabled() {
  if (enabled_) return;
  enabled_ = true;
  if (active_) SetVisible(true);
}
void PlayingWidget::SetDisabled() {
  if (!enabled_) return;
  enabled_ = false;
  SetVisible(false);
}
void PlayingWidget::SetVisible(const bool visible) {
  if (timeline_show_hide_->state() == QTimeLine::Running) {
    if (timeline_show_hide_->direction() == QTimeLine::Backward && enabled_ && active_) timeline_show_hide_->toggleDirection();
    else if (timeline_show_hide_->direction() == QTimeLine::Forward && (!enabled_ || !active_)) timeline_show_hide_->toggleDirection();
    return;
  }
  if (visible != visible_) {
    timeline_show_hide_->setFrameRange(0, total_height_);
    timeline_show_hide_->setDirection(visible ? QTimeLine::Forward : QTimeLine::Backward);
    timeline_show_hide_->start();
  }
}
void PlayingWidget::set_ideal_height(const int height) { small_ideal_height_ = height; UpdateHeight(); }
QSize PlayingWidget::sizeHint() const { return QSize(desired_height_, total_height_); }
void PlayingWidget::CreateModeAction(const Mode mode, const QString &text, QActionGroup *group) {
  QAction *action = new QAction(text, group);
  action->setCheckable(true);
  QObject::connect(action, &QAction::triggered, this, [this, mode]() { SetMode(mode); });
  if (mode == mode_) action->setChecked(true);
}
void PlayingWidget::SetMode(const Mode mode) {
  mode_ = mode;
  fit_cover_width_action_->setEnabled(mode_ != Mode::SmallSongDetails);
  UpdateHeight(); UpdateDetailsText(); update();
  Settings s; s.beginGroup(kSettingsGroup); s.setValue(kMode, static_cast<int>(mode_)); s.endGroup();
}
void PlayingWidget::FitCoverWidth(const bool fit) {
  fit_width_ = fit; UpdateHeight(); update();
  Settings s; s.beginGroup(kSettingsGroup); s.setValue(kFitCoverWidth, fit_width_); s.endGroup();
}
void PlayingWidget::ShowAboveStatusBar(const bool above) {
  Settings s; s.beginGroup(kSettingsGroup); s.setValue(kAboveStatusBar, above); s.endGroup();
  Q_EMIT ShowAboveStatusBarChanged(above);
}
void PlayingWidget::Playing() {}
void PlayingWidget::Stopped() {
  playing_ = false; active_ = false; song_playing_ = Song(); song_ = Song(); image_current_ = QImage(); SetVisible(false);
}
void PlayingWidget::Error() { active_ = false; }
void PlayingWidget::SongChanged(const Song &song) {
  bool changed = (song.artist() != song_playing_.artist() || song.album() != song_playing_.album() || song.title() != song_playing_.title());
  playing_ = true; song_playing_ = song; song_ = song;
  if (changed) UpdateDetailsText();
}
void PlayingWidget::AlbumCoverLoaded(const Song &song, const QImage &image) {
  if (!playing_ || song != song_playing_ || image == image_current_) return;
  active_ = true; downloading_covers_ = false; song_ = song; image_current_ = image; SetImage(image);
}
void PlayingWidget::SetImage(const QImage &image) {
  if (enabled_ && visible_ && active_) {
    QSize psize; psize.setWidth(static_cast<int>(size().width() * devicePixelRatioF()));
    if (size().height() > 0) psize.setHeight(static_cast<int>(size().height() * devicePixelRatioF()));
    else psize.setHeight(static_cast<int>(total_height_ * devicePixelRatioF()));
    pixmap_previous_track_ = QPixmap(psize); pixmap_previous_track_.setDevicePixelRatio(devicePixelRatioF());
    pixmap_previous_track_.fill(QColor(0x10,0x11,0x14));
    pixmap_previous_track_opacity_ = 1.0; QPainter p(&pixmap_previous_track_); DrawContents(&p); p.end();
  } else pixmap_previous_track_ = QPixmap();
  image_original_ = image; UpdateDetailsText(); ScaleCover();
  if (enabled_ && active_) {
    SetVisible(true);
    if (!pixmap_previous_track_.isNull()) { timeline_fade_->stop(); timeline_fade_->start(); }
  }
}
void PlayingWidget::ScaleCover() {
  QImage image = ImageUtils::ScaleImage(image_original_, QSize(desired_height_, desired_height_), devicePixelRatioF(), true);
  if (image.isNull()) pixmap_cover_ = QPixmap(); else pixmap_cover_ = QPixmap::fromImage(image); update();
}
void PlayingWidget::SetHeight(int height) {
  setMaximumHeight(height); update();
  if (height >= total_height_ - 5) visible_ = true;
if (height <= 5) visible_ = false;
  if (timeline_show_hide_->state() == QTimeLine::Running) {
    if (timeline_show_hide_->direction() == QTimeLine::Backward && enabled_ && active_) timeline_show_hide_->toggleDirection();
    if (timeline_show_hide_->direction() == QTimeLine::Forward && (!enabled_ || !active_)) timeline_show_hide_->toggleDirection();
  }
}
void PlayingWidget::UpdateHeight() {
  switch (mode_) {
    case Mode::SmallSongDetails: desired_height_ = small_ideal_height_; total_height_ = small_ideal_height_; break;
    case Mode::LargeSongDetails:
      if (fit_width_) desired_height_ = width(); else desired_height_ = qMin(kMaxCoverSize, width());
      total_height_ = kTopBorder + desired_height_ + kBottomOffset + static_cast<int>(details_->size().height()) + 40;
      break;
  }
  timeline_show_hide_->setFrameRange(0, total_height_);
  if (visible_ && active_ && timeline_show_hide_->state() != QTimeLine::Running) setMaximumHeight(total_height_);
  if (song_.is_valid()) ScaleCover();
  updateGeometry();
}
void PlayingWidget::UpdateDetailsText() {
  QString html;
  // Estilo Roon + Astra - mayúsculas técnicas
  details_->setDefaultStyleSheet(
    u"p { font-family: 'Segoe UI', 'Ubuntu', sans-serif; font-size: 11px; color: #8A8C94; line-height: 16px; margin:0; } "
    u"b { font-family: 'Segoe UI', sans-serif; font-size: 16px; font-weight: 700; color: #E7E8EC; letter-spacing: 0.8px; text-transform: uppercase; } "
    u"span.meta { font-family: 'JetBrains Mono', monospace; font-size: 11px; color: #8A8C94; } "
    u"span.accent { color: #C1272D; font-weight: bold; }"_s
  );
  switch (mode_) {
    case Mode::SmallSongDetails: details_->setTextWidth(-1); html += "<p>"_L1; break;
    case Mode::LargeSongDetails: details_->setTextWidth(desired_height_); html += "<p align=left>"_L1; break;
  }
  // Formato como tu mockup: TITULO - ODYSSEY / Исполнитель: ...
  html += QStringLiteral("<b>%1</b><br/><span class='meta'>%2 • <span class='accent'>%3</span> • %4</span>")
            .arg(song_.PrettyTitle().toUpper().toHtmlEscaped(),
                 song_.artist().toHtmlEscaped(),
                 song_.album().toHtmlEscaped(),
                 QString::number(song_.year()));
  html += "</p>"_L1;
  details_->setHtml(html);
  if (mode_ == Mode::LargeSongDetails) UpdateHeight();
  update();
}
void PlayingWidget::paintEvent(QPaintEvent *e) {
  Q_UNUSED(e);
  QPainter p(this);
  p.fillRect(rect(), QColor(0x10,0x11,0x14));
  DrawContents(&p);
  if (!pixmap_previous_track_.isNull()) {
    p.setOpacity(pixmap_previous_track_opacity_);
    p.drawPixmap(0, 0, pixmap_previous_track_);
  }
}
void PlayingWidget::DrawContents(QPainter *p) {
  p->setRenderHint(QPainter::Antialiasing, true);
  p->setRenderHint(QPainter::SmoothPixmapTransform);
  switch (mode_) {
    case Mode::SmallSongDetails: {
      QRect target(0,0,small_ideal_height_,small_ideal_height_);
      QPainterPath path; path.addRoundedRect(target, 10, 10);
      p->setClipPath(path);
      p->drawPixmap(target, pixmap_cover_);
      p->setClipping(false);
      if (downloading_covers_ && spinner_animation_) p->drawPixmap(small_ideal_height_ - 18, 6, 16, 16, spinner_animation_->currentPixmap());
      p->translate(small_ideal_height_ + kPadding, 0);
      details_->drawContents(p);
      p->translate(-small_ideal_height_ - kPadding, 0);
      break;
    }
    case Mode::LargeSongDetails: {
      const int text_height = static_cast<int>(details_->size().height());
      const int cover_size = fit_width_ ? qMin(width()-20, kMaxCoverSize) : qMin(kMaxCoverSize, width()-20);
      const int x_offset = 10;

      // Tapa redondeada 18px + borde #2A2B31
      QRect coverRect(x_offset, kTopBorder, cover_size, cover_size);
      QPainterPath path; path.addRoundedRect(coverRect, 18, 18);
      p->setClipPath(path);
      p->drawPixmap(coverRect, pixmap_cover_);
      p->setClipping(false);
      p->setPen(QPen(QColor(0x2A,0x2B,0x31), 1));
      p->setBrush(Qt::NoBrush);
      p->drawRoundedRect(coverRect, 18, 18);

      {
       QPixmap badge(u":/data/hires-roon.png"_s);
        if (!badge.isNull()) {
          int badgeSize = 48;
          int bx = x_offset + cover_size - badgeSize - 10;
          int by = kTopBorder + cover_size - badgeSize - 10;
          QPainterPath badgePath;
          badgePath.addRoundedRect(bx, by, badgeSize, badgeSize, 12, 12);
          p->setClipping(false);
          p->save();
          p->setClipPath(badgePath);
          p->drawPixmap(QRect(bx, by, badgeSize, badgeSize), badge);
          p->restore();
        }
      }

      if (downloading_covers_ && spinner_animation_) p->drawPixmap(x_offset + 45, 35, 16, 16, spinner_animation_->currentPixmap());

      // Texto debajo con estilo Roon
      if (timeline_show_hide_->state() != QTimeLine::Running) {
        p->translate(x_offset, kTopBorder + cover_size + 16);
        details_->drawContents(p);
        p->translate(-x_offset, -(kTopBorder + cover_size + 16));
      }

      // Onda roja inferior como en tu mockup (fake por ahora, después la conectamos al analyzer real)
      int waveY = height() - 28;
      p->setPen(QPen(QColor(0xC1,0x27,0x2D,120), 1.5));
      for(int i=0; i< cover_size; i+=4){
        int h = (QRandomGenerator::global()->bounded(18)) + 2;
        if(i % 20 == 0) h+=10;
        p->drawLine(x_offset+i, waveY, x_offset+i, waveY - h);
      }
      break;
    }
  }
}
void PlayingWidget::FadePreviousTrack(const qreal value) {
  if (!visible_) return;
  pixmap_previous_track_opacity_ = value;
  if (qFuzzyCompare(pixmap_previous_track_opacity_, static_cast<qreal>(0.0))) pixmap_previous_track_ = QPixmap();
  update();
}
void PlayingWidget::resizeEvent(QResizeEvent *e) {
  if (e->oldSize() != e->size()) {
    if (mode_ == Mode::LargeSongDetails) { UpdateHeight(); UpdateDetailsText(); }
  }
}
void PlayingWidget::contextMenuEvent(QContextMenuEvent *e) { menu_->popup(mapToGlobal(e->pos())); }
void PlayingWidget::mouseDoubleClickEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton && song_.is_valid()) album_cover_choice_controller_->ShowCover(song_, image_original_);
}
void PlayingWidget::dragEnterEvent(QDragEnterEvent *e) {
  if (AlbumCoverChoiceController::CanAcceptDrag(e)) e->acceptProposedAction();
  QWidget::dragEnterEvent(e);
}
void PlayingWidget::dropEvent(QDropEvent *e) {
  album_cover_choice_controller_->SaveCover(&song_, e);
  QWidget::dropEvent(e);
}
void PlayingWidget::SearchCoverInProgress() {
  downloading_covers_ = true;
  spinner_animation_ = make_unique<QMovie>(u":/pictures/spinner.gif"_s, QByteArray(), this);
  QObject::connect(&*spinner_animation_, &QMovie::updated, this, &PlayingWidget::Update);
  spinner_animation_->start(); update();
}
void PlayingWidget::AutomaticCoverSearchDone() {
  downloading_covers_ = false; spinner_animation_.reset(); update();
}
