// exitnotifier.cpp
//
//  Emit a Qt signal upon reception of SIGINT or SIGTERM
//
//   (C) Copyright 2019-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "exitnotifier.h"

int __exit_notifier_sockets[2];

void __ExitNotifierSigHandler(int signo)
{
  switch(signo) {
  case SIGINT:
  case SIGTERM:
    if(write(__exit_notifier_sockets[0],"X",1)<0) {
      fprintf(stderr,"error writing exit notifier [%s]\n",strerror(errno));
    }
  }
}


ExitNotifier::ExitNotifier(QObject *parent)
  : QObject(parent)
{
  if(socketpair(AF_UNIX,SOCK_STREAM,0,__exit_notifier_sockets)!=0) {
    syslog(LOG_WARNING,"unable to install exit handler [%s]",strerror(errno));
    return;
  }

  exit_notifier=
    new QSocketNotifier(__exit_notifier_sockets[1],QSocketNotifier::Read,this);
  connect(exit_notifier,SIGNAL(activated(int)),this,SLOT(activatedData(int)));

  ::signal(SIGINT,__ExitNotifierSigHandler);
  ::signal(SIGTERM,__ExitNotifierSigHandler);
  syslog(LOG_DEBUG,"installed exit notifier");
}


ExitNotifier::~ExitNotifier()
{
  delete exit_notifier;
  close(__exit_notifier_sockets[0]);
  close(__exit_notifier_sockets[1]);
  syslog(LOG_DEBUG,"removed exit notifier");
}


void ExitNotifier::activatedData(int sock)
{
  syslog(LOG_DEBUG,"emitting exit notification");
  emit aboutToExit();

  syslog(LOG_DEBUG,"exiting normally");
  exit(0);
}
