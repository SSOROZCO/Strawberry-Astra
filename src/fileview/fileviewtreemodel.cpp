/*
 * Strawberry Music Player - Strawberry-Roon Fork
 * Copyright 2025, Jonas Kvinge <jonas@jkvinge.net>
 * Copyright 2026, Sebastián S. Orozco - Strawberry-Roon / Astra Linux SE Mod (Custom Roon Folder Icons)
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

#include <QObject>
#include <QVariant>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QDir>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QMimeData>
#include <QUrl>
#include <QIcon>

#include "core/simpletreemodel.h"
#include "core/logging.h"
#include "fileviewtreemodel.h"
#include "fileviewtreeitem.h"

using namespace Qt::Literals::StringLiterals;

FileViewTreeModel::FileViewTreeModel(QObject *parent)
    : SimpleTreeModel<FileViewTreeItem>(new FileViewTreeItem(this), parent),
      icon_provider_(new QFileIconProvider()) {
}

FileViewTreeModel::~FileViewTreeModel() {
  delete root_;
  delete icon_provider_;
}

Qt::ItemFlags FileViewTreeModel::flags(const QModelIndex &idx) const {

  const FileViewTreeItem *item = IndexToItem(idx);
  if (!item) return Qt::NoItemFlags;

  switch (item->type) {
    case FileViewTreeItem::Type::VirtualRoot:
    case FileViewTreeItem::Type::Directory:
    case FileViewTreeItem::Type::File:
      return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;
    case FileViewTreeItem::Type::Root:
    default:
      return Qt::ItemIsEnabled;
  }

}

QVariant FileViewTreeModel::data(const QModelIndex &idx, const int role) const {

  if (!idx.isValid()) return QVariant();

  const FileViewTreeItem *item = IndexToItem(idx);
  if (!item) return QVariant();

  switch (role) {
    case Qt::DisplayRole:
      if (item->type == FileViewTreeItem::Type::VirtualRoot) {
        return item->display_text.isEmpty() ? item->file_path : item->display_text;
      }
      return item->file_info.fileName();

    case Qt::DecorationRole:
      return GetIcon(item);

    case Role_Type:
      return QVariant::fromValue(item->type);

    case Role_FilePath:
      return item->file_path;

    case Role_FileName:
      return item->file_info.fileName();

    default:
      return QVariant();
  }

}

bool FileViewTreeModel::hasChildren(const QModelIndex &parent) const {

  const FileViewTreeItem *item = IndexToItem(parent);
  if (!item) return false;

  if (item->type == FileViewTreeItem::Type::Root) return true;
  if (item->type == FileViewTreeItem::Type::VirtualRoot) return true;
  if (item->type == FileViewTreeItem::Type::Directory) {
    return true;
  }
  return false;
}

bool FileViewTreeModel::canFetchMore(const QModelIndex &parent) const {

  const FileViewTreeItem *item = IndexToItem(parent);
  if (!item) return false;
  return !item->lazy_loaded && (item->type == FileViewTreeItem::Type::VirtualRoot || item->type == FileViewTreeItem::Type::Directory);
}

void FileViewTreeModel::fetchMore(const QModelIndex &parent) {

  FileViewTreeItem *item = IndexToItem(parent);
  if (!item || item->lazy_loaded) return;
  LazyLoad(item);
}

void FileViewTreeModel::LazyLoad(FileViewTreeItem *item) {

  if (item->lazy_loaded) return;

  QDir dir(item->file_path);
  if (!dir.exists()) {
    item->lazy_loaded = true;
    return;
  }

  const QDir::Filters filters = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot;
  if (!name_filters_.isEmpty()) {
    dir.setNameFilters(name_filters_);
  }

  const QFileInfoList entries = dir.entryInfoList(filters, QDir::Name | QDir::DirsFirst);
  if (!entries.isEmpty()) {
    BeginInsert(item, 0, static_cast<int>(entries.count()) - 1);

    for (const QFileInfo &entry : entries) {
      FileViewTreeItem *child = new FileViewTreeItem(
        entry.isDir() ? FileViewTreeItem::Type::Directory : FileViewTreeItem::Type::File,
        item
      );
      child->file_path = entry.absoluteFilePath();
      child->file_info = entry;
      child->lazy_loaded = false;
      child->display_text = entry.fileName();
    }

    EndInsert();
  }

  item->lazy_loaded = true;
}

QIcon FileViewTreeModel::GetIcon(const FileViewTreeItem *item) const {

  if (!item) return QIcon();

  // --- ASTRA LINUX SE MOD - ROON STYLE ICONS ---
  static QIcon icon_roon_folder;
  static bool icons_loaded = false;
  if (!icons_loaded) {
    // Cargamos el icono desde los recursos, si no existe usamos fallback oscuro
    icon_roon_folder = QIcon(u":/pictures/roon-folder.png"_s);
    if (icon_roon_folder.isNull()) {
      // Fallback: si no pusiste el png, usa uno del tema pero tuneado
      icon_roon_folder = QIcon::fromTheme(u"folder"_s);
    }
    icons_loaded = true;
  }
  // ---------------------------------------------

  switch (item->type) {
    case FileViewTreeItem::Type::VirtualRoot:
    case FileViewTreeItem::Type::Directory:
      return icon_roon_folder;
    case FileViewTreeItem::Type::File:
      return icon_provider_->icon(item->file_info);
    default:
      return QIcon();
  }

}

QStringList FileViewTreeModel::mimeTypes() const {
  return QStringList() << u"text/uri-list"_s;
}

QMimeData *FileViewTreeModel::mimeData(const QModelIndexList &indexes) const {

  if (indexes.isEmpty()) return nullptr;

  QList<QUrl> urls;
  for (const QModelIndex &idx : indexes) {
    const FileViewTreeItem *item = IndexToItem(idx);
    if (item && (item->type == FileViewTreeItem::Type::File || item->type == FileViewTreeItem::Type::Directory || item->type == FileViewTreeItem::Type::VirtualRoot)) {
      urls << QUrl::fromLocalFile(item->file_path);
    }
  }

  if (urls.isEmpty()) return nullptr;

  QMimeData *data = new QMimeData();
  data->setUrls(urls);
  return data;

}

void FileViewTreeModel::SetRootPaths(const QStringList &paths) {

  Reset();

  for (const QString &path : paths) {
    QFileInfo info(path);
    if (!info.exists() || !info.isDir()) continue;

    FileViewTreeItem *virtual_root = new FileViewTreeItem(FileViewTreeItem::Type::VirtualRoot, root_);
    virtual_root->file_path = info.absoluteFilePath();
    virtual_root->file_info = info;
    virtual_root->display_text = info.absoluteFilePath();
    virtual_root->lazy_loaded = false;
  }

}

void FileViewTreeModel::SetNameFilters(const QStringList &filters) {
  name_filters_ = filters;
}

void FileViewTreeModel::Reset() {

  beginResetModel();
  qDeleteAll(root_->children);
  root_->children.clear();
  endResetModel();

}
