/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 * Copyright 2018-2026, Jonas Kvinge <jonas@jkvinge.net>
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
#include "version.h"

#include <QtGlobal>

#include <cstdlib>
#include <ctime>
#include <memory>

#ifdef Q_OS_UNIX
#  include <unistd.h>
#endif

#ifdef Q_OS_MACOS
#  include <sys/resource.h>
#  include <sys/sysctl.h>
#endif

#ifdef Q_OS_WIN32
#  include <windows.h>
#  include <iostream>
#endif  // Q_OS_WIN32

#include <glib.h>

#include <QObject>
#include <QApplication>
#include <QCoreApplication>
#include <QSysInfo>
#include <QLibraryInfo>
#include <QFileDevice>
#include <QIODevice>
#include <QByteArray>
#include <QNetworkProxy>
#include <QFile>
#include <QDir>
#include <QString>
#include <QSettings>
#include <QLoggingCategory>
#include <QStyle>
#include <QStyleHints>
#include <QMessageBox>
#ifdef HAVE_TRANSLATIONS
#  include <QTranslator>
#endif

#include "main.h"

#include "includes/scoped_ptr.h"
#include "includes/shared_ptr.h"

#include "core/logging.h"
#include "core/standardpaths.h"
#include "core/settings.h"

#include "utilities/envutils.h"
#include "utilities/styleutils.h"

#include <kdsingleapplication.h>

#ifdef Q_OS_UNIX
  #include "core/unixsignalwatcher.h"
#endif

#ifdef HAVE_QTSPARKLE
#  include <qtsparkle-qt6/Updater>
#endif  // HAVE_QTSPARKLE

#ifdef Q_OS_MACOS
#  include "utilities/macosutils.h"
#  include "core/mac_startup.h"
#endif

#ifdef HAVE_MPRIS2
#  include "mpris2/mpris2.h"
#endif

#ifdef HAVE_DISCORD_RPC
#  include "discord/discordrichpresence.h"
#endif

#include "core/iconloader.h"
#include "core/commandlineoptions.h"
#include "core/networkproxyfactory.h"

#include "core/application.h"
#include "core/metatypes.h"
#include "core/mainwindow.h"

#ifdef Q_OS_MACOS
#  include "systemtrayicon/macsystemtrayicon.h"
#else
#  include "systemtrayicon/qtsystemtrayicon.h"
#endif

#ifdef HAVE_TRANSLATIONS
#  include "core/translations.h"
#endif

#include "constants/behavioursettings.h"
#include "constants/appearancesettings.h"

#if defined(Q_OS_MACOS)
#  include "osd/osdmac.h"
#elif defined(HAVE_DBUS)
#  include "osd/osddbus.h"
#else
#  include "osd/osdbase.h"
#endif

#include "engine/gststartup.h"

using namespace Qt::Literals::StringLiterals;
using std::make_shared;

