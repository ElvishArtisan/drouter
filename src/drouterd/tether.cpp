// tether.cpp
//
//  State Manager for twin Drouter instances
//
//   (C) Copyright 2019 Fred Gleason <fredg@paravelsystems.com>
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

#include <signal.h>
#include <stdlib.h>
#include <time.h>

#include <QProcess>
#include <QStringList>

#include <sy/syinterfaces.h>

#include "config.h"
#include "tether.h"

QStringList global_exit_args;

void __Tether_atexit()
{
  if(global_exit_args.size()>0) {
    QProcess *proc=new QProcess();
    proc->start("/sbin/ip",global_exit_args);
    proc->waitForFinished();
  }
}


void SigHandler(int signo)
{
  switch(signo) {
  case SIGINT:
  case SIGTERM:
    exit(0);
    break;
  }
}


Tether::Tether(QObject *parent)
  : QObject(parent)
{
  tether_active_state=false;
  tether_config=NULL;

  srandom(time(NULL));

  tether_udp_socket=new QUdpSocket(this);
  connect(tether_udp_socket,SIGNAL(readyRead()),this,SLOT(udpReadyReadData()));

  tether_tty_device=new TTYDevice(this);
  tether_tty_device->setSpeed(DROUTER_TETHER_TTY_SPEED);
  tether_tty_device->setWordLength(DROUTER_TETHER_TTY_WORD_LENGTH);
  tether_tty_device->setParity(DROUTER_TETHER_TTY_PARITY);
  tether_tty_device->setFlowControl(DROUTER_TETHER_TTY_FLOW_CONTROL);
  connect(tether_tty_device,SIGNAL(readyRead()),this,SLOT(ttyReadyReadData()));

  tether_interval_timer=new QTimer(this);
  tether_interval_timer->setSingleShot(true);
  connect(tether_interval_timer,SIGNAL(timeout()),
	  this,SLOT(intervalTimeoutData()));

  tether_window_timer=new QTimer(this);
  tether_window_timer->setSingleShot(true);
  connect(tether_window_timer,SIGNAL(timeout()),
	  this,SLOT(windowTimeoutData()));
}


bool Tether::instanceIsActive() const
{
  if(!tether_config->tetherIsActivated()) {
    return true;
  }
  if(!tether_config->tetherIsSane()) {
    return false;
  }
  return tether_active_state;
}


bool Tether::start(Config *config,QString *err_msg)
{
  tether_config=config;

  if(!tether_config->tetherIsActivated()) {
    emit instanceStateChanged(true);
    return true;
  }

  if(!tether_udp_socket->bind(DROUTER_TETHER_UDP_PORT)) {
    *err_msg=QString().
      sprintf("unable to bind tether udp port %u",DROUTER_TETHER_UDP_PORT);
    return false;
  }
  tether_tty_device->setName(tether_config->tetherSerialDevice(Config::This));
  if(!tether_tty_device->open(QIODevice::ReadWrite)) {
    *err_msg="unable to open tether tty port \""+tether_tty_device->name()+"\"";
    return false;
  }
  atexit(__Tether_atexit);
  signal(SIGINT,SigHandler);
  signal(SIGTERM,SigHandler);
  tether_interval_timer->start(GetInterval());

  return true;
}


void Tether::udpReadyReadData()
{
  QHostAddress addr;
  char data[1];
  int n;

  while((n=tether_udp_socket->readDatagram(data,1,&addr))>0) {
    if(addr==tether_config->tetherIpAddress(Config::That)) {
      if(tether_window_timer->isActive()) {
	if(data[0]=='?') {
	  Backoff();
	}
	else {
	  tether_udp_state=data[0];
	}
      }
      else {
	if(data[0]=='?') {
	  if(tether_active_state) {
	    tether_udp_socket->
	      writeDatagram("+",1,tether_config->tetherIpAddress(Config::That),
			    DROUTER_TETHER_UDP_PORT);
	  }
	  else {
	    tether_udp_socket->
	      writeDatagram("-",1,tether_config->tetherIpAddress(Config::That),
			    DROUTER_TETHER_UDP_PORT);
	  }
	}	
      }
    }
  }
}


void Tether::ttyReadyReadData()
{
  char data[1];
  int n;

  while((n=tether_tty_device->read(data,1))>0) {
    if(tether_window_timer->isActive()) {
      if(data[0]=='?') {
	Backoff();
      }
      else {
	tether_tty_state=data[0];
      }
    }
    else {
      if(data[0]=='?') {
	if(tether_active_state) {
	  tether_tty_device->write("+",1);
	}
	else {
	  tether_tty_device->write("-",1);
	}
      }	
    }
  }
}


void Tether::intervalTimeoutData()
{
  tether_udp_state='-';
  tether_udp_replied=false;
  tether_udp_socket->
    writeDatagram("?",1,tether_config->tetherIpAddress(Config::That),
		  DROUTER_TETHER_UDP_PORT);

  tether_tty_state='-';
  tether_tty_replied=false;
  tether_tty_device->write("?",1);

  tether_window_timer->start(DROUTER_TETHER_WINDOW_INTERVAL);
}


void Tether::windowTimeoutData()
{
  if(tether_active_state) {
    if((tether_udp_state=='+')||(tether_tty_state=='+')) {
      tether_active_state=false;
      ModifySharedAddress("del");
      emit instanceStateChanged(false);
    }
  }
  else {
    if((tether_udp_state!='+')&&(tether_tty_state!='+')) {
      tether_active_state=true;
      ModifySharedAddress("add");
      emit instanceStateChanged(true);
    }
  }

  tether_interval_timer->start(GetInterval());
}


void Tether::Backoff()
{
  tether_window_timer->stop();
  tether_interval_timer->start(GetInterval());  
}


int Tether::GetInterval() const
{
  int64_t val=0;

  val+=DROUTER_TETHER_BASE_INTERVAL*random()/RAND_MAX;

  return val;
}


bool Tether::ModifySharedAddress(const QString &keyword) const
{
  QStringList args;
  QString iface_name;
  uint32_t iface_mask;
  SyInterfaces *ifaces=new SyInterfaces();

  ifaces->update();
  for(int i=0;i<ifaces->quantity();i++) {
    if(ifaces->ipv4Address(i)==tether_config->tetherIpAddress(Config::This)) {
      iface_name=ifaces->name(i);
      iface_mask=SyInterfaces::toCidrMask(ifaces->ipv4Netmask(i));

      args.push_back("addr");
      args.push_back(keyword);
      args.push_back(tether_config->tetherSharedIpAddress().toString()+
		     QString().sprintf("/%u",iface_mask));
      args.push_back("dev");
      args.push_back(iface_name);

      if(keyword=="add") {
	global_exit_args.clear();
	global_exit_args.push_back("addr");
	global_exit_args.push_back("del");
	global_exit_args.
	  push_back(tether_config->tetherSharedIpAddress().toString()+
		    QString().sprintf("/%u",iface_mask));
	global_exit_args.push_back("dev");
	global_exit_args.push_back(iface_name);
      }
      QProcess *proc=new QProcess();
      proc->start("/sbin/ip",args);
      proc->waitForFinished();
      delete ifaces;
      return true;
    }
  }
  delete ifaces;

  return false;
}
