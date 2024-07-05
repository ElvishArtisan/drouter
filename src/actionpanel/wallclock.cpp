// wallclock.cpp
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

#include <QTimeZone>

#include "wallclock.h"

WallClock::WallClock(QWidget *parent)
  : QWidget(parent)
{
  d_time_font=QFont(font().family(),20,QFont::Normal);
  d_time_metrics=new QFontMetrics(d_time_font);
  d_time_format="hh:mm:ss";

  d_zone_font=QFont(font().family(),12,QFont::Normal);
  d_zone_metrics=new QFontMetrics(d_zone_font);

  d_date_font=QFont(font());
  d_date_metrics=new QFontMetrics(d_date_font);
  d_date_format="dddd, MMMM dd yyyy";

  d_timer=new QTimer(this);
  d_timer->setSingleShot(false);
  connect(d_timer,SIGNAL(timeout()),this,SLOT(timeoutData()));

  d_timer->start(100);
}


QSize WallClock::sizeHint() const
{
  return QSize(120,80);
}


QSizePolicy WallClock::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}


void WallClock::setDateFormat(const QString &fmt)
{
  d_date_format=fmt;
}


void WallClock::setTimeFormat(const QString &fmt)
{
  d_time_format=fmt;
}


void WallClock::timeoutData()
{
  QDateTime now=QDateTime::currentDateTime();
  QDateTime rounded(now.date(),
	 QTime(now.time().hour(),now.time().minute(),now.time().second()));

  if(rounded!=d_datetime) {
    d_datetime=rounded;
    update();
    /*
    d_time_label->setText(d_datetime.time().toString("hh:mm:ss"));
    d_zone_label->setText(QTimeZone::systemTimeZone().abbreviation(d_datetime));
    d_date_label->setText(d_datetime.date().toString("dddd, MMMM d yyyy"));
    */
  }
}


void WallClock::paintEvent(QPaintEvent *e)
{
  int w=size().width();
  int h=size().height();

  QString timestr=d_datetime.time().toString(d_time_format);
  QString zonestr=QTimeZone::systemTimeZone().abbreviation(d_datetime);
  QString datestr=d_datetime.date().toString(d_date_format);

  QPainter *p=new QPainter(this);
  p->setPen(palette().color(QPalette::WindowText));
  p->setBrush(palette().color(QPalette::WindowText));

  //
  // Time
  //
  int time_w=d_time_metrics->horizontalAdvance(timestr)+5+
    d_zone_metrics->horizontalAdvance(zonestr);
  p->setFont(d_time_font);
  p->drawText((w-time_w)/2,d_time_metrics->height(),timestr);

  //
  // Time Zone
  //
  p->setFont(d_zone_font);
  p->drawText((w-time_w)/2+d_time_metrics->horizontalAdvance(timestr)+5,
	      d_time_metrics->height(),zonestr);

  //
  // Date
  //
  p->setFont(d_date_font);
  p->drawText((w-d_date_metrics->horizontalAdvance(datestr))/2,
	      h-d_date_metrics->height(),datestr);

  delete p;
}
