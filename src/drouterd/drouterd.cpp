// drouterd.cpp
//
// Dynamic router service for Livewire networks
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <QCoreApplication>
#include <QHostAddress>

#include <sy/sycmdswitch.h>

#ifdef LIBSYSTEMD
#include <systemd/sd-daemon.h>
#endif  // LIBSYSTEMD

#include "drouterd.h"
#include "protocolfactory.h"

bool global_reload=false;

void SigHandler(int signo)
{
  switch(signo) {
  case SIGHUP:
    global_reload=true;
    break;
  }
}


MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  int socks[2]={-1,-1};
  int n;
  QString err_msg;
  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"drouterd",VERSION,DROUTERD_USAGE);
  for(unsigned i=0;i<(cmd->keys());i++) {
    if(!cmd->processed(i)) {
      fprintf(stderr,"drouterd: unknown option\n");
      exit(256);
    }
  }

  //
  // Open Syslog
  //
  openlog("drouterd",LOG_PERROR,LOG_DAEMON);

  //
  // DRouter
  //
  main_drouter=new DRouter(this);

#ifdef LIBSYSTEMD
  //
  // Get sockets from SystemD
  //
  n=sd_listen_fds(0);
  if(n>0) {
    if(n==2) {
      socks[0]=SD_LISTEN_FDS_START+0;
      socks[1]=SD_LISTEN_FDS_START+1;
    }
    else {
      fprintf(stderr,"drouterd: error receiving sockets from SystemD\n");
      exit(1);
    }
  }
#endif  // LIBSYSTEMD

  //
  // Protocols
  //
  main_protocols.
    push_back(ProtocolFactory(main_drouter,Protocol::ProtocolD,socks[0],this));
  main_protocols.
    push_back(ProtocolFactory(main_drouter,Protocol::ProtocolSa,socks[1],this));

  //
  // Set Signals
  //
  main_signal_timer=new QTimer(this);
  connect(main_signal_timer,SIGNAL(timeout()),this,SLOT(signalData()));
  main_signal_timer->start(500);
  signal(SIGHUP,SigHandler);
}


void MainObject::signalData()
{
  if(global_reload) {
    for(int i=0;i<main_protocols.size();i++) {
      main_protocols.at(i)->reload();
    }
    syslog(LOG_INFO,"reloaded configuration");
    global_reload=false;
  }
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
