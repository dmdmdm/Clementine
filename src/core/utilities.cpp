/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "utilities.h"

#include <QtGlobal>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QIODevice>
#include <QStringList>
#include <QTemporaryFile>

#if defined(Q_OS_UNIX)
#  include <sys/statvfs.h>
#elif defined(Q_OS_WIN32)
#  include <windows.h>
#endif

#ifdef Q_OS_DARWIN
#  include "core/mac_startup.h"
#endif

#include <boost/scoped_array.hpp>

namespace Utilities {

static QString tr(const char* str) {
  return QCoreApplication::translate("", str);
}

QString PrettyTime(int seconds) {
  // last.fm sometimes gets the track length wrong, so you end up with
  // negative times.
  seconds = qAbs(seconds);

  int hours = seconds / (60*60);
  int minutes = (seconds / 60) % 60;
  seconds %= 60;

  QString ret;
  if (hours)
    ret.sprintf("%d:%02d:%02d", hours, minutes, seconds);
  else
    ret.sprintf("%d:%02d", minutes, seconds);

  return ret;
}

QString WordyTime(quint64 seconds) {
  quint64 days = seconds / (60*60*24);

  // TODO: Make the plural rules translatable
  QStringList parts;

  if (days)
    parts << (days == 1 ? tr("1 day") : tr("%1 days").arg(days));
  parts << PrettyTime(seconds - days*60*60*24);

  return parts.join(" ");
}

QString Ago(int seconds_since_epoch, const QLocale& locale) {
  const QDateTime now = QDateTime::currentDateTime();
  const QDateTime then = QDateTime::fromTime_t(seconds_since_epoch);
  const int days_ago = then.date().daysTo(now.date());
  const QString time = then.time().toString(locale.timeFormat(QLocale::ShortFormat));

  if (days_ago == 0)
    return tr("Today") + " " + time;
  if (days_ago == 1)
    return tr("Yesterday") + " " + time;
  if (days_ago <= 7)
    return tr("%1 days ago").arg(days_ago);

  return then.date().toString(locale.dateTimeFormat());
}

QString PrettySize(quint64 bytes) {
  QString ret;

  if (bytes > 0) {
    if (bytes <= 1000)
      ret = QString::number(bytes) + " bytes";
    else if (bytes <= 1000*1000)
      ret.sprintf("%.1f KB", float(bytes) / 1000);
    else if (bytes <= 1000*1000*1000)
      ret.sprintf("%.1f MB", float(bytes) / (1000*1000));
    else
      ret.sprintf("%.1f GB", float(bytes) / (1000*1000*1000));
  }
  return ret;
}

quint64 FileSystemCapacity(const QString& path) {
#if defined(Q_OS_UNIX)
  struct statvfs fs_info;
  if (statvfs(path.toLocal8Bit().constData(), &fs_info) == 0)
    return quint64(fs_info.f_blocks) * quint64(fs_info.f_bsize);
#elif defined(Q_OS_WIN32)
  _ULARGE_INTEGER ret;
  if (GetDiskFreeSpaceEx(QDir::toNativeSeparators(path).toLocal8Bit().constData(),
                         NULL, &ret, NULL) != 0)
    return ret.QuadPart;
#endif

  return 0;
}

quint64 FileSystemFreeSpace(const QString& path) {
#if defined(Q_OS_UNIX)
  struct statvfs fs_info;
  if (statvfs(path.toLocal8Bit().constData(), &fs_info) == 0)
    return quint64(fs_info.f_bavail) * quint64(fs_info.f_bsize);
#elif defined(Q_OS_WIN32)
  _ULARGE_INTEGER ret;
  if (GetDiskFreeSpaceEx(QDir::toNativeSeparators(path).toLocal8Bit().constData(),
                         &ret, NULL, NULL) != 0)
    return ret.QuadPart;
#endif

  return 0;
}

QString MakeTempDir() {
  QString path;
  {
    QTemporaryFile tempfile;
    tempfile.open();
    path = tempfile.fileName();
  }

  QDir d;
  d.mkdir(path);

  return path;
}

void RemoveRecursive(const QString& path) {
  QDir dir(path);
  foreach (const QString& child, dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs))
    RemoveRecursive(path + "/" + child);

  foreach (const QString& child, dir.entryList(QDir::NoDotAndDotDot | QDir::Files))
    QFile::remove(path + "/" + child);

  dir.rmdir(path);
}

bool Copy(QIODevice* source, QIODevice* destination) {
  if (!source->open(QIODevice::ReadOnly))
    return false;

  if (!destination->open(QIODevice::WriteOnly))
    return false;

  const qint64 bytes = source->size();
  boost::scoped_array<char> data(new char[bytes]);
  qint64 pos = 0;

  qint64 bytes_read;
  do {
    bytes_read = source->read(data.get() + pos, bytes - pos);
    if (bytes_read == -1)
      return false;

    pos += bytes_read;
  } while (bytes_read > 0 && pos != bytes);

  pos = 0;
  qint64 bytes_written;
  do {
    bytes_written = destination->write(data.get() + pos, bytes - pos);
    if (bytes_written == -1)
      return false;

    pos += bytes_written;
  } while (bytes_written > 0 && pos != bytes);

  return true;
}

QString ColorToRgba(const QColor& c) {
  return QString("rgba(%1, %2, %3, %4)")
      .arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}

QString GetConfigPath(ConfigPath config) {
  switch (config) {
    case Path_Root: {
      #ifdef Q_OS_DARWIN
        return mac::GetApplicationSupportPath() + "/" + QCoreApplication::organizationName();
      #else
        return QString("%1/.config/%2").arg(QDir::homePath(), QCoreApplication::organizationName());
      #endif
    }
    break;

    case Path_AlbumCovers:
      return GetConfigPath(Path_Root) + "/albumcovers";

    case Path_NetworkCache:
      return GetConfigPath(Path_Root) + "/networkcache";

    case Path_GstreamerRegistry:
      return GetConfigPath(Path_Root) +
          QString("/gst-registry-%1-bin").arg(QCoreApplication::applicationVersion());

    case Path_DefaultMusicLibrary:
      #ifdef Q_OS_DARWIN
        return mac::GetMusicDirectory();
      #else
        return QDir::homePath();
      #endif

    default:
      qFatal("%s", Q_FUNC_INFO);
      return QString::null;
  }
}

}  // namespace Utilities


ScopedWCharArray::ScopedWCharArray(const QString& str)
  : chars_(str.length()),
    data_(new wchar_t[chars_ + 1])
{
  str.toWCharArray(data_.get());
  data_[chars_] = '\0';
}
