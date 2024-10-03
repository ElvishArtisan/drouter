// getnode.cpp
//
// getnode() LWRP dump utility
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

#include <QCoreApplication>
#include <QStringList>

#include <stdio.h>
#include <stdlib.h>

#include <sy5/sycmdswitch.h>

#include "getnode.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  QString hostname;
  unsigned port=93;
  d_end_count=0;
  
  SyCmdSwitch *cmd=new SyCmdSwitch("getnode",VERSION,GETNODE_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--hostname") {
      QStringList f0=cmd->value(i).split(":",Qt::KeepEmptyParts);
      if(f0.size()>2) {
	fprintf(stderr,"getnode: invalid hostname:port specified\n");
	exit(1);
      }
      if(f0.size()>1) {
	bool ok=false;
	port=f0.last().toUInt(&ok);
	if((!ok)||(port>0xffff)) {
	  fprintf(stderr,"getnode: invalid port specified\n");
	  exit(1);
	}
      }
      hostname=f0.first();
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"getnode: unknown switch \"%s\"\n",
	      cmd->key(i).toUtf8().constData());
      exit(1);
    }
  }
  if(hostname.trimmed().isEmpty()) {
    fprintf(stderr,"getnode: invalid hostname\n");
    exit(1);
  }

  d_socket=new QTcpSocket(this);
  connect(d_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(d_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(d_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
  d_socket->connectToHost(hostname,port);
}


void MainObject::connectedData()
{
  SendCommand("SRC");
  SendCommand("DST");
  SendCommand("CFG GPO");
}


void MainObject::readyReadData()
{
  QByteArray data=d_socket->readAll();

  for(int i=0;i<data.size();i++) {
    switch(data.at(i)) {
    case '\r':
      break;

    case '\n':
      printf("%s\r\n",d_accum.constData());
      if(d_accum=="END") {
	d_end_count++;
	if(d_end_count==3) {
	  exit(0);
	}
      }
      d_accum.clear();
      break;

    default:
      d_accum+=data.at(i);
      break;
    }
  }
}


void MainObject::errorData(QAbstractSocket::SocketError err)
{
  fprintf(stderr,"getnode: received socket error %d\n",err);
  exit(1);
}


void MainObject::SendCommand(const QString &msg)
{
  d_socket->write((msg+"\r\n").toUtf8());
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();

  return a.exec();
}
