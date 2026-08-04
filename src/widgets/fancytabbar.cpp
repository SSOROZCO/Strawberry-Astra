/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2018, Vikram Ambrose <ambroseworks@gmail.com>
 * Copyright 2018-2024, Jonas Kvinge <jonas@jkvinge.net>
 *
 * ASTRA MOD - FiiO x Steve Jobs Edition
 */

#include <algorithm>

#include <QTabBar>
#include <QString>
#include <QFontMetrics>
#include <QFont>
#include <QStylePainter>
#include <QPaintEvent>
#include <QTransform>
#include <QLinearGradient>
#include <QColor>
#include <QPen>
#include <QTimer>
#include <QApplication>

#include "core/stylehelper.h"
#include "fancytabbar.h"
#include "fancytabwidget.h"
#include "fancytabdata.h"

using namespace Qt::Literals::StringLiterals;

namespace {
constexpr int TabSize_LargeSidebarMinWidth = 70;
}

FancyTabBar::FancyTabBar(QWidget *parent)
    : QTabBar(parent),
      mouseHoverTabIndex(-1) {
  setMouseTracking(true);

  QTimer::singleShot(500, [](){
    QString qss = QStringLiteral(R"(
      QHeaderView::section {
        background-color: #0A0A0A;
        color: #6E6E73;
        font-family: 'Inter';
        font-size: 9px;
        font-weight: 600;
        letter-spacing: 0.8px;
        border: none;
        border-bottom: 1px solid #1C1C1E;
        padding: 8px 10px;
      }
      QHeaderView::section:hover {
        color: #A1A1A6;
        background-color: #141416;
      }
      QTableView::item:selected {
        background-color: #1C1C1E;
        color: #F5F5F7;
        border-left: 2px solid #C1272D;
      }
      QTableView::item {
        padding: 6px 10px;
        border-bottom: 1px solid #111113;
      }
      QTableView {
        gridline-color: transparent;
      }
    )");
    qApp->setStyleSheet(qApp->styleSheet() + qss);
  });
}

QString FancyTabBar::TabText(const int index) const {
  return tabText(index).isEmpty() || !tabData(index).isValid() ? ""_L1 : tabData(index).value<FancyTabData*>()->label();
}

QSize FancyTabBar::sizeHint() const {
  FancyTabWidget *tabWidget = qobject_cast<FancyTabWidget*>(parentWidget());
  if (!tabWidget || tabWidget->mode() == FancyTabWidget::Mode::Tabs || tabWidget->mode() == FancyTabWidget::Mode::IconOnlyTabs) {
    return QTabBar::sizeHint();
  }
  QSize size;
  int h = 0;
  for (int i = 0; i < count(); ++i) {
    if (tabSizeHint(i).width() > size.width()) size.setWidth(tabSizeHint(i).width());
    h += tabSizeHint(i).height();
  }
  size.setHeight(h);
  return size;
}

int FancyTabBar::width() const {
  FancyTabWidget *tabWidget = qobject_cast<FancyTabWidget*>(parentWidget());
  if (tabWidget && (tabWidget->mode() == FancyTabWidget::Mode::LargeSidebar || tabWidget->mode() == FancyTabWidget::Mode::SmallSidebar)) {
    int w = 0;
    for (int i = 0; i < count(); ++i) {
      if (tabSizeHint(i).width() > w) w = tabSizeHint(i).width();
    }
    return w;
  }
  else {
    return QTabBar::width();
  }
}

QSize FancyTabBar::tabSizeHint(const int index) const {
  FancyTabWidget *tabWidget = qobject_cast<FancyTabWidget*>(parentWidget());
  if (!tabWidget) return QTabBar::tabSizeHint(index);
  QSize size;
  if (tabWidget->mode() == FancyTabWidget::Mode::LargeSidebar) {
    QFont bold_font(font());
    bold_font.setBold(true);
    QFontMetrics fm(bold_font);
    int w = std::max(TabSize_LargeSidebarMinWidth, tabWidget->iconsize_largesidebar() + 22);
    for (int i = 0; i < count(); ++i) {
      QRect rect = fm.boundingRect(QRect(0, 0, std::max(TabSize_LargeSidebarMinWidth, tabWidget->iconsize_largesidebar() + 22), height()), Qt::TextWordWrap, TabText(i));
      rect.setWidth(rect.width() + 10);
      if (rect.width() > w) w = rect.width();
    }
    QRect rect = fm.boundingRect(QRect(0, 0, w, height()), Qt::TextWordWrap, TabText(index));
    size = QSize(w, tabWidget->iconsize_largesidebar() + rect.height() + 10);
  }
  else if (tabWidget->mode() == FancyTabWidget::Mode::IconsSidebar) {
    size = QSize(tabWidget->iconsize_largesidebar() + 20, tabWidget->iconsize_largesidebar() + 20);
  }
  else if (tabWidget->mode() == FancyTabWidget::Mode::SmallSidebar) {
    QFont bold_font(font());
    bold_font.setBold(true);
    QFontMetrics fm(bold_font);
    QRect rect = fm.boundingRect(QRect(0, 0, 100, tabWidget->height()), Qt::AlignHCenter, TabText(index));
    int w = std::max(tabWidget->iconsize_smallsidebar(), rect.height()) + 15;
    int h = tabWidget->iconsize_smallsidebar() + rect.width() + 20;
    size = QSize(w, h);
  }
  else {
    size = QTabBar::tabSizeHint(index);
  }
  return size;
}

