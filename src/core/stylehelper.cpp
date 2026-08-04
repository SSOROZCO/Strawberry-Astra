/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "stylehelper.h"

#include <QPixmapCache>
#include <QPainter>
#include <QApplication>
#include <QFileInfo>
#include <QCommonStyle>
#include <QStyleOption>
#include <QWindow>
#include <qmath.h>

using namespace Qt::Literals::StringLiterals;

static int clamp(float x) {
  const int val = x > 255 ? 255 : static_cast<int>(x);
  return val < 0 ? 0 : val;
}

namespace Utils {

// Astra SE - Forced Dark Roon Palette
// Rojo Roscosmos #C1272D - Fondo #101114
QColor StyleHelper::m_baseColor = QColor(0x10, 0x11, 0x14);
QColor StyleHelper::m_requestedBaseColor = QColor(0xC1, 0x27, 0x2D);
QColor StyleHelper::m_IconsBaseColor = 0xffC1272D; // Era #dcdcdc -> ahora rojo Astra
QColor StyleHelper::m_IconsDisabledColor = 0x88a0a0a0;
QColor StyleHelper::m_ProgressBarTitleColor = 0xffE7E8EC;

QColor StyleHelper::mergedColors(const QColor &colorA, const QColor &colorB, int factor) {
  const int maxFactor = 100;
  QColor tmp = colorA;
  tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
  tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
  tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
  return tmp;
}

QColor StyleHelper::alphaBlendedColors(const QColor &colorA, const QColor &colorB) {
  const int alpha = colorB.alpha();
  const int antiAlpha = 255 - alpha;
  return QColor(
               (colorA.red() * antiAlpha + colorB.red() * alpha) / 255,
               (colorA.green() * antiAlpha + colorB.green() * alpha) / 255,
               (colorA.blue() * antiAlpha + colorB.blue() * alpha) / 255
               );
}

qreal StyleHelper::sidebarFontSize() {
#ifdef Q_OS_MACOS
  return 10;
#else
  return 7.5;
#endif
}

QColor StyleHelper::notTooBrightHighlightColor() {
  // Astra SE: siempre rojo Roscosmos, nunca celeste
  return QColor(0xC1, 0x27, 0x2D);
}

QPalette StyleHelper::sidebarFontPalette(const QPalette &original) {
  QPalette palette = original;
  const QColor textColor = m_ProgressBarTitleColor;
  palette.setColor(QPalette::WindowText, textColor);
  palette.setColor(QPalette::Text, textColor);
  return palette;
}

QColor StyleHelper::panelTextColor(bool lightColored) {
  Q_UNUSED(lightColored)
  return QColor(0xE7, 0xE8, 0xEC); // Siempre claro sobre fondo oscuro
}

QColor StyleHelper::baseColor(bool lightColored) {
  Q_UNUSED(lightColored)
  return QColor(0x10, 0x11, 0x14); // #101114 forzado oscuro
}

QColor StyleHelper::highlightColor(bool lightColored) {
  Q_UNUSED(lightColored)
  return QColor(0xC1, 0x27, 0x2D); // #C1272D rojo Astra - era el que calculaba celeste
}

QColor StyleHelper::shadowColor(bool lightColored) {
  Q_UNUSED(lightColored)
  return QColor(0x0A, 0x0A, 0x0C);
}

QColor StyleHelper::borderColor(bool lightColored) {
  Q_UNUSED(lightColored)
  return QColor(0x2A, 0x2B, 0x31);
}

QColor StyleHelper::toolBarBorderColor() {
  return QColor(0x2A, 0x2B, 0x31);
}

void StyleHelper::setBaseColor(const QColor &newcolor) {
  Q_UNUSED(newcolor)
  // Bloqueamos cualquier cambio de color que pida el sistema - Astra siempre oscuro
  m_requestedBaseColor = QColor(0xC1, 0x27, 0x2D);
  m_baseColor = QColor(0x10, 0x11, 0x14);
  const QList<QWidget*> widgets = QApplication::topLevelWidgets();
  for (QWidget *w : widgets) w->update();
}

static void verticalGradientHelper(QPainter *p, const QRect spanRect, const QRect rect, bool lightColored) {
  Q_UNUSED(lightColored)
  p->fillRect(rect, QColor(0x16, 0x16, 0x1A));
}

void StyleHelper::verticalGradient(QPainter *painter, const QRect spanRect, const QRect clipRect, bool lightColored) {
  Q_UNUSED(spanRect) Q_UNUSED(lightColored)
  painter->fillRect(clipRect, QColor(0x16, 0x16, 0x1A));
}

static void horizontalGradientHelper(QPainter *p, const QRect spanRect, const QRect rect, bool lightColored) {
  Q_UNUSED(spanRect) Q_UNUSED(lightColored)
  p->fillRect(rect, QColor(0x16, 0x16, 0x1A));
}

void StyleHelper::horizontalGradient(QPainter *painter, const QRect spanRect, const QRect clipRect, bool lightColored) {
  Q_UNUSED(spanRect) Q_UNUSED(lightColored)
  painter->fillRect(clipRect, QColor(0x16, 0x16, 0x1A));
}

static void menuGradientHelper(QPainter *p, const QRect spanRect, const QRect rect) {
  Q_UNUSED(spanRect)
  p->fillRect(rect, QColor(0x10, 0x11, 0x14));
}

void StyleHelper::drawArrow(QStyle::PrimitiveElement element, QPainter *painter, const QStyleOption *option) {
  if (option->rect.width() <= 1 || option->rect.height() <= 1) return;
  const qreal devicePixelRatio = painter->device()->devicePixelRatio();
  const bool enabled = option->state & QStyle::State_Enabled;
  QRect r = option->rect;
  int size = qMin(r.height(), r.width());
  QPixmap pixmap;
  QString pixmapName = QString::asprintf("StyleHelper::drawArrow-%d-%d-%d-%f", element, size, (enabled ? 1 : 0), devicePixelRatio);
  if (!QPixmapCache::find(pixmapName, &pixmap)) {
    QImage image(size * static_cast<int>(devicePixelRatio), size * static_cast<int>(devicePixelRatio), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter p(&image);
    QStyleOption tweakedOption(*option);
    tweakedOption.state = QStyle::State_Enabled;
    auto drawCommonStyleArrow = [&tweakedOption, element, &p](const QRect rect, const QColor &color) -> void
    {
      static const QCommonStyle* const style = qobject_cast<QCommonStyle*>(QApplication::style());
      if (!style) return;
      tweakedOption.palette.setColor(QPalette::ButtonText, color.rgb());
      tweakedOption.rect = rect;
      p.setOpacity(color.alphaF());
      style->QCommonStyle::drawPrimitive(element, &tweakedOption, &p);
    };
    if (!enabled) drawCommonStyleArrow(image.rect(), m_IconsDisabledColor);
    else {
      drawCommonStyleArrow(image.rect().translated(0, static_cast<int>(devicePixelRatio)), toolBarDropShadowColor());
      drawCommonStyleArrow(image.rect(), m_IconsBaseColor);
    }
    p.end();
    pixmap = QPixmap::fromImage(image);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    QPixmapCache::insert(pixmapName, pixmap);
  }
  int xOffset = r.x() + (r.width() - size) / 2;
  int yOffset = r.y() + (r.height() - size) / 2;
  painter->drawPixmap(xOffset, yOffset, pixmap);
}

void StyleHelper::menuGradient(QPainter *painter, const QRect spanRect, const QRect clipRect) {
  painter->fillRect(clipRect, QColor(0x10, 0x11, 0x14));
}

QPixmap StyleHelper::disabledSideBarIcon(const QPixmap &enabledicon) {
  QImage im = enabledicon.toImage().convertToFormat(QImage::Format_ARGB32);
  for (int y = 0; y < im.height(); ++y) {
    auto scanLine = reinterpret_cast<QRgb*>(im.scanLine(y));
    for (int x = 0; x < im.width(); ++x) {
      QRgb pixel = *scanLine;
      char intensity = static_cast<char>(qGray(pixel));
      *scanLine = qRgba(intensity, intensity, intensity, qAlpha(pixel));
      ++scanLine;
    }
  }
  return QPixmap::fromImage(im);
}

void StyleHelper::drawCornerImage(const QImage &img, QPainter *painter, const QRect rect, int left, int top, int right, int bottom) {
  const qreal imagePixelRatio = img.devicePixelRatio();
  const qreal leftDIP = left * imagePixelRatio;
  const qreal topDIP = top * imagePixelRatio;
  const qreal rightDIP = right * imagePixelRatio;
  const qreal bottomDIP = bottom * imagePixelRatio;
  const QSize size = img.size();
  if (top > 0) {
    painter->drawImage(QRectF(rect.left() + left, rect.top(), rect.width() - right - left, top), img, QRectF(leftDIP, 0, size.width() - rightDIP - leftDIP, topDIP));
    if (left > 0) painter->drawImage(QRectF(rect.left(), rect.top(), left, top), img, QRectF(0, 0, leftDIP, topDIP));
    if (right > 0) painter->drawImage(QRectF(rect.left() + rect.width() - right, rect.top(), right, top), img, QRectF(size.width() - rightDIP, 0, rightDIP, topDIP));
  }
  if (left > 0) painter->drawImage(QRectF(rect.left(), rect.top() + top, left, rect.height() - top - bottom), img, QRectF(0, topDIP, leftDIP, size.height() - bottomDIP - topDIP));
  painter->drawImage(QRectF(rect.left() + left, rect.top() + top, rect.width() - right - left, rect.height() - bottom - top), img, QRectF(leftDIP, topDIP, size.width() - rightDIP - leftDIP, size.height() - bottomDIP - topDIP));
  if (right > 0) painter->drawImage(QRectF(rect.left() + rect.width() - right, rect.top() + top, right, rect.height() - top - bottom), img, QRectF(size.width() - rightDIP, topDIP, rightDIP, size.height() - bottomDIP - topDIP));
  if (bottom > 0) {
    painter->drawImage(QRectF(rect.left() + left, rect.top() + rect.height() - bottom, rect.width() - right - left, bottom), img, QRectF(leftDIP, size.height() - bottomDIP, size.width() - rightDIP - leftDIP, bottomDIP));
    if (left > 0) painter->drawImage(QRectF(rect.left(), rect.top() + rect.height() - bottom, left, bottom), img, QRectF(0, size.height() - bottomDIP, leftDIP, bottomDIP));
    if (right > 0) painter->drawImage(QRectF(rect.left() + rect.width() - right, rect.top() + rect.height() - bottom, right, bottom), img, QRectF(size.width() - rightDIP, size.height() - bottomDIP, rightDIP, bottomDIP));
  }
}

void StyleHelper::tintImage(QImage &img, const QColor &tintColor) {
  QPainter p(&img);
  p.setCompositionMode(QPainter::CompositionMode_Screen);
  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      QRgb rgbColor = img.pixel(x, y);
      int alpha = qAlpha(rgbColor);
      QColor c = QColor(rgbColor);
      if (alpha > 0) {
        float l = c.lightnessF();
        QColor newColor = QColor::fromHslF(tintColor.hslHueF(), tintColor.hslSaturationF(), l);
        newColor.setAlpha(alpha);
        img.setPixel(x, y, newColor.rgba());
      }
    }
  }
}

