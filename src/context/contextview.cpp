/*
 * Strawberry Music Player
 * Copyright 2013-2025, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Astra Linux SE Mod - ContextView Roon-style + USB DAC detection
 * Copyright 2025-2026 Sebastián S. Orozco <https://github.com/SSOROZCO>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "config.h"

#include <utility>

#include <QtGlobal>
#include <QObject>
#include <QWidget>
#include <QList>
#include <QVariant>
#include <QString>
#include <QUrl>
#include <QImage>
#include <QIcon>
#include <QFont>
#include <QSize>
#include <QSizePolicy>
#include <QMenu>
#include <QAction>
#include <QFontDatabase>
#include <QLayoutItem>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QScrollArea>
#include <QSpacerItem>
#include <QLabel>
#include <QTextEdit>
#include <QSettings>
#include <QResizeEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QProcess>
#include <QTimer>

#include "core/song.h"
#include "core/settings.h"
#include "utilities/strutils.h"
#include "utilities/timeutils.h"
#include "widgets/resizabletextedit.h"
#include "collection/collectionview.h"
#include "covermanager/albumcoverchoicecontroller.h"
#include "lyrics/lyricsfetcher.h"
#include "constants/contextsettings.h"
#include "constants/timeconstants.h"

#include "contextview.h"
#include "contextalbum.h"

using namespace Qt::Literals::StringLiterals;

namespace {
constexpr int kWidgetSpacing = 50;
}  // namespace

ContextView::ContextView(QWidget *parent)
    : QWidget(parent),
      collectionview_(nullptr),
      album_cover_choice_controller_(nullptr),
      lyrics_fetcher_(nullptr),
      menu_options_(new QMenu(this)),
      action_show_album_(nullptr),
      action_show_data_(nullptr),
      action_show_lyrics_(nullptr),
      action_search_lyrics_(nullptr),
      layout_container_(new QVBoxLayout()),
      widget_scrollarea_(new QWidget(this)),
      layout_scrollarea_(new QVBoxLayout()),
      scrollarea_(new QScrollArea(this)),
      textedit_top_(new ResizableTextEdit(this)),
      widget_album_(new ContextAlbum(this)),
      widget_stacked_(new QStackedWidget(this)),
      widget_stop_(new QWidget(this)),
      widget_play_(new QWidget(this)),
      layout_stop_(new QVBoxLayout()),
      layout_play_(new QVBoxLayout()),
      label_stop_summary_(new QLabel(this)),
      widget_play_data_(new QWidget(this)),
      layout_play_data_(new QGridLayout()),
      textedit_play_lyrics_(new ResizableTextEdit(this)),
      spacer_play_data_(new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Fixed)),
      label_filetype_title_(new QLabel(this)),
      label_length_title_(new QLabel(this)),
      label_samplerate_title_(new QLabel(this)),
      label_bitdepth_title_(new QLabel(this)),
      label_bitrate_title_(new QLabel(this)),
      label_filetype_(new QLabel(this)),
      label_length_(new QLabel(this)),
      label_samplerate_(new QLabel(this)),
      label_dac_(new QLabel(this)),
      label_bitdepth_(new QLabel(this)),
      label_bitrate_(new QLabel(this)),
      lyrics_tried_(false),
      lyrics_id_(-1) {

  setLayout(layout_container_);

  layout_container_->setObjectName(u"context-layout-container"_s);
  layout_container_->setContentsMargins(0, 0, 0, 0);
  layout_container_->addWidget(scrollarea_);

  scrollarea_->setObjectName(u"context-scrollarea"_s);
  scrollarea_->setWidgetResizable(true);
  scrollarea_->setWidget(widget_scrollarea_);
  scrollarea_->setContentsMargins(0, 0, 0, 0);
  scrollarea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollarea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  widget_scrollarea_->setObjectName(u"context-widget-scrollarea"_s);
  widget_scrollarea_->setLayout(layout_scrollarea_);
  widget_scrollarea_->setContentsMargins(0, 0, 0, 0);

  textedit_top_->setReadOnly(true);
  textedit_top_->setFrameShape(QFrame::NoFrame);

  layout_scrollarea_->setObjectName(u"context-layout-scrollarea"_s);
    layout_scrollarea_->setContentsMargins(12, 12, 12, 12);
  textedit_top_->setMinimumWidth(220);
  textedit_top_->setMaximumWidth(320);
  layout_scrollarea_->addWidget(textedit_top_);
  layout_scrollarea_->addWidget(widget_album_);
  layout_scrollarea_->addWidget(widget_stacked_);
  layout_scrollarea_->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));

  widget_stacked_->setContentsMargins(0, 0, 0, 0);
  widget_stacked_->addWidget(widget_stop_);
  widget_stacked_->addWidget(widget_play_);
  widget_stacked_->setCurrentWidget(widget_stop_);

  widget_stop_->setLayout(layout_stop_);
  widget_stop_->setContentsMargins(0, 0, 0, 0);
  widget_play_->setLayout(layout_play_);
  widget_play_->setContentsMargins(0, 0, 0, 0);

  layout_stop_->setContentsMargins(0, 0, 0, 0);
  layout_play_->setContentsMargins(0, 0, 0, 0);

  // Stopped

  label_stop_summary_->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  layout_stop_->setContentsMargins(0, 0, 0, 0);
  layout_stop_->addWidget(label_stop_summary_);

  // Playing

  label_filetype_title_->setText(tr("Filetype"));
  label_length_title_->setText(tr("Length"));
  label_samplerate_title_->setText(tr("Samplerate"));
  label_bitdepth_title_->setText(tr("Bit depth"));
  label_bitrate_title_->setText(tr("Bitrate"));

  label_filetype_title_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  label_length_title_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  label_samplerate_title_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  label_bitdepth_title_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  label_bitrate_title_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  label_filetype_->setWordWrap(true);
  label_length_->setWordWrap(true);
  label_samplerate_->setWordWrap(true);
  label_bitdepth_->setWordWrap(true);
  label_bitrate_->setWordWrap(true);

  layout_play_data_->setContentsMargins(0, 0, 0, 0);
  layout_play_data_->addWidget(label_filetype_title_, 0, 0);
  layout_play_data_->addWidget(label_filetype_, 0, 1);
  layout_play_data_->addWidget(label_length_title_, 1, 0);
  layout_play_data_->addWidget(label_length_, 1, 1);
  layout_play_data_->addWidget(label_samplerate_title_, 2, 0);
  layout_play_data_->addWidget(label_samplerate_, 2, 1);
  layout_play_data_->addWidget(label_bitdepth_title_, 3, 0);
  layout_play_data_->addWidget(label_bitdepth_, 3, 1);
  layout_play_data_->addWidget(label_bitrate_title_, 4, 0);
  layout_play_data_->addWidget(label_bitrate_, 4, 1);

  widget_play_data_->setLayout(layout_play_data_);

  textedit_play_lyrics_->setReadOnly(true);
  textedit_play_lyrics_->setFrameShape(QFrame::NoFrame);
  textedit_play_lyrics_->hide();

  layout_play_->setContentsMargins(0, 0, 0, 0);
  layout_play_->addWidget(widget_play_data_);
  layout_play_->addSpacerItem(spacer_play_data_);
  layout_play_->addWidget(textedit_play_lyrics_);
  layout_play_->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));

  labels_play_ << label_filetype_title_
               << label_length_title_
               << label_samplerate_title_
               << label_bitdepth_title_
               << label_bitrate_title_;

  labels_play_data_ << label_filetype_
                    << label_length_
                    << label_samplerate_
                    << label_bitdepth_
                    << label_bitrate_;

  labels_play_all_ = labels_play_ << labels_play_data_;

  textedit_play_ << textedit_play_lyrics_;

   QObject::connect(widget_album_, &ContextAlbum::FadeStopFinished, this, &ContextView::FadeStopFinished);

  label_dac_->setWordWrap(true);
  label_dac_->hide();

    // --- ASTRA LINUX SE: CACHEO DAC UNIVERSAL ---
  // Mod by Sebastián S. Orozco - https://github.com/SSOROZCO
  // Detecta DAC USB real (FiiO, iFi, Topping, etc) al iniciar Astra.
  // Ignora Built-in / HDMI. Solo se resetea al abrir el programa.
  QProcess p;
  // Solo sinks que son USB (device.bus = "usb")
  p.start(u"bash"_s, QStringList() << u"-c"_s << u"pactl list sinks | grep -B5 'device.bus = \"usb\"' | grep 'Description:' | head -n1 | cut -d: -f2 | xargs"_s);
  p.waitForFinished(500);
  cached_dac_ = QString::fromUtf8(p.readAllStandardOutput()).trimmed();

  if(cached_dac_.contains(u"Built-in"_s, Qt::CaseInsensitive) ||
     cached_dac_.contains(u"HDMI"_s, Qt::CaseInsensitive) ||
     cached_dac_.isEmpty()){
    cached_dac_.clear();
  } else {
    if(cached_dac_.contains(u"FiiO Q1"_s, Qt::CaseInsensitive)){
      cached_dac_ = u"FiiO Q1"_s;
    }
    // si es iFi, Topping, SMSL, etc lo dejamos tal cual viene de pactl
  }

  // Fallback por lsusb solo para FiiO
  if(cached_dac_.isEmpty()){
    QProcess u;
    u.start(u"bash"_s, QStringList() << u"-c"_s << u"lsusb | grep -i fiio"_s);
    u.waitForFinished(300);
    if(!QString::fromUtf8(u.readAllStandardOutput()).trimmed().isEmpty()) cached_dac_ = u"FiiO Q1"_s;
  }
}

void ContextView::Init(CollectionView *collectionview, AlbumCoverChoiceController *album_cover_choice_controller, SharedPtr<LyricsProviders> lyrics_providers) {

  collectionview_ = collectionview;
  album_cover_choice_controller_ = album_cover_choice_controller;

  widget_album_->Init(this, album_cover_choice_controller_);
  lyrics_fetcher_ = new LyricsFetcher(lyrics_providers, this);

  QObject::connect(collectionview_, &CollectionView::TotalSongCountUpdated_, this, &ContextView::UpdateNoSong);
  QObject::connect(collectionview_, &CollectionView::TotalArtistCountUpdated_, this, &ContextView::UpdateNoSong);
  QObject::connect(collectionview_, &CollectionView::TotalAlbumCountUpdated_, this, &ContextView::UpdateNoSong);
  QObject::connect(lyrics_fetcher_, &LyricsFetcher::LyricsFetched, this, &ContextView::UpdateLyrics);

  AddActions();

}

void ContextView::AddActions() {

  action_show_album_ = new QAction(tr("Show album cover"), this);
  action_show_album_->setCheckable(true);
  action_show_album_->setChecked(true);

  action_show_data_ = new QAction(tr("Show song technical data"), this);
  action_show_data_->setCheckable(true);
  action_show_data_->setChecked(true);

  action_show_lyrics_ = new QAction(tr("Show song lyrics"), this);
  action_show_lyrics_->setCheckable(true);
  action_show_lyrics_->setChecked(true);

  action_search_lyrics_ = new QAction(tr("Automatically search for song lyrics"), this);
  action_search_lyrics_->setCheckable(true);
  action_search_lyrics_->setChecked(true);

  menu_options_->addAction(action_show_album_);
  menu_options_->addAction(action_show_data_);
  menu_options_->addAction(action_show_lyrics_);
  menu_options_->addAction(action_search_lyrics_);
  menu_options_->addSeparator();

  ReloadSettings();

  QObject::connect(action_show_album_, &QAction::triggered, this, &ContextView::ActionShowAlbum);
  QObject::connect(action_show_data_, &QAction::triggered, this, &ContextView::ActionShowData);
  QObject::connect(action_show_lyrics_, &QAction::triggered, this, &ContextView::ActionShowLyrics);
  QObject::connect(action_search_lyrics_, &QAction::triggered, this, &ContextView::ActionSearchLyrics);

}

void ContextView::ReloadSettings() {

  QString default_font;
  if (QFontDatabase::families().contains(QLatin1String(ContextSettings::kDefaultFontFamily))) {
    default_font = QLatin1String(ContextSettings::kDefaultFontFamily);
  }
  else {
    default_font = font().family();
  }

  Settings s;
  s.beginGroup(ContextSettings::kSettingsGroup);
  title_fmt_ = s.value(ContextSettings::kSettingsTitleFmt, QLatin1String(ContextSettings::kDefaultTitleFmt)).toString();
  summary_fmt_ = s.value(ContextSettings::kSettingsSummaryFmt, QLatin1String(ContextSettings::kDefaultSummaryFmt)).toString();
  action_show_album_->setChecked(s.value(ContextSettings::kAlbum, ContextSettings::kDefaultAlbum).toBool());
  action_show_data_->setChecked(s.value(ContextSettings::kTechnicalData, ContextSettings::kDefaultTechnicalData).toBool());
  action_show_lyrics_->setChecked(s.value(ContextSettings::kSongLyrics, ContextSettings::kDefaultSongLyrics).toBool());
  action_search_lyrics_->setChecked(s.value(ContextSettings::kSearchLyrics, ContextSettings::kDefaultSearchLyrics).toBool());
  font_headline_.setFamily(s.value(ContextSettings::kFontHeadline, default_font).toString());
  font_headline_.setPointSizeF(s.value(ContextSettings::kFontSizeHeadline, ContextSettings::kDefaultFontSizeHeadline).toReal());
  font_nosong_.setFamily(font_headline_.family());
  font_nosong_.setPointSizeF(font_headline_.pointSizeF() * 1.6F);
  font_normal_.setFamily(s.value(ContextSettings::kFontNormal, default_font).toString());
  font_normal_.setPointSizeF(s.value(ContextSettings::kFontSizeNormal, font().pointSizeF()).toReal());
  s.endGroup();

  UpdateFonts();

  if (widget_stacked_->currentWidget() == widget_stop_) {
    NoSong();
  }
  else {
    SetSong();
  }

}

void ContextView::resizeEvent(QResizeEvent *e) {

  if (e->size().width() != e->oldSize().width()) {
    widget_album_->UpdateWidth(width() - kWidgetSpacing);
  }

  QWidget::resizeEvent(e);

}

void ContextView::Playing() {}

void ContextView::Stopped() {

  song_playing_ = Song();
  song_prev_ = Song();
  lyrics_.clear();
  image_original_ = QImage();
  widget_album_->SetImage();

}

void ContextView::Error() {}

void ContextView::SongChanged(const Song &song) {

  if (widget_stacked_->currentWidget() == widget_play_ && song_playing_.is_valid() && song == song_playing_ && song.title() == song_playing_.title() && song.album() == song_playing_.album() && song.artist() == song_playing_.artist()) {
    UpdateSong(song);
  }
  else {
    song_prev_ = song_playing_;
    song_playing_ = song;
    lyrics_ = song.lyrics();
    lyrics_id_ = -1;
    lyrics_tried_ = false;
    SetSong();
  }

  SearchLyrics();

}

void ContextView::SearchLyrics() {

  if (lyrics_.isEmpty() && action_show_lyrics_->isChecked() && action_search_lyrics_->isChecked() && !song_playing_.artist().isEmpty() && !song_playing_.title().isEmpty() && !lyrics_tried_ && lyrics_id_ == -1) {
    lyrics_fetcher_->Clear();
    lyrics_tried_ = true;
    lyrics_id_ = static_cast<qint64>(lyrics_fetcher_->Search(song_playing_.effective_albumartist(), song_playing_.artist(), song_playing_.album(), song_playing_.title(), song_playing_.length_nanosec() / kNsecPerSec));
  }

}

void ContextView::FadeStopFinished() {

  widget_stacked_->setCurrentWidget(widget_stop_);
  NoSong();
  ResetSong();
  widget_stacked_->updateGeometry();

}

void ContextView::SetLabelText(QLabel *label, int value, const QString &suffix, const QString &def) {
  label->setText(value <= 0 ? def : (QString::number(value) + QLatin1Char(' ') + suffix));
}

void ContextView::UpdateNoSong() {
  if (widget_stacked_->currentWidget() == widget_stop_) NoSong();
}

void ContextView::NoSong() {

  if (!widget_album_->isVisibleTo(this)) {
    widget_album_->show();
  }

  textedit_top_->setFont(font_nosong_);
  textedit_top_->SetText(tr("No song playing"));

  QString html;
  if (collectionview_->TotalSongs() == 1) html += tr("%1 song").arg(collectionview_->TotalSongs());
  else html += tr("%1 songs").arg(collectionview_->TotalSongs());
  html += "<br />"_L1;

  if (collectionview_->TotalArtists() == 1) html += tr("%1 artist").arg(collectionview_->TotalArtists());
  else html += tr("%1 artists").arg(collectionview_->TotalArtists());
  html += "<br />"_L1;

  if (collectionview_->TotalAlbums() == 1) html += tr("%1 album").arg(collectionview_->TotalAlbums());
  else html += tr("%1 albums").arg(collectionview_->TotalAlbums());
  html += "<br />"_L1;

  label_stop_summary_->setFont(font_normal_);
  label_stop_summary_->setText(html);

}

void ContextView::UpdateFonts() {

  for (QLabel *l : std::as_const(labels_play_all_)) {
    l->setFont(font_normal_);
  }
  for (QTextEdit *e : std::as_const(textedit_play_)) {
    e->setFont(font_normal_);
  }

}

void ContextView::SetSong() {

  // === ASTRA MOD: Roon minimal title, sin cortes ===
  QString title = Utilities::ReplaceMessage(title_fmt_, song_playing_, u"<br />"_s, true);
  QString summary = Utilities::ReplaceMessage(summary_fmt_, song_playing_, u"<br />"_s, true);

  QString html_top = QStringLiteral(
    "<div style='line-height: 1.3; max-width: 300px;'>"
    "<div style='font-family: Inter, SF Pro Display; font-weight: 700; font-size: 13px; color: #F5F5F7; letter-spacing: -0.2px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;'>%1</div>"
    "<div style='font-family: Inter; font-weight: 400; font-size: 11px; color: #8E8E93; margin-top: 2px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;'>%2</div>"
    "</div>"
  ).arg(title, summary);

  textedit_top_->setFont(font_headline_);
  textedit_top_->SetText(html_top);
  textedit_top_->setMinimumWidth(240);
  textedit_top_->setStyleSheet(u"background: transparent; border: none; padding: 0px 4px;"_s);

  label_stop_summary_->clear();

  bool widget_album_changed = !song_prev_.is_valid();
  if (action_show_album_->isChecked() && !widget_album_->isVisibleTo(this)) {
    widget_album_->show();
    widget_album_changed = true;
  }
  else if (!action_show_album_->isChecked() && widget_album_->isVisibleTo(this)) {
    widget_album_->hide();
    widget_album_changed = true;
  }
  if (widget_album_changed) Q_EMIT AlbumEnabledChanged();

  if (action_show_data_->isChecked()) {
    widget_play_data_->show();

    const QString ft = song_playing_.TextForFiletype().toUpper();
    const bool isLossy = ft.contains(u"MP3"_s) || ft.contains(u"AAC"_s) || ft.contains(u"OGG"_s) || ft.contains(u"OPUS"_s) || ft.contains(u"MPEG"_s);
    const bool isDSD = ft.contains(u"DSD"_s) || ft.contains(u"DSF"_s) || ft.contains(u"DSIFF"_s);
    const bool isHiRes = !isLossy && (isDSD || song_playing_.samplerate() > 48000 || song_playing_.bitdepth() > 16);
    const bool isLossless = !isLossy;
    const int samplerate = song_playing_.samplerate();
    const int bitdepth = song_playing_.bitdepth();
    const int bitrate = song_playing_.bitrate();

    label_filetype_title_->hide();
    label_length_title_->hide();
    label_samplerate_title_->hide();
    label_bitdepth_title_->hide();
    label_bitrate_title_->hide();
    label_bitdepth_->hide();
    label_bitrate_->hide();

        if (isHiRes) {
      widget_play_data_->setStyleSheet(u"QWidget { background: #141416; border: 1px solid #2A1A1A; border-left: 3px solid #B71C1C; border-radius: 10px; }"_s);
    } else {
      widget_play_data_->setStyleSheet(u"QWidget { background: #121214; border: 1px solid #232326; border-radius: 10px; }"_s);
    }
    layout_play_data_->setContentsMargins(14,12,14,12);
    layout_play_data_->setSpacing(4);

    layout_play_data_->removeWidget(label_filetype_);
    layout_play_data_->removeWidget(label_length_);
    layout_play_data_->removeWidget(label_samplerate_);
    layout_play_data_->removeWidget(label_dac_);
    layout_play_data_->addWidget(label_filetype_, 0, 0);
    layout_play_data_->addWidget(label_length_, 1, 0);
    layout_play_data_->addWidget(label_samplerate_, 2, 0);
    layout_play_data_->addWidget(label_dac_, 3, 0);

    label_filetype_->setText(u"%1 • %2BIT / %3Hz"_s.arg(ft, QString::number(bitdepth > 0 ? bitdepth : 16), QString::number(samplerate > 0 ? samplerate : 44100)));
    label_filetype_->setStyleSheet(u"color: #D32F2F; font-family: 'Inter'; font-weight: 700; font-size: 11px; letter-spacing: 1px; background: transparent; border: none;"_s);
    label_filetype_->show();

    label_length_->setText(u"%1 kbps • %2"_s.arg(bitrate).arg(Utilities::PrettyTimeNanosec(song_playing_.length_nanosec())));
    label_length_->setStyleSheet(u"color: #8A8C94; font-family: 'JetBrains Mono'; font-size: 10px; background: transparent; border: none;"_s);
    label_length_->show();

    // --- DAC SIEMPRE VISIBLE, ORGULLOSO ---
    if(!cached_dac_.isEmpty()){
      if(cached_dac_.contains(u"FiiO Q1"_s, Qt::CaseInsensitive) || cached_dac_.contains(u"FIIO"_s)){
        label_dac_->setText(u"⬢ FiiO Q1 • USB DAC"_s);
        label_dac_->setStyleSheet(u"color: #FF2D2D; font-family: 'Inter', 'JetBrains Mono'; font-weight: 800; font-size: 11px; letter-spacing: 0.8px; background: transparent; border: none; margin-top: 8px; padding-top: 6px; border-top: 1px solid #2A2A2E;"_s);
      } else {
        label_dac_->setText(cached_dac_);
        label_dac_->setStyleSheet(u"color: #FFFFFF; font-family: 'Inter', 'JetBrains Mono'; font-weight: 700; font-size: 11px; letter-spacing: 0.8px; background: transparent; border: none; margin-top: 8px; padding-top: 6px; border-top: 1px solid #2A2A2E;"_s);
      }
      label_dac_->show();
    } else {
      label_dac_->hide();
    }

    if (isHiRes) {
      label_samplerate_->setText(u"• HI-RES AUDIO"_s);
      label_samplerate_->setStyleSheet(u"color: #C1272D; font-family: 'JetBrains Mono'; font-weight: 800; font-size: 9px; letter-spacing: 2px; background: transparent; border-top: 1px solid #1E1E22; margin-top: 6px; padding-top: 6px;"_s);
      label_samplerate_->show();
    } else if (isLossless) {
      label_samplerate_->setText(u"● LOSSLESS"_s);
      label_samplerate_->setStyleSheet(u"color: #8A8C94; font-family: 'JetBrains Mono'; font-weight: 800; font-size: 9px; letter-spacing: 2px; background: transparent; border-top: 1px solid #1E1E22; margin-top: 6px; padding-top: 6px;"_s);
      label_samplerate_->show();
       } else {
      label_samplerate_->setText(u"● %1"_s.arg(ft));
      label_samplerate_->setStyleSheet(u"color: #6A6C74; font-family: 'JetBrains Mono'; font-weight: 600; font-size: 9px; letter-spacing: 2px; background: transparent; border-top: 1px solid #1E1E22; margin-top: 6px; padding-top: 6px;"_s);
      label_samplerate_->show();
    }

    spacer_play_data_->changeSize(20, 20, QSizePolicy::Fixed);
  } // cierra if(action_show_data_)

  if (action_show_lyrics_->isChecked() && !lyrics_.isEmpty()) {
    textedit_play_lyrics_->SetText(lyrics_);
    textedit_play_lyrics_->show();
  }
  else {
    textedit_play_lyrics_->clear();
    textedit_play_lyrics_->hide();
  }

  widget_stacked_->setCurrentWidget(widget_play_);
  widget_stacked_->updateGeometry();
} // cierra SetSong()

void ContextView::UpdateSong(const Song &song) {

  const QString top_text = QStringLiteral("<b>%1</b><br />%2").arg(Utilities::ReplaceMessage(title_fmt_, song, u"<br />"_s, true), Utilities::ReplaceMessage(summary_fmt_, song, u"<br />"_s, true));
  if (top_text != textedit_top_->Text()) {
    textedit_top_->SetText(top_text);
  }

  if (action_show_data_->isChecked()) {
    const QString ft = song.TextForFiletype().toUpper();
    const bool isLossy = ft.contains(u"MP3"_s) || ft.contains(u"AAC"_s) || ft.contains(u"OGG"_s) || ft.contains(u"OPUS"_s) || ft.contains(u"MPEG"_s);
    const bool isDSD = ft.contains(u"DSD"_s) || ft.contains(u"DSF"_s) || ft.contains(u"DSIFF"_s);
    const bool isHiRes = !isLossy && (isDSD || song.samplerate() > 48000 || song.bitdepth() > 16);
    const bool isLossless = !isLossy;

    if (isHiRes) widget_play_data_->setStyleSheet(u"QWidget { background: #0A0A0C; border: 1.5px solid #C1272D; border-radius: 14px; }"_s);
    else widget_play_data_->setStyleSheet(u"QWidget { background: #08080A; border: 1px solid #1E1E22; border-radius: 14px; }"_s);

    label_filetype_->setText(u"%1 • %2BIT / %3Hz"_s.arg(ft, QString::number(song.bitdepth() > 0 ? song.bitdepth() : 16), QString::number(song.samplerate() > 0 ? song.samplerate() : 44100)));
    label_length_->setText(u"%1 kbps • %2"_s.arg(song.bitrate()).arg(Utilities::PrettyTimeNanosec(song.length_nanosec())));

    // --- DAC SIEMPRE VISIBLE, ORGULLOSO, ROJO ROON ---
    if(!cached_dac_.isEmpty()){
      if(cached_dac_.contains(u"FiiO Q1"_s, Qt::CaseInsensitive) || cached_dac_.contains(u"FIIO"_s)){
        label_dac_->setText(u"⬢ FiiO Q1 • USB DAC"_s);
        label_dac_->setStyleSheet(u"color: #FF2D2D; font-family: 'Inter', 'JetBrains Mono'; font-weight: 800; font-size: 11px; letter-spacing: 0.8px; background: transparent; border: none; margin-top: 8px; padding-top: 6px; border-top: 1px solid #2A2A2E;"_s);
      } else {
        label_dac_->setText(cached_dac_);
        label_dac_->setStyleSheet(u"color: #FFFFFF; font-family: 'Inter', 'JetBrains Mono'; font-weight: 700; font-size: 11px; letter-spacing: 0.8px; background: transparent; border: none; margin-top: 8px; padding-top: 6px; border-top: 1px solid #2A2A2E;"_s);
      }
      label_dac_->show();
    } else {
      label_dac_->hide();
    }

    if (isHiRes) {
      label_samplerate_->setText(u"● HI-RES AUDIO"_s);
      label_samplerate_->setStyleSheet(u"color: #C1272D; font-family: 'JetBrains Mono'; font-weight: 800; font-size: 9px; letter-spacing: 2px; background: transparent; border-top: 1px solid #1E1E22; margin-top: 6px; padding-top: 6px;"_s);
      label_samplerate_->show();
    } else if (isLossless) {
      label_samplerate_->setText(u"● LOSSLESS"_s);
      label_samplerate_->setStyleSheet(u"color: #8A8C94; font-family: 'JetBrains Mono'; font-weight: 800; font-size: 9px; letter-spacing: 2px; background: transparent; border-top: 1px solid #1E1E22; margin-top: 6px; padding-top: 6px;"_s);
      label_samplerate_->show();
    } else {
      label_samplerate_->setText(u"● %1"_s.arg(ft));
      label_samplerate_->setStyleSheet(u"color: #6A6C74; font-family: 'JetBrains Mono'; font-weight: 600; font-size: 9px; letter-spacing: 2px; background: transparent; border-top: 1px solid #1E1E22; margin-top: 6px; padding-top: 6px;"_s);
      label_samplerate_->show();
    }
  }

  song_playing_ = song;
  widget_stacked_->updateGeometry();
}
void ContextView::ResetSong() {

  for (QLabel *l : std::as_const(labels_play_data_)) {
    l->clear();
  }

  for (QTextEdit *l : std::as_const(textedit_play_)) {
    l->clear();
  }

  widget_play_data_->hide();
  textedit_play_lyrics_->hide();

}

void ContextView::UpdateLyrics(const quint64 id, const QString &provider, const QString &lyrics) {

  if (static_cast<qint64>(id) != lyrics_id_) return;

  if (lyrics.isEmpty()) {
    lyrics_ = "No lyrics found.\n"_L1;
  }
  else {
    lyrics_ = lyrics + "\n\n(Lyrics from "_L1 + provider + ")\n"_L1;
  }
  lyrics_id_ = -1;

  if (action_show_lyrics_->isChecked() && !lyrics_.isEmpty()) {
    textedit_play_lyrics_->SetText(lyrics_);
    textedit_play_lyrics_->show();
  }
  else {
    textedit_play_lyrics_->clear();
    textedit_play_lyrics_->hide();
  }

}

void ContextView::contextMenuEvent(QContextMenuEvent *e) {

  if (menu_options_ && widget_stacked_->currentWidget() == widget_stop_) {
    menu_options_->popup(mapToGlobal(e->pos()));
  }
  else {
    QWidget::contextMenuEvent(e);
  }

}

void ContextView::dragEnterEvent(QDragEnterEvent *e) {

  if (song_playing_.is_valid() && AlbumCoverChoiceController::CanAcceptDrag(e)) {
    e->acceptProposedAction();
  }

  QWidget::dragEnterEvent(e);

}

void ContextView::dropEvent(QDropEvent *e) {

  if (song_playing_.is_valid()) {
    album_cover_choice_controller_->SaveCover(&song_playing_, e);
  }

  QWidget::dropEvent(e);

}

void ContextView::AlbumCoverLoaded(const Song &song, const QImage &image) {

  if (song != song_playing_ || image == image_original_) return;

  widget_album_->SetImage(image);
  image_original_ = image;

}

void ContextView::ActionShowAlbum() {

  Settings s;
  s.beginGroup(ContextSettings::kSettingsGroup);
  s.setValue(ContextSettings::kAlbum, action_show_album_->isChecked());
  s.endGroup();

  if (song_playing_.is_valid()) SetSong();

}

void ContextView::ActionShowData() {

  Settings s;
  s.beginGroup(ContextSettings::kSettingsGroup);
  s.setValue(ContextSettings::kTechnicalData, action_show_data_->isChecked());
  s.endGroup();

  if (song_playing_.is_valid()) SetSong();

}

void ContextView::ActionShowLyrics() {

  Settings s;
  s.beginGroup(ContextSettings::kSettingsGroup);
  s.setValue(ContextSettings::kSongLyrics, action_show_lyrics_->isChecked());
  s.endGroup();

  if (song_playing_.is_valid()) SetSong();

  SearchLyrics();

}

void ContextView::ActionSearchLyrics() {

  Settings s;
  s.beginGroup(ContextSettings::kSettingsGroup);
  s.setValue(ContextSettings::kSearchLyrics, action_search_lyrics_->isChecked());
  s.endGroup();

  if (song_playing_.is_valid()) SetSong();

  SearchLyrics();

}