int main(int argc, char *argv[]) {

#ifdef Q_OS_MACOS
  // Do Mac specific startup to get media keys working.
  // This must go before QApplication initialization.
  mac::MacMain();
#endif

  QCoreApplication::setApplicationName(u"Strawberry"_s);
  QCoreApplication::setOrganizationName(u"Strawberry"_s);
  QCoreApplication::setApplicationVersion(QStringLiteral(STRAWBERRY_VERSION_DISPLAY));
  QCoreApplication::setOrganizationDomain(u"strawberrymusicplayer.org"_s);

  // This makes us show up nicely in gnome-volume-control
  g_set_application_name("Strawberry");
  g_setenv("PULSE_PROP_application.icon_name", "strawberry", TRUE);
  g_setenv("PULSE_PROP_media.role", "music", TRUE);

  RegisterMetaTypes();

  // Initialize logging.  Log levels are set after the commandline options are parsed below.
  logging::Init();
  g_log_set_default_handler(reinterpret_cast<GLogFunc>(&logging::GLog), nullptr);

  CommandlineOptions options(argc, argv);
  {
    // Only start a core application now, so we can check if there's another instance without requiring an X server.
    // This MUST be done before parsing the commandline options so QTextCodec gets the right system locale for filenames.
    QCoreApplication core_app(argc, argv);
    KDSingleApplication single_app(QCoreApplication::applicationName().toLower(), KDSingleApplication::Option::IncludeUsernameInSocketName);
    // Parse commandline options - need to do this before starting the full QApplication, so it works without an X server
    if (!options.Parse()) return 1;
    logging::SetLevels(options.log_levels());
    if (!single_app.isPrimaryInstance()) {
      if (options.is_empty()) {
        qLog(Info) << "Strawberry is already running - activating existing window (1)";
      }
      if (!single_app.sendMessage(options.Serialize())) {
        qLog(Error) << "Could not send message to primary instance.";
      }
      return 0;
    }
  }

#ifdef Q_OS_MACOS
  // Must happen after QCoreApplication::setOrganizationName().
  Utilities::SetEnv("XDG_CONFIG_HOME", StandardPaths::WritableLocation(StandardPaths::StandardLocation::AppConfigLocation));
#endif

  // Output the version, so when people attach log output to bug reports they don't have to tell us which version they're using.
  qLog(Info) << "Strawberry" << STRAWBERRY_VERSION_DISPLAY << "Qt" << QLibraryInfo::version().toString();
  qLog(Info) << QStringLiteral("%1 %2 - (%3 %4) [%5]").arg(QSysInfo::prettyProductName(), QSysInfo::productVersion(), QSysInfo::kernelType(), QSysInfo::kernelVersion(), QSysInfo::currentCpuArchitecture());

  // Seed the random number generators.
  time_t t = time(nullptr);
  srand(static_cast<uint>(t));

#ifdef Q_OS_MACOS
  Utilities::IncreaseFDLimit();
#endif

  QGuiApplication::setApplicationDisplayName(u"Strawberry-Roon"_s);
  QGuiApplication::setDesktopFileName(u"org.strawberrymusicplayer.strawberry"_s);
  QGuiApplication::setQuitOnLastWindowClosed(false);

  QApplication a(argc, argv);

#ifdef Q_OS_LINUX
  if (Utilities::IsWSL()) {
    const QString message = u"Strawberry is not supported when running under the Windows Subsystem for Linux (WSL). Please use the native Windows version instead."_s;
    qLog(Error) << message;
    QMessageBox::critical(nullptr, u"Unsupported environment"_s, message);
    return 1;
  }
#endif

  KDSingleApplication single_app(QCoreApplication::applicationName().toLower(), KDSingleApplication::Option::IncludeUsernameInSocketName);
  if (!single_app.isPrimaryInstance()) {
    if (options.is_empty()) {
      qLog(Info) << "Strawberry is already running - activating existing window (2)";
    }
    if (!single_app.sendMessage(options.Serialize())) {
      qLog(Error) << "Could not send message to primary instance.";
    }
    return 0;
  }

  QThread::currentThread()->setObjectName(u"Main"_s);

  if (QGuiApplication::platformName() != "wayland"_L1) {
    QGuiApplication::setWindowIcon(IconLoader::Load(u"strawberry"_s));
  }

#if defined(USE_BUNDLE)
  qLog(Debug) << "Looking for resources in" << QCoreApplication::libraryPaths();
#endif

  GstStartup::Initialize();

  // Gnome on Ubuntu has menu icons disabled by default.  I think that's a bad idea, and makes some menus in Strawberry look confusing.
  QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus, false);

  const QString default_style = QApplication::style() ? QApplication::style()->objectName() : QString();
  {
    Settings s;
    s.beginGroup(AppearanceSettings::kSettingsGroup);
    const QString style_name = s.value(AppearanceSettings::kStyle).toString();
    const bool dark_mode = s.value(AppearanceSettings::kDarkMode, false).toBool();
    s.endGroup();
    if (!style_name.isEmpty() && style_name.compare("default"_L1, Qt::CaseInsensitive) != 0) {
      if (!QApplication::setStyle(style_name)) {
        qLog(Error) << "Could not set style" << style_name << "- falling back to default style" << default_style;
        if (!QApplication::setStyle(default_style)) {
          qLog(Error) << "Could not set default style" << default_style;
        }
      }
    }
    if (dark_mode && QApplication::style()) {
      const QString current_style = QApplication::style() ? QApplication::style()->objectName() : QString();
      const bool dark_mode_supported = Utilities::StyleHasDarkModeSupport(current_style);
      if (dark_mode_supported) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif
      }
    }
    if (QApplication::style()) {
      qLog(Debug) << "Style:" << QApplication::style()->objectName();
    }
  }

  // Set the permissions on the config file on Unix - it can contain passwords for streaming services, so it's important that other users can't read it.
  // On Windows these are stored in the registry instead.
