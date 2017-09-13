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

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <QCoreApplication>
#include <QHostAddress>

#include <sy/sycmdswitch.h>

#include "drouterd.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
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
  connect(main_drouter,SIGNAL(nodeAdded(const SyNode &)),
	  this,SLOT(nodeAddedData(const SyNode &)));
  connect(main_drouter,SIGNAL(nodeAboutToBeRemoved(const SyNode &)),
	  this,SLOT(nodeAboutToBeRemovedData(const SyNode &)));
  connect(main_drouter,SIGNAL(srcChanged(const SyNode &,int,const SySource &)),
	  this,SLOT(srcChangedData(const SyNode &,int,const SySource &)));
  connect(main_drouter,
	  SIGNAL(dstChanged(const SyNode &,int,const SyDestination &)),
	  this,
	  SLOT(dstChangedData(const SyNode &,int,const SyDestination &)));
}


void MainObject::nodeAddedData(const SyNode &node)
{
  printf("Added node at %s\n",
	 (const char *)node.hostAddress().toString().toUtf8());
  printf("%s\n",(const char *)node.dump().toUtf8());
}


void MainObject::nodeAboutToBeRemovedData(const SyNode &node)
{
  printf("Removed node at %s\n",
	 (const char *)node.hostAddress().toString().toUtf8());
}


void MainObject::srcChangedData(const SyNode &node,int slot,const SySource &src)
{
  printf("SOURCE %s:%d changed\n",
	 (const char *)node.hostAddress().toString().toUtf8(),slot);
  printf("%s\n",(const char *)src.dump().toUtf8());
}


void MainObject::dstChangedData(const SyNode &node,int slot,
				const SyDestination &dst)
{
  printf("DEST %s:%d changed\n",
	 (const char *)node.hostAddress().toString().toUtf8(),slot);
  printf("%s\n",(const char *)dst.dump().toUtf8());
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
