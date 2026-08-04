/*
 * Strawberry Music Player
 * Copyright 2026, Jonas Kvinge <jonas@jkvinge.net>
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

#include "config.h"

#include <QApplication>
#include <QObject>
#include <QThread>
#include <QList>
#include <QMap>
#include <QPalette>
#include <QColor>

#include "settings.h"
#include "appearance.h"
#include "constants/appearancesettings.h"

Appearance::Appearance(QObject *parent) : QObject(parent), system_palette_(QApplication::palette()) {}

namespace {

bool IsTextColorRole(const QPalette::ColorRole color_role) {
  return color_role == QPalette::WindowText || color_role == QPalette::Text || color_role == QPalette::ButtonText || color_role == QPalette::BrightText || color_role == QPalette::PlaceholderText || color_role == QPalette::HighlightedText;
}

QPalette::ColorRole BackgroundRoleForTextRole(const QPalette::ColorRole color_role) {
  switch (color_role) {
    case QPalette::Text:
    case QPalette::PlaceholderText:
      return QPalette::Base;
    case QPalette::ButtonText:
      return QPalette::Button;
    case QPalette::HighlightedText:
      return QPalette::Highlight;
    default:
      return QPalette::Window;
  }
}

QColor DimmedTextColor(const QColor &text_color, const QColor &background_color) {
  return QColor((text_color.red() + background_color.red()) / 2, (text_color.green() + background_color.green()) / 2, (text_color.blue() + background_color.blue()) / 2);
}

}  // namespace

const QList<Appearance::ColorRole> &Appearance::ColorRoles() {
  static const QList<ColorRole> color_roles = {
    { QPalette::Window, QLatin1String(AppearanceSettings::kColorWindow) },
    { QPalette::WindowText, QLatin1String(AppearanceSettings::kColorWindowText) },
    { QPalette::Base, QLatin1String(AppearanceSettings::kColorBase) },
    { QPalette::AlternateBase, QLatin1String(AppearanceSettings::kColorAlternateBase) },
    { QPalette::Text, QLatin1String(AppearanceSettings::kColorText) },
    { QPalette::Button, QLatin1String(AppearanceSettings::kColorButton) },
    { QPalette::ButtonText, QLatin1String(AppearanceSettings::kColorButtonText) },
    { QPalette::BrightText, QLatin1String(AppearanceSettings::kColorBrightText) },
    { QPalette::PlaceholderText, QLatin1String(AppearanceSettings::kColorPlaceholderText) },
    { QPalette::ToolTipBase, QLatin1String(AppearanceSettings::kColorToolTipBase) },
    { QPalette::ToolTipText, QLatin1String(AppearanceSettings::kColorToolTipText) }
  };
  return color_roles;
}

const QMap<QPalette::ColorRole, QColor> &Appearance::DarkColors() {
  // Astra SE - Roon Dark + Roscosmos Red
  // Fondo principal #101114, Base #16161A, Texto #E7E8EC, Highlight #C1272D
  static const QMap<QPalette::ColorRole, QColor> dark_colors = {
    { QPalette::Window, QColor(0x10, 0x11, 0x14) },
    { QPalette::WindowText, QColor(0xE7, 0xE8, 0xEC) },
    { QPalette::Base, QColor(0x16, 0x16, 0x1A) },
    { QPalette::AlternateBase, QColor(0x1E, 0x1E, 0x24) },
    { QPalette::Text, QColor(0xE7, 0xE8, 0xEC) },
    { QPalette::Button, QColor(0x10, 0x11, 0x14) },
    { QPalette::ButtonText, QColor(0xE7, 0xE8, 0xEC) },
    { QPalette::BrightText, QColor(0xC1, 0x27, 0x2D) },
    { QPalette::PlaceholderText, QColor(0x8A, 0x8C, 0x94) },
    { QPalette::ToolTipBase, QColor(0x10, 0x11, 0x14) },
    { QPalette::ToolTipText, QColor(0xE7, 0xE8, 0xEC) },
    { QPalette::Highlight, QColor(0xC1, 0x27, 0x2D) },
    { QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF) }
  };
  return dark_colors;
}

void Appearance::LoadCustomPaletteColors() {
  // Astra SE: Forzamos siempre nuestro DarkColors, ignoramos lo que el usuario haya guardado
  // Así mantenemos coherencia y el rojo de ventana que te gusta no se pisa
  SetCustomPaletteColors(DarkColors());
}

void Appearance::SetCustomPaletteColors(const QMap<QPalette::ColorRole, QColor> &colors) {
  Q_ASSERT(QThread::currentThread() == qApp->thread());
  QPalette palette = QApplication::palette();
  for (auto it = colors.constBegin(); it != colors.constEnd(); ++it) {
    if (it.value().isValid()) {
      palette.setColor(QPalette::Active, it.key(), it.value());
      palette.setColor(QPalette::Inactive, it.key(), it.value());
      if (!IsTextColorRole(it.key())) {
        palette.setColor(QPalette::Disabled, it.key(), it.value());
      }
    }
  }
  // Forzamos Highlight rojo Astra siempre
  palette.setColor(QPalette::Active, QPalette::Highlight, QColor(0xC1, 0x27, 0x2D));
  palette.setColor(QPalette::Inactive, QPalette::Highlight, QColor(0xC1, 0x27, 0x2D));
  palette.setColor(QPalette::Active, QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));
  palette.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));

  for (auto it = colors.constBegin(); it != colors.constEnd(); ++it) {
    if (it.value().isValid() && IsTextColorRole(it.key())) {
      const QColor background_color = palette.color(QPalette::Disabled, BackgroundRoleForTextRole(it.key()));
      palette.setColor(QPalette::Disabled, it.key(), DimmedTextColor(it.value(), background_color));
    }
  }
  QApplication::setPalette(palette);
}
