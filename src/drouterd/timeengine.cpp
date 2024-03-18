//   timeengine.cpp
//
//   An event timer engine.
//
//   (C) Copyright 2002-2024 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Library General Public License 
//   version 2 as published by the Free Software Foundation.
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

#include "timeengine.h"

TimeEngine::TimeEngine(QObject *parent)
  : QObject(parent)
{
  d_mapper=new QSignalMapper(this);
  connect(d_mapper,SIGNAL(mapped(int)),this,SLOT(timerData(int)));
}


TimeEngine::~TimeEngine()
{
  clear();
  delete d_mapper;
}


void TimeEngine::clear()
{
  for(QMap<int,QTimer *>::const_iterator it=d_timers.begin();
      it!=d_timers.end();it++) {
    d_mapper->removeMappings(it.value());
    delete it.value();
  }
  d_timers.clear();
  d_times.clear();
}


QTime TimeEngine::event(int id) const
{
  return d_times.value(id);
}


void TimeEngine::addEvent(int id,const QTime &time)
{
  d_times[id]=time;
  d_timers[id]=new QTimer(this);
  d_timers.value(id)->setTimerType(Qt::PreciseTimer);
  d_timers.value(id)->setSingleShot(true);
  d_mapper->setMapping(d_timers.value(id),id);
  connect(d_timers.value(id),SIGNAL(timeout()),d_mapper,SLOT(map()));
  StartEvent(id);
}


void TimeEngine::removeEvent(int id)
{
  d_timers.value(id)->stop();
  d_mapper->removeMappings(d_timers.value(id));
  delete d_timers.value(id);
  d_timers.remove(id);
  d_times.remove(id);
}


void TimeEngine::timerData(int id)
{
  emit timeout(id);
  StartEvent(id);
}


void TimeEngine::StartEvent(int id)
{
  QTime start_time=d_times.value(id);
  QTime now=QDateTime::currentDateTime().time();
  int interval=0;

  if(start_time<now) {
    interval=86400000+now.msecsTo(start_time);
  }
  else {
    interval=now.msecsTo(start_time);
  }
  d_timers.value(id)->start(interval);
  /*
  printf("timeengine: id: %d  now: %s  start: %s  timer_interval: %d\n",
	 id,now.toString("hh:mm:ss.zzz").toUtf8().constData(),
	 start_time.toString("hh:mm:ss.zzz").toUtf8().constData(),interval);
  */
}
