// watchdog.cpp
//
// Abstract watchdog
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

#include "watchdog.h"

Watchdog::Watchdog(QObject *parent)
  : QObject(parent)
{
  d_poll_interval=WATCHDOG_DEFAULT_POLL_INTERVAL;
  d_timeout_interval=WATCHDOG_DEFAULT_TIMEOUT_INTERVAL;

  d_poll_timer=new QTimer(this);
  d_poll_timer->setSingleShot(true);
  connect(d_poll_timer,SIGNAL(timeout()),this,SLOT(pollData()));

  d_timeout_timer=new QTimer(this);
  d_timeout_timer->setSingleShot(true);
  connect(d_timeout_timer,SIGNAL(timeout()),this,SLOT(timeoutData()));
}


Watchdog::~Watchdog()
{
  delete d_poll_timer;
  delete d_timeout_timer;
}


int Watchdog::pollInterval() const
{
  return d_poll_interval;
}


void Watchdog::setPollInterval(int msecs)
{
  d_poll_interval=msecs;
}


int Watchdog::timeoutInterval() const
{
  return d_timeout_interval;
}


void Watchdog::setTimeoutInterval(int msecs)
{
  d_timeout_interval=msecs;
}


bool Watchdog::isActive()
{
  return d_timeout_timer->isActive();
}


void Watchdog::start()
{
  d_timeout_timer->start(5000+d_timeout_interval);
  d_poll_timer->start(5000);
}


void Watchdog::stop()
{
  d_poll_timer->stop();
  d_timeout_timer->stop();
}


void Watchdog::touch()
{
  if(d_timeout_timer->isActive()) {
    d_timeout_timer->stop();
    d_timeout_timer->start(d_timeout_interval);
  }
}


void Watchdog::pollData()
{
  emit poll();
  d_poll_timer->start(d_poll_interval);
}


void Watchdog::timeoutData()
{
  d_poll_timer->stop();
  emit timeout();
}
