// wallclock.h
//
// Digital Clock
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef WALLCLOCK_H
#define WALLCLOCK_H

#include <QDateTime>
#include <QFontMetrics>
#include <QPainter>
#include <QTimer>
#include <QWidget>

class WallClock : public QWidget
{
  Q_OBJECT
 public:
  WallClock(QWidget *parent=0);
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;
  void setDateFormat(const QString &fmt);
  void setTimeFormat(const QString &fmt);

 public slots:
  void timeoutData();

 protected:
  void paintEvent(QPaintEvent *e);

 private:
  QDateTime d_datetime;
  QTimer *d_timer;
  QFont d_time_font;
  QFontMetrics *d_time_metrics;
  QString d_time_format;
  QFont d_zone_font;
  QFontMetrics *d_zone_metrics;
  QFont d_date_font;
  QFontMetrics *d_date_metrics;
  QString d_date_format;
};


#endif  // WALLCLOCK_H

