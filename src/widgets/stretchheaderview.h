/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 * Copyright 2018-2024, Jonas Kvinge <jonas@jkvinge.net>
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
#ifndef STRETCHHEADERVIEW_H
#define STRETCHHEADERVIEW_H

#include <QObject>
#include <QByteArray>
#include <QHeaderView>
#include <QList>
#include <QString>

class QWidget;
class QAbstractItemModel;
class QMouseEvent;
class QResizeEvent;
class QPainter;

class StretchHeaderView : public QHeaderView {
  Q_OBJECT

 public:
  explicit StretchHeaderView(const Qt::Orientation orientation, QWidget *parent = nullptr);

  using ColumnWidthType = double;

  void setModel(QAbstractItemModel *model) override;

  bool is_stretch_enabled() const { return stretch_enabled_; }

  bool RestoreState(const QByteArray &state);
  QByteArray SaveState() const;
  QByteArray ResetState();

  void ShowSection(const int logical_index);
  void HideSection(const int logical_index);
  void SetSectionHidden(const int logical_index, const bool hidden);
  void SetColumnWidth(const int logical_index, const ColumnWidthType width);

 public Q_SLOTS:
  void ToggleStretchEnabled();
  void SetStretchEnabled(const bool enabled);

 Q_SIGNALS:
  void StretchEnabledChanged(const bool enabled);

 protected:
  void mouseMoveEvent(QMouseEvent *e) override;
  void resizeEvent(QResizeEvent *e) override;
  // Astra SE FASE 5 - Roon header
  void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;

 private:
  void NormaliseWidths(const QList<int> &sections = QList<int>());
  void ResizeSections(const QList<int> &sections = QList<int>());

 private Q_SLOTS:
  void SectionResized(const int logical, const int old_size, const int new_size);

 private:
  bool stretch_enabled_;
  QList<ColumnWidthType> column_widths_;
  bool in_mouse_move_event_;
  int forced_resize_logical_index_;
};

#endif  // STRETCHHEADERVIEW_H