void FancyTabBar::leaveEvent(QEvent *event) {
  Q_UNUSED(event);
  mouseHoverTabIndex = -1;
  update();
}

void FancyTabBar::mouseMoveEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  mouseHoverTabIndex = tabAt(pos);
  if (mouseHoverTabIndex > -1) {
    update();
  }
  QTabBar::mouseMoveEvent(event);
}

void FancyTabBar::paintEvent(QPaintEvent *pe) {
  FancyTabWidget *tabWidget = qobject_cast<FancyTabWidget*>(parentWidget());
  if (!tabWidget ||
      (tabWidget->mode() != FancyTabWidget::Mode::LargeSidebar &&
      tabWidget->mode() != FancyTabWidget::Mode::SmallSidebar &&
      tabWidget->mode() != FancyTabWidget::Mode::IconsSidebar)) {
    QTabBar::paintEvent(pe);
    return;
  }
  const bool vertical_text_tabs = tabWidget->mode() == FancyTabWidget::Mode::SmallSidebar;
  QStylePainter p(this);
  for (int index = 0; index < count(); ++index) {
    const bool selected = tabWidget->currentIndex() == index;
    const bool hover = (index == mouseHoverTabIndex);
    QRect tabrect = tabRect(index);
    if (selected) {
      p.fillRect(tabrect, QColor(0x14, 0x14, 0x16));
      QRect lineRect = tabrect;
      lineRect.setWidth(3);
      p.fillRect(lineRect, QColor(0xC1, 0x27, 0x2D));
      p.setPen(QColor(0x23, 0x23, 0x26));
      p.drawLine(tabrect.bottomLeft() + QPoint(3,0), tabrect.bottomRight());
    }
    else if (hover && isTabEnabled(index)) {
      p.fillRect(tabrect, QColor(0x1C, 0x1C, 0x1E));
      QRect lineRect = tabrect;
      lineRect.setWidth(3);
      p.fillRect(lineRect, QColor(0x2C, 0x2C, 0x2E));
    }
    else {
      p.fillRect(tabrect, QColor(0x0F, 0x0F, 0x10));
    }
    {
      p.save();
      QTransform m;
      int textFlags = 0;
      Qt::Alignment iconFlags;
      QRect tabrectText;
      QRect tabrectLabel;
      if (vertical_text_tabs) {
        m = QTransform::fromTranslate(tabrect.left(), tabrect.bottom());
        m.rotate(-90);
        textFlags = Qt::AlignVCenter;
        iconFlags = Qt::AlignVCenter;
        tabrectLabel = QRect(QPoint(0, 0), m.mapRect(tabrect).size());
        tabrectText = tabrectLabel;
        tabrectText.translate(tabWidget->iconsize_smallsidebar() + 8, 0);
      }
      else {
        m = QTransform::fromTranslate(tabrect.left(), tabrect.top());
        textFlags = Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWordWrap;
        iconFlags = Qt::AlignHCenter | Qt::AlignTop;
        tabrectLabel = QRect(QPoint(0, 0), m.mapRect(tabrect).size());
        tabrectText = tabrectLabel;
        tabrectText.translate(0, -5);
      }
      p.setTransform(m);
      QFont boldFont(p.font());
      boldFont.setBold(selected);
      boldFont.setPointSizeF(boldFont.pointSizeF() * (selected ? 1.0 : 0.95));
      p.setFont(boldFont);
      if (selected) {
        p.setPen(QColor(0xF5, 0xF5, 0xF7));
      } else {
        p.setPen(QColor(0x8E, 0x8E, 0x93));
      }
      p.drawText(tabrectText, textFlags, TabText(index));
      QRect tabrectIcon;
      if (vertical_text_tabs) {
        tabrectIcon = tabrectLabel;
        tabrectIcon.setSize(QSize(tabWidget->iconsize_smallsidebar(), tabWidget->iconsize_smallsidebar()));
        const int moveRight = (QTabBar::width() - tabWidget->iconsize_smallsidebar()) / 2;
        tabrectIcon.translate(5, moveRight);
      }
      else {
        tabrectIcon = tabrectLabel;
        tabrectIcon.setSize(QSize(tabWidget->iconsize_largesidebar(), tabWidget->iconsize_largesidebar()));
        const int moveRight = (QTabBar::width() - tabWidget->iconsize_largesidebar() - 1) / 2;
        if (tabWidget->mode() == FancyTabWidget::Mode::IconsSidebar) {
          tabrectIcon.translate(moveRight, (tabSizeHint(0).height() - tabWidget->iconsize_largesidebar() - 5) / 2);
        }
        else {
          tabrectIcon.translate(moveRight, 5);
        }
      }
      p.setOpacity(selected ? 1.0 : 0.7);
      tabIcon(index).paint(&p, tabrectIcon, iconFlags);
      p.restore();
    }
  }
}
