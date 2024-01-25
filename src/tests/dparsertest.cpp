// dparsertest.cpp
//
// dparsertest(8) routing daemon
//
//   (C) Copyright 2017-2021 Fred Gleason <fredg@paravelsystems.com>
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <QCoreApplication>
#include <QSignalMapper>

#include <sy5/sycmdswitch.h>
#include <sy5/syconfig.h>
#include <sy5/syinterfaces.h>
#include <sy5/syprofile.h>

#include "dparsertest.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  test_hostname="localhost";

  SyCmdSwitch *cmd=new SyCmdSwitch("dparsertest",VERSION,DPARSERTEST_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--hostname") {
      test_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"dparsertest: unknown option \"%s\"\n",
	      (const char *)cmd->key(i).toUtf8());
      exit(1);
    }
  }

  test_parser=new DRDParser(this);
  connect(test_parser,SIGNAL(connected(bool)),this,SLOT(connectedData(bool)));
  connect(test_parser,
	  SIGNAL(error(QAbstractSocket::SocketError,const QString &)),
	  this,SLOT(errorData(QAbstractSocket::SocketError,const QString &)));
  connect(test_parser,SIGNAL(nodeAdded(const QHostAddress &)),
	  this,SLOT(nodeAddedData(const QHostAddress &)));
  connect(test_parser,SIGNAL(nodeRemoved(const QHostAddress &)),
	  this,SLOT(nodeRemovedData(const QHostAddress &)));
  connect(test_parser,
	  SIGNAL(destinationChanged(const QHostAddress &,int,SyDestination *)),
	  this,
	SLOT(destinationChangedData(const QHostAddress &,int,SyDestination *)));
  connect(test_parser,SIGNAL(destinationAdded(const QHostAddress &,int)),
	  this,SLOT(destinationAddedData(const QHostAddress &,int)));
  connect(test_parser,SIGNAL(destinationRemoved(const QHostAddress &,int)),
	  this,SLOT(destinationRemovedData(const QHostAddress &,int)));
  connect(test_parser,SIGNAL(sourceChanged(const QHostAddress &,int,SySource *)),
	  this,SLOT(sourceChangedData(const QHostAddress &,int,SySource *)));
  connect(test_parser,SIGNAL(sourceAdded(const QHostAddress &,int)),
	  this,SLOT(sourceAddedData(const QHostAddress &,int)));
  connect(test_parser,SIGNAL(sourceRemoved(const QHostAddress &,int)),
	  this,SLOT(sourceRemovedData(const QHostAddress &,int)));
  connect(test_parser,SIGNAL(crosspointChanged(const QHostAddress &,int,
					      const QHostAddress,int)),
	  this,SLOT(crosspointChangedData(const QHostAddress &,int,
					  const QHostAddress,int)));
  connect(test_parser,SIGNAL(crosspointCleared(const QHostAddress &,int)),
	  this,SLOT(crosspointClearedData(const QHostAddress &,int)));
  test_parser->connectToHost(test_hostname,23883);
}


void MainObject::connectedData(bool state)
{
  printf("connectedData(%d)\n",state);
}


void MainObject::errorData(QAbstractSocket::SocketError err,
			   const QString &err_msg)
{
  fprintf(stderr,"dparsertest: %s\n",(const char *)err_msg.toUtf8());
}


void MainObject::destinationChangedData(const QHostAddress &addr,int slot,
					SyDestination *dst)
{
  printf("destination changed at %s:%d\n",
	 (const char *)addr.toString().toUtf8(),slot);  
  printf("%s\n",(const char *)dst->dump().toUtf8());
}


void MainObject::destinationAddedData(const QHostAddress &addr,int slot)
{
  printf("destination added at %s:%d\n",
	 (const char *)addr.toString().toUtf8(),slot);
}


void MainObject::destinationRemovedData(const QHostAddress &addr,int slot)
{
  printf("destination removed at %s:%d\n",
	 (const char *)addr.toString().toUtf8(),slot);
}


void MainObject::nodeAddedData(const QHostAddress &addr)
{
  printf("node added at %s\n",(const char *)addr.toString().toUtf8());
  printf("%s\n",(const char *)test_parser->node(addr)->dump().toUtf8());
}


void MainObject::nodeRemovedData(const QHostAddress &addr)
{
  printf("node removed at %s\n",(const char *)addr.toString().toUtf8());
  printf("\n");
}


void MainObject::sourceChangedData(const QHostAddress &addr,int slot,
				   SySource *src)
{
  printf("source changed at %s:%d\n",
	 (const char *)addr.toString().toUtf8(),slot);  
  printf("%s\n",(const char *)src->dump().toUtf8());
}


void MainObject::sourceAddedData(const QHostAddress &addr,int slot)
{
  printf("source added at %s:%d\n",
	 (const char *)addr.toString().toUtf8(),slot);
}


void MainObject::sourceRemovedData(const QHostAddress &addr,int slot)
{
  printf("source removed at %s:%d\n",
	 (const char *)addr.toString().toUtf8(),slot);
}


void MainObject::crosspointChangedData(const QHostAddress &daddr,int dslot,
				       const QHostAddress &saddr,int sslot)
{
  printf("crosspoint %s:%d changed to %s:%d\n",
	 (const char *)daddr.toString().toUtf8(),dslot,
	 (const char *)saddr.toString().toUtf8(),sslot);
}


void MainObject::crosspointClearedData(const QHostAddress &daddr,int dslot)
{
  printf("crosspoint %s:%d cleared\n",
	 (const char *)daddr.toString().toUtf8(),dslot);
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
