/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2013, David Sansome <me@davidsansome.com>
 * Copyright 2018-2021, Jonas Kvinge <jonas@jkvinge.net>
 * ASTRA MOD - Roon Sidebar Playlists
 */

#include "config.h"

#include <QWidget>
#include <QFont>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QRect>
#include <QPaintEvent>
#include <QStyleOptionViewItem>

#include "widgets/autoexpandingtreeview.h"
#include "playlistlistview.h"
#include "playlist.h"

using namespace Qt::Literals::StringLiterals;

namespace {
constexpr int kCardRadius = 6;
constexpr int kCardMargin = 3;
}

PlaylistListView::PlaylistListView(QWidget *parent) : AutoExpandingTreeView(parent) {
  setRootIsDecorated(false);
  setUniformRowHeights(true);
}

void PlaylistListView::paintEvent(QPaintEvent *event) {
  if (model()->rowCount() <= 0) {
    QPainter p(viewport());
    QRect rect(viewport()->rect());
    p.fillRect(rect, QColor(0x0A, 0x0A, 0x0A));
    p.setPen(QColor(0x6E, 0x6E, 0x73));
    QFont f = p.font();
    f.setPointSize(9);
    p.setFont(f);
    p.drawText(rect, Qt::AlignHCenter | Qt::TextWordWrap,
               "\n\n"_L1 + tr("You can favorite playlists by clicking the star icon") +
               "\n\n"_L1 + tr("Favorited playlists will be saved here"));
  } else {
    QPainter p(viewport());
    p.fillRect(viewport()->rect(), QColor(0x0A, 0x0A, 0x0A));
    AutoExpandingTreeView::paintEvent(event);
  }
}

void PlaylistListView::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, false); // Roon no usa antialias en filas

  QRect rowRect = option.rect.adjusted(kCardMargin, 1, -kCardMargin, -1);
  bool selected = option.state & QStyle::State_Selected;
  bool hover = option.state & QStyle::State_MouseOver;

  // Roon detecta playing por la propiedad del model, no por UserRole
  // En Strawberry el current playlist tiene font bold o data especial
  bool isCurrent = false;
  // Si el model tiene la playlist activa, suele venir como font bold
  if (option.font.bold()) isCurrent = true;
  // Fallback: si el texto es el que suena, marcamos
  // index.data(Playlist::Role_IsCurrent) si existe

  QColor bg;
  if (isCurrent) {
    bg = QColor(0xC1, 0x27, 0x2D); // ROON PLAYING - Rojo FiiO
  } else if (selected) {
    bg = QColor(0x1C, 0x1C, 0x1E); // ROON SELECTED - Gris Apple
  } else if (hover) {
    bg = QColor(0x14, 0x14, 0x16);
  } else {
    bg = QColor(0x0A, 0x0A, 0x0A);
  }

  // Fondo
  painter->fillRect(rowRect, bg);

  // Linea izquierda Roon
  if (isCurrent) {
    QRect line = rowRect;
    line.setWidth(3);
    painter->fillRect(line, QColor(0xFF, 0xFF, 0xFF)); // blanca cuando suena
  } else if (selected) {
    QRect line = rowRect;
    line.setWidth(2);
    painter->fillRect(line, QColor(0xC1, 0x27, 0x2D)); // roja cuando seleccionada
  }

  painter->restore();

  // Texto
  QStyleOptionViewItem opt = option;
  opt.rect = rowRect.adjusted(12, 0, -6, 0);
  if (isCurrent) {
    opt.palette.setColor(QPalette::Text, QColor(0xFF, 0xFF, 0xFF));
  } else if (selected) {
    opt.palette.setColor(QPalette::Text, QColor(0xF5, 0xF5, 0xF7));
  } else {
    opt.palette.setColor(QPalette::Text, QColor(0x8E, 0x8E, 0x93));
  }
  QFont f = opt.font;
  f.setPointSize(10);
  f.setBold(isCurrent || selected);
  opt.font = f;

  QTreeView::drawRow(painter, opt, index);
}

bool PlaylistListView::ItemsSelected() const {
  return selectionModel()->selectedRows().count() > 0;
}

void PlaylistListView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  Q_UNUSED(selected) Q_UNUSED(deselected)
  Q_EMIT ItemsSelectedChanged(selectionModel()->selectedRows().count() > 0);
}

void PlaylistListView::dragEnterEvent(QDragEnterEvent *e) {
  if (e->mimeData()->hasFormat(QLatin1String(Playlist::kRowsMimetype))) e->acceptProposedAction();
  else AutoExpandingTreeView::dragEnterEvent(e);
}

void PlaylistListView::dragMoveEvent(QDragMoveEvent *e) {
  QModelIndex drag_hover_tab_ = indexAt(e->position().toPoint());
  if (e->mimeData()->hasFormat(QLatin1String(Playlist::kRowsMimetype))) {
    if (drag_hover_tab_ != currentIndex()) {
      e->setDropAction(Qt::CopyAction);
      e->accept(visualRect(drag_hover_tab_));
      setCurrentIndex(drag_hover_tab_);
      if (drag_hover_timer_.isActive()) drag_hover_timer_.stop();
      drag_hover_timer_.start(500, this);
    }
  } else AutoExpandingTreeView::dragMoveEvent(e);
}

void PlaylistListView::dragLeaveEvent(QDragLeaveEvent *e) {
  if (drag_hover_timer_.isActive()) drag_hover_timer_.stop();
  AutoExpandingTreeView::dragLeaveEvent(e);
}

void PlaylistListView::timerEvent(QTimerEvent *e) {
  QTreeView::timerEvent(e);
  if (e->timerId() == drag_hover_timer_.timerId()) {
    drag_hover_timer_.stop();
    if (currentIndex().isValid()) Q_EMIT doubleClicked(currentIndex());
  }
}

void PlaylistListView::dropEvent(QDropEvent *e) {
  if (e->mimeData()->hasFormat(QLatin1String(Playlist::kRowsMimetype))) {
    if (drag_hover_timer_.isActive()) drag_hover_timer_.stop();
    if (currentIndex().isValid()) Q_EMIT ItemMimeDataDroppedSignal(currentIndex(), e->mimeData());
  } else AutoExpandingTreeView::dropEvent(e);
}
