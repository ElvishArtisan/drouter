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

#include <QCoreApplication>

#include <sy/sycmdswitch.h>

#include "dprotod.h"
#include "protocol_d.h"
#include "protocol_sa.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  main_protocol=NULL;
  bool protocol_d=false;
  bool protocol_sa=false;
  int protocols_defined=0;

  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"dprotod",VERSION,DPROTOD_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--protocol-d") {
      protocol_d=true;
      protocols_defined++;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--protocol-sa") {
      protocol_sa=true;
      protocols_defined++;
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"dprotod: unrecognized option\n");
      exit(1);
    }
  }
  if(protocols_defined==0) {
    fprintf(stderr,"dprotod: no --protocol specified\n");
    exit(1);
  }
  if(protocols_defined>1) {
    fprintf(stderr,
	    "dprotod: only one --protocol may be specified per instance\n");
    exit(1);
  }

  if(protocol_d) {
    main_protocol=new ProtocolD(this);
  }
  if(protocol_sa) {
    main_protocol=new ProtocolSa(this);
  }
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();
  return a.exec();
}