#ifdef Q_OS_UNIX
  {
    Settings s;
    if (QFile::exists(s.fileName())) {
      if (!QFile::setPermissions(s.fileName(), QFile::ReadOwner | QFile::WriteOwner)) {
        qLog(Error) << "Could not set permissions for settingsfile" << s.fileName();
      }
    }
    else {
      qLog(Error) << "Missing settingsfile" << s.fileName();
    }
  }
#endif

  // Resources
  Q_INIT_RESOURCE(data);
  Q_INIT_RESOURCE(icons);
#if defined(HAVE_TRANSLATIONS) && !defined(INSTALL_TRANSLATIONS)
  Q_INIT_RESOURCE(translations);
#endif

  IconLoader::Init();

#ifdef HAVE_TRANSLATIONS

  QStringList languages;

  // Load language from command line options
  if (!options.language().isEmpty()) {
    languages << options.language();
  }

  // Load language from settings
  if (languages.isEmpty()) {
    Settings s;
    s.beginGroup(BehaviourSettings::kSettingsGroup);
    const QString language = s.value(BehaviourSettings::kLanguage).toString();
    s.endGroup();
    if (!language.isEmpty()) {
      languages << language;
    }
  }

  // Use system UI languages
  if (languages.isEmpty()) {
#  if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    languages = QLocale::system().uiLanguages(QLocale::TagSeparator::Underscore);
#  else
    const QStringList system_languages = QLocale::system().uiLanguages();
    for (const QString &language : system_languages) {
      QString language_underscore = language;
      language_underscore = language_underscore.replace(u'-', u'_');
      languages << language_underscore;
    }
#  endif
  }

  if (languages.isEmpty()) {
    languages << QLocale::system().name();
  }

  ScopedPtr<Translations> translations = std::make_unique<Translations>();

  for (const QString &language : std::as_const(languages)) {
    if (translations->LoadTranslation(u"qt"_s, QLibraryInfo::path(QLibraryInfo::TranslationsPath), language)) {
      break;
    }
  }

  static const QStringList language_paths = QStringList() << u":/i18n"_s
                                                          << QStringLiteral(TRANSLATIONS_DIR)
                                                          << QCoreApplication::applicationDirPath()
                                                          << QDir::currentPath();

  for (const QString &language : std::as_const(languages)) {
    bool language_loaded = false;
    for (const QString &language_path : language_paths) {
      if (translations->LoadTranslation(u"strawberry"_s, language_path, language)) {
        language_loaded = true;
        break;
      }
    }
    if (language_loaded) {
      break;
    }
  }

#  ifdef HAVE_QTSPARKLE
  for (const QString &language : std::as_const(languages)) {
    if (qtsparkle::LoadTranslations(language)) {
      break;
    }
  }
#  endif  // HAVE_QTSPARKLE

