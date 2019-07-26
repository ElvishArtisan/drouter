// tethertest.cpp
//
// Test harness for the 'Tether' class.
//
//   (C) Copyright 2019 Fred Gleason <fredg@paravelsystems.com>
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

#include <QCoreApplication>

#include "config.h"
#include "tethertest.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  QString err_msg;
  Config *config=new Config();
  config->load();

  if(!config->tetherIsActivated()) {
    printf("Tethering is deactivated in the configuration.\n");
    exit(0);
  }

  if(!config->tetherIsSane()) {
    fprintf(stderr,"tethertest: tether configuration is not sane\n");
    exit(256);
  }
  Tether *tether=new Tether(this);
  connect(tether,SIGNAL(instanceStateChanged(bool)),
	  this,SLOT(instanceStateChangedData(bool)));
  if(!tether->start(config,&err_msg)) {
    fprintf(stderr,"tethertest: start() failed [%s]\n",
	    (const char *)err_msg.toUtf8());
    exit(256);
  }
  
}


void MainObject::instanceStateChangedData(bool state)
{
  printf("instanceStateChanged(%d)\n",state);
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();

  return a.exec();
}