QLinearGradient StyleHelper::statusBarGradient(const QRect statusBarRect) {
  QLinearGradient grad(statusBarRect.topLeft(), QPoint(statusBarRect.center().x(), statusBarRect.bottom()));
  grad.setColorAt(0, QColor(0x10, 0x11, 0x14));
  grad.setColorAt(1, QColor(0x10, 0x11, 0x14));
  return grad;
}

QString StyleHelper::dpiSpecificImageFile(const QString &fileName) {
  if (qApp->devicePixelRatio() > 1.0) {
    const QString atDprfileName = imageFileWithResolution(fileName, qRound(qApp->devicePixelRatio()));
    if (QFile::exists(atDprfileName)) return atDprfileName;
  }
  return fileName;
}

QString StyleHelper::imageFileWithResolution(const QString &fileName, int dpr) {
  const QFileInfo fi(fileName);
  return dpr == 1 ? fileName : fi.path() + QLatin1Char('/') + fi.completeBaseName() + QLatin1Char('@') + QString::number(dpr) + "x."_L1 + fi.suffix();
}

QList<int> StyleHelper::availableImageResolutions(const QString &fileName) {
  QList<int> result;
  const int maxResolutions = qApp->devicePixelRatio();
  for (int i = 1; i <= maxResolutions; ++i) {
    if (QFile::exists(imageFileWithResolution(fileName, i))) result.append(i);
  }
  return result;
}

}  // namespace Utils
