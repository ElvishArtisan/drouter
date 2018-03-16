// dprotod.cpp
//
// Protocol dispatcher for drouterd(8)
//
//   (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
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

#include <sys/types.h>
#include <sys/wait.h>

#include <QCoreApplication>

#include "dprotod.h"
#include "protocol_d.h"

void SigHandler(int signo)
{
  switch(signo) {
  case SIGCHLD:
    waitpid(-1,NULL,WNOHANG);
    break;
  }
}


MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  //
  // The ProtocolD Server
  //
  main_protocol=new ProtocolD(this);
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();
  return a.exec();
}