#endif  // HAVE_TRANSLATIONS

   Application app;

  // Astra SE Roon - FASE 7 Final Unificada
  QFile qssFile(QStringLiteral(":/roon.qss"));
  QString roonQss;
  if (qssFile.open(QFile::ReadOnly)) {
    roonQss = QString::fromUtf8(qssFile.readAll());
    qssFile.close();
  }

  QString astraSEStyle = QStringLiteral(R"(
    QMainWindow, QWidget#MainWindow, QFrame { background-color: #101114; color: #E7E8EC; }
    /* Sidebar colección - #101114 */
    QTreeView#collection, QTreeView, QListView { background: #101114; alternate-background-color: #16161A; color: #8A8C94; border: none; outline: none; }
    QTreeView::item, QListView::item { padding: 6px; border: none; }
    QTreeView::item:hover, QListView::item:hover { background: rgba(193,39,45,0.12); color: #E7E8EC; }
    QTreeView::item:selected, QListView::item:selected { background: #C1272D; color: #FFFFFF; }
    QTreeView::item:selected:active { background: #C1272D; }
    /* Header playlist - FASE 5 */
    QHeaderView::section { background: #16161A; color: #8A8C94; border: none; border-bottom: 1px solid #2A2B31; border-right: 1px solid #1E1E24; padding: 8px 10px; font-size: 11px; font-weight: 600; }
    QHeaderView { background: #16161A; }
    /* Tablas playlist - FASE 6 */
    QTableView, QTreeView#playlist { background: #101114; alternate-background-color: #16161A; gridline-color: #1E1E24; }
    /* Botones Play/Pause */
    QToolButton, QPushButton { background: #1E1E24; color: #E7E8EC; border: 1px solid #2A2B31; border-radius: 6px; padding: 5px 12px; }
    QToolButton:hover, QPushButton:hover { background: #2A2B31; border: 1px solid #C1272D; }
    QToolButton:checked, QPushButton:checked { background: #C1272D; color: white; }
    /* Scrollbars Roon */
    QScrollBar:vertical, QScrollBar:horizontal { background: #101114; border: none; width: 8px; height: 8px; }
    QScrollBar::handle { background: #2A2B31; border-radius: 4px; }
    QScrollBar::handle:hover { background: #C1272D; }
    /* Buscador */
    QLineEdit, QTextEdit, QSearchField { background: #16161A; color: #E7E8EC; border: 1px solid #2A2B31; border-radius: 6px; padding: 6px; selection-background-color: #C1272D; }

PlayingWidget { background: #101114; border-top: 1px solid #1E1E24; }
QSlider::groove:horizontal { background: #1E1E24; height: 4px; border-radius: 2px; }
QSlider::handle:horizontal { background: #E7E8EC; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }
QSlider::handle:horizontal:hover { background: #C1272D; }
QSlider::sub-page:horizontal { background: #C1272D; border-radius: 2px; }
QSlider::add-page:horizontal { background: #1E1E24; }
/* FASE 9 - Info card bajo portada estilo Roon */
QFrame#track_info, QFrame#fileinfo, QGroupBox {
  background: #16161A;
  border: 1px solid #1E1E24;
  border-radius: 12px;
  padding: 8px;
}
QLabel { color: #8A8C94; }
QLabel#track_info_label { color: #E7E8EC; }
  )");

  qApp->setStyleSheet(roonQss + QStringLiteral("\n") + astraSEStyle);

  // Network proxy
  QNetworkProxyFactory::setApplicationProxyFactory(NetworkProxyFactory::Instance());

  // Create the tray icon and OSD
  SharedPtr<SystemTrayIcon> tray_icon = make_shared<SystemTrayIcon>();

#if defined(Q_OS_MACOS)
  OSDMac osd(tray_icon, &app);
#elif defined(HAVE_DBUS)
  OSDDBus osd(tray_icon, &app);
#else
  OSDBase osd(tray_icon, &app);
#endif

#ifdef HAVE_MPRIS2
  mpris::Mpris2 mpris2(app.player(), app.playlist_manager(), app.current_albumcover_loader());
#endif
#ifdef HAVE_DISCORD_RPC
  DiscordRichPresence discord_rich_presence(app.player(), app.playlist_manager());
#endif

  // Window
  MainWindow w(&app,
               tray_icon,
               &osd,
#ifdef HAVE_DISCORD_RPC
               &discord_rich_presence,
#endif
               options,
               default_style);

#ifdef Q_OS_UNIX
  UnixSignalWatcher unix_signal_watcher;
  unix_signal_watcher.WatchForSignal(SIGTERM);
  QObject::connect(&unix_signal_watcher, &UnixSignalWatcher::UnixSignal, &w, &MainWindow::Exit);
#endif

#if QT_CONFIG(sessionmanager)
  QObject::connect(&a, &QApplication::commitDataRequest, &w, &MainWindow::CommitData, Qt::DirectConnection);
#endif

#ifdef Q_OS_MACOS
  mac::EnableFullScreen(w);
#endif  // Q_OS_MACOS

#ifdef HAVE_MPRIS2
  QObject::connect(&mpris2, &mpris::Mpris2::RaiseMainWindow, &w, &MainWindow::Raise);
#endif
  QObject::connect(&single_app, &KDSingleApplication::messageReceived, &w, QOverload<const QByteArray&>::of(&MainWindow::CommandlineOptionsReceived));

  int ret = QCoreApplication::exec();

#if defined(__MINGW32__) && !defined(HAVE_WINPTHREADS)
  // Workaround crash on exit with the GCC win32 threading model (not needed with winpthreads).
  TerminateProcess(GetCurrentProcess(), 0);
#endif

  return ret;

}
