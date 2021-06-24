// gpioflasher.cpp
//
// Component for flashing GPIO devices via LWRP
//
//   (C) Copyright 2021 Fred Gleason <fredg@paravelsystems.com>
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

#include <syslog.h>

#include "gpioflasher.h"

GpioFlasher::GpioFlasher(QObject *parent)
  : QObject(parent)
{
  d_state=false;

  for(int i=0;i<2;i++) {
    d_lwrps[i]=NULL;
    d_types[i]=SyGpioBundleEvent::TypeGpi;
    d_slots[i]=-1;
    d_codes[i]="xxxxx";

  d_timer=new QTimer(this);
  d_timer->setSingleShot(true);
  connect(d_timer,SIGNAL(timeout()),this,SLOT(timeoutData()));
  }
}


GpioFlasher::~GpioFlasher()
{
  setActive(false);
  delete d_timer;
}


void GpioFlasher::addGpio(Config::TetherRole role,SyLwrpClient *lwrp,
			  SyGpioBundleEvent::Type type,int slot,
			  const QString &code)
{
  d_lwrps[role]=lwrp;
  d_types[role]=type;
  d_slots[role]=slot;
  if(role==Config::This) {
    d_codes[role]=code;
  }
  else {
    d_codes[role]=SyGpioBundle::invertCode(code);
  }
}


bool GpioFlasher::isActive() const
{
  return d_timer->isActive();
}


void GpioFlasher::setActive(bool state)
{
  if(state!=d_timer->isActive()) {
    if(state) {
      d_state=false;
      d_timer->start(100);
    }
    else {
      d_timer->stop();
      if(d_lwrps[Config::This]!=NULL) {
	if(d_types[Config::This]==SyGpioBundleEvent::TypeGpi) {
	  d_lwrps[Config::This]->
	    setGpiCode(d_slots[Config::This],
		       SyGpioBundle::invertCode(d_codes[Config::This]));
	}
	else {
	  d_lwrps[Config::This]->
	    setGpoCode(d_slots[Config::This],
		       SyGpioBundle::invertCode(d_codes[Config::This]));
	}
      }
    }
  }
}


void GpioFlasher::timeoutData()
{
  if(d_state) {
    if(d_lwrps[Config::This]!=NULL) {
      if(d_types[Config::This]==SyGpioBundleEvent::TypeGpi) {
	d_lwrps[Config::This]->
	  setGpiCode(d_slots[Config::This],
		     SyGpioBundle::invertCode(d_codes[Config::This]));
      }
      else {
	d_lwrps[Config::This]->
	  setGpoCode(d_slots[Config::This],
		     SyGpioBundle::invertCode(d_codes[Config::This]));
      }
    }
    d_timer->start(100);
  }
  else {
    for(int i=0;i<2;i++) {  // Clear the Config::That instance as well
      if(d_lwrps[i]!=NULL) {
	if(d_types[i]==SyGpioBundleEvent::TypeGpi) {
	  d_lwrps[i]->setGpiCode(d_slots[i],d_codes[i]);
	}
	else {
	  d_lwrps[i]->setGpoCode(d_slots[i],d_codes[i]);
	}
      }
    }
    d_timer->start(900);
  }
  d_state=!d_state;
}
