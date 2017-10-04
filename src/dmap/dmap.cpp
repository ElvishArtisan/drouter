// dmap.cpp
//
// dmap(8) routing daemon
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <QCoreApplication>
#include <QSignalMapper>

#include <sy/sycmdswitch.h>
#include <sy/syconfig.h>
#include <sy/syinterfaces.h>
#include <sy/syprofile.h>

#include "dmap.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  bool ok=false;

  map_output_map="";
  map_no_off_source=false;
  map_router_number=0;
  map_router_name="Livewire";
  map_router_type=EndPointMap::AudioRouter;

  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"dmap",VERSION,DMAP_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--no-off-source") {
      map_no_off_source=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--output-map") {
      map_output_map=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--router-name") {
      map_router_name=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--router-number") {
      map_router_number=cmd->value(i).toInt(&ok);
      if((!ok)||(map_router_number<=0)) {
	fprintf(stderr,"dmap: invalid matrix number\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--router-type") {
      bool found=false;
      for(int j=0;j<EndPointMap::LastRouter;j++) {
	EndPointMap::RouterType rtype=(EndPointMap::RouterType)j;
	if(cmd->value(i).toLower()==
	   EndPointMap::routerTypeString(rtype).toLower()) {
	  map_router_type=rtype;
	  found=true;
	}
      }
      if(!found) {
	fprintf(stderr,"dmap: unknown router type \"%s\"\n",
		(const char *)cmd->value(i).toUtf8());
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--skip-node") {
      map_skip_node_addresses.
	push_back(QHostAddress(cmd->value(i)));
      if(map_skip_node_addresses.back().isNull()) {
	fprintf(stderr,"dmap: invalid --skip-node argument\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--skip-node-list") {
      int line=ProcessSkipNodeList(cmd->value(i));
      if(line<0) {
	fprintf(stderr,"unable to process --skip-node-list argument [%s]\n",
		strerror(-line));
	exit(256);
      }
      if(line>0) {
	fprintf(stderr,"invalid address in --use-node-list target [%s:%d]\n",
		(const char *)cmd->value(i).toUtf8(),line);
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--use-node") {
      map_node_addresses.push_back(QHostAddress(cmd->value(i)));
      if(map_node_addresses.back().isNull()) {
	fprintf(stderr,"dmap: invalid --use-node argument\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--use-node-list") {
      int line=ProcessUseNodeList(cmd->value(i));
      if(line<0) {
	fprintf(stderr,"unable to process --use-node-list argument [%s]\n",
		strerror(-line));
	exit(256);
      }
      if(line>0) {
	fprintf(stderr,"invalid address in --use-node-list target [%s:%d]\n",
		(const char *)cmd->value(i).toUtf8(),line);
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"dmap: unknown option\n");
      exit(256);
    }
  }

  //
  // Create Map
  //
  map_map=new EndPointMap();
  map_map->setRouterType(map_router_type);
  map_map->setRouterName(map_router_name);
  map_map->setRouterNumber(map_router_number-1);

  //
  // Connect to DRouter
  //
  map_parser=new DParser(this);
  connect(map_parser,SIGNAL(connected(bool)),this,SLOT(connectedData(bool)));
  connect(map_parser,
	  SIGNAL(error(QAbstractSocket::SocketError,const QString &)),
	  this,
	  SLOT(errorData(QAbstractSocket::SocketError,const QString &)));
  map_parser->connectToHost("localhost",23883);
}


void MainObject::connectedData(bool state)
{
  QList<QHostAddress> hosts;

  //
  // Get Node List
  //
  QList<QHostAddress> addrs=map_parser->nodeHostAddresses();
  if(map_node_addresses.size()>0) {
    addrs=map_node_addresses;
  }
  for(int i=0;i<addrs.size();i++) {
    bool match=false;
    for(int j=0;j<map_skip_node_addresses.size();j++) {
      if(addrs.at(i)==map_skip_node_addresses.at(j)) {
	match=true;
      }
    }
    if(!match) {
      hosts.push_back(addrs.at(i));
    }
  }

  //
  // Sort Node List
  //
  bool changed=true;
  while(changed) {
    changed=false;
    for(int i=0;i<hosts.size()-1;i++) {
      if(hosts.at(i).toIPv4Address()>hosts.at(i+1).toIPv4Address()) {
	QHostAddress addr=hosts.at(i);
	hosts[i]=hosts.at(i+1);
	hosts[i+1]=addr;
	changed=true;
      }
    }
  }

  if(map_router_type==EndPointMap::AudioRouter) {
    //
    // Sources
    //
    for(int i=0;i<hosts.size();i++) {
      SyNode *node=map_parser->node(hosts.at(i));
      for(unsigned j=0;j<node->srcSlotQuantity();j++) {
	map_map->
	  insert(EndPointMap::Input,map_map->quantity(EndPointMap::Input),
		 node->hostAddress(),j);
      }
    }

    //
    // Destinations
    //
    for(int i=0;i<hosts.size();i++) {
      SyNode *node=map_parser->node(hosts.at(i));
      for(unsigned j=0;j<node->dstSlotQuantity();j++) {
	map_map->
	  insert(EndPointMap::Output,map_map->quantity(EndPointMap::Output),
		 node->hostAddress(),j);
      }
    }
  }
  if(map_router_type==EndPointMap::GpioRouter) {
    //
    // GPIs
    //
    for(int i=0;i<hosts.size();i++) {
      SyNode *node=map_parser->node(hosts.at(i));
      for(unsigned j=0;j<node->gpiSlotQuantity();j++) {
	map_map->
	  insert(EndPointMap::Input,map_map->quantity(EndPointMap::Input),
		 node->hostAddress(),j);
      }
    }

    //
    // GPOs
    //
    for(int i=0;i<hosts.size();i++) {
      SyNode *node=map_parser->node(hosts.at(i));
      for(unsigned j=0;j<node->gpoSlotQuantity();j++) {
	map_map->
	  insert(EndPointMap::Output,map_map->quantity(EndPointMap::Output),
		 node->hostAddress(),j);
      }
    }
  }

  //
  // Save
  //
  if(map_output_map.isEmpty()) {
    map_map->save(stdout);
  }
  else {
    if(!map_map->save(map_output_map)) {
      fprintf(stderr,"dmap: unable to save map\n");
      exit(1);
    }
  }

  exit(0);
}


void MainObject::errorData(QAbstractSocket::SocketError err,
			   const QString &err_msg)
{
  fprintf(stderr,"dmap: parser error [%s]\n",(const char *)err_msg.toUtf8());
  exit(1);
}


int MainObject::ProcessUseNodeList(const QString &filename)
{
  FILE *f=NULL;
  int count=0;
  char line[32];
  QHostAddress addr;

  if((f=fopen(filename.toUtf8(),"r"))==NULL) {
    return -errno;
  }
  while(fgets(line,31,f)!=NULL) {
    count++;
    addr.setAddress(QString(line).trimmed());
    if(addr.isNull()) {
      fclose(f);
      return count;
    }
    map_node_addresses.push_back(addr);
  }
  fclose(f);

  return 0;
}


int MainObject::ProcessSkipNodeList(const QString &filename)
{
  FILE *f=NULL;
  int count=0;
  char line[32];
  QHostAddress addr;

  if((f=fopen(filename.toUtf8(),"r"))==NULL) {
    return -errno;
  }
  while(fgets(line,31,f)!=NULL) {
    count++;
    addr.setAddress(QString(line).trimmed());
    if(addr.isNull()) {
      fclose(f);
      return count;
    }
    map_skip_node_addresses.push_back(addr);
  }
  fclose(f);

  return 0;
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
