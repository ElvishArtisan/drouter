// endpointmap.cpp
//
// Map integers to DRouter endpoints.
//
// (C) Copyright 2017-2022 Fred Gleason <fredg@paravelsystems.com>
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
#include <unistd.h>

#include <QDir>

#include <sy5/syprofile.h>

#include "endpointmap.h"

Snapshot::Snapshot(const QString &name)
{
  snap_name=name;
}


QString Snapshot::name() const
{
  return snap_name;
}


void Snapshot::setName(const QString &str)
{
  snap_name=str;
}


int Snapshot::routeQuantity() const
{
  return snap_inputs.size();
}


int Snapshot::routeInput(int n) const
{
  return snap_inputs.at(n);
}


int Snapshot::routeOutput(int n) const
{
  return snap_outputs.at(n);
}


void Snapshot::addRoute(int output,int input)
{
  snap_outputs.push_back(output);
  snap_inputs.push_back(input);
}




EndPointMap::EndPointMap()
{
  map_router_type=EndPointMap::AudioRouter;
  map_router_name="Livewire";
  map_router_number=0;
}


EndPointMap::RouterType EndPointMap::routerType() const
{
  return map_router_type;
}


void EndPointMap::setRouterType(EndPointMap::RouterType type)
{
  map_router_type=type;
}


QString EndPointMap::routerName() const
{
  return map_router_name;
}


void EndPointMap::setRouterName(const QString &str)
{
  map_router_name=str;
}


int EndPointMap::routerNumber() const
{
  return map_router_number;
}


void EndPointMap::setRouterNumber(int num)
{
  map_router_number=num;
}


int EndPointMap::quantity(EndPointMap::Type type) const
{
  return map_host_addresses[type].size();
}


QHostAddress EndPointMap::hostAddress(EndPointMap::Type type,int n) const
{
  if(n<map_host_addresses[type].size()) {
    return map_host_addresses[type].at(n);
  }
  return QHostAddress();
}


void EndPointMap::setHostAddress(EndPointMap::Type type,int n,const QHostAddress &addr)
{
  map_host_addresses[type][n]=addr;
}


void EndPointMap::setHostAddress(EndPointMap::Type type,int n,const QString &addr)
{
  map_host_addresses[type][n].setAddress(addr);
}


int EndPointMap::slot(EndPointMap::Type type,int n) const
{
  if(n<map_slots[type].size()) {
    return map_slots[type].at(n);
  }
  return -1;
}


void EndPointMap::setSlot(EndPointMap::Type type,int n,int slot)
{
  map_slots[type][n]=slot;
}


QString EndPointMap::name(EndPointMap::Type type,int n,const QString &orig_name) const
{
  QString ret;

  if(n<map_names[type].size()) {
    ret=map_names[type].at(n);
  }
  if(ret.isEmpty()) {
    ret=orig_name;
  }
  return ret;
}


void EndPointMap::setName(EndPointMap::Type type,int n,const QString &str)
{
  map_names[type][n]=str;
}


int EndPointMap::endPoint(Type type,const QHostAddress &hostaddr,int slot) const
{
  for(int i=0;i<map_host_addresses[type].size();i++) {
    if((map_host_addresses[type].at(i)==hostaddr)&&
       (map_slots[type].at(i)==slot)) {
      return i;
    }
  }
  return -1;
}


int EndPointMap::endPoint(Type type,const QString &hostaddr,int slot) const
{
  return endPoint(type,QHostAddress(hostaddr),slot);
}


void EndPointMap::insert(EndPointMap::Type type,int n,const QHostAddress &host_addr,int slot,
			 const QString &name)
{
  map_host_addresses[type].insert(n,host_addr);
  map_slots[type].insert(n,slot);
  map_names[type].insert(n,name);
}


void EndPointMap::insert(EndPointMap::Type type,int n,const QString &host_addr,int slot,
			 const QString &name)
{
  map_host_addresses[type].insert(n,QHostAddress(host_addr));
  map_slots[type].insert(n,slot);
  map_names[type].insert(n,name);
}


void EndPointMap::erase(EndPointMap::Type type,int n)
{
  map_host_addresses[type].erase(map_host_addresses[type].begin()+n);
  map_slots[type].erase(map_slots[type].begin()+n);
}


int EndPointMap::snapshotQuantity() const
{
  return map_snapshots.size();
}


Snapshot *EndPointMap::snapshot(int n) const
{
  return map_snapshots.at(n);
}


Snapshot *EndPointMap::snapshot(const QString &name)
{
  for(int i=0;i<map_snapshots.size();i++) {
    if(map_snapshots.at(i)->name()==name) {
      return map_snapshots.at(i);
    }
  }
  return NULL;
}


bool EndPointMap::load(const QString &filename,QStringList *unused_lines)
{
  SyProfile *p=new SyProfile();
  if(!p->setSource(filename)) {
    delete p;
    return false;
  }
  
  for(int i=0;i<EndPointMap::LastType;i++) {
    EndPointMap::Type type=(EndPointMap::Type)i;
    int count=0;
    QHostAddress addr;
    bool ok=false;
    map_host_addresses[type].clear();
    map_slots[type].clear();

    QString name=p->stringValue("Global","RouterType").toLower();
    map_router_type=EndPointMap::AudioRouter;
    for(int i=0;i<EndPointMap::LastRouter;i++) {
      EndPointMap::RouterType rtype=(EndPointMap::RouterType)i;
      if(EndPointMap::routerTypeString(rtype).toLower()==name) {
	map_router_type=rtype;
      }
    }
    map_router_name=p->stringValue("Global","RouterName","Livewire");
    map_router_number=p->intValue("Global","RouterNumber",1)-1;

    addr=p->addressValue(EndPointMap::typeString(type)+
	     QString::asprintf("%d",count+1),"HostAddress",QHostAddress(),&ok);
    while(ok) {
      map_host_addresses[type].push_back(addr);
      map_slots[type].push_back(p->intValue(EndPointMap::typeString(type)+
	       QString::asprintf("%d",count+1),"Slot")-1);
      map_names[type].push_back(p->stringValue(EndPointMap::typeString(type)+
	       QString::asprintf("%d",count+1),"Name"));
      count++;
      addr=p->addressValue(EndPointMap::typeString(type)+
	       QString::asprintf("%d",count+1),"HostAddress",QHostAddress(),&ok);
    }
  }

  //
  // Snapshots
  //
  QString name;
  int snap=0;
  QString section=QString::asprintf("Snapshot%d",snap+1);
  bool ok=false;

  for(int i=0;i<map_snapshots.size();i++) {
    delete map_snapshots.at(i);
  }
  map_snapshots.clear();

  name=p->stringValue(section,"Name","",&ok);
  while(ok) {
    map_snapshots.push_back(new Snapshot(name));
    int route=0;
    int output=
      p->intValue(section,QString::asprintf("Route%dOutput",route+1),0,&ok);
    while(ok) {
      map_snapshots.back()->addRoute(output,p->intValue(section,QString::asprintf("Route%dInput",route+1),0,&ok));
      route++;
      output=
	p->intValue(section,QString::asprintf("Route%dOutput",route+1),0,&ok);
    }

    snap++;
    section=QString::asprintf("Snapshot%d",snap+1);
    name=p->stringValue(section,"Name","",&ok);
  }
  if(unused_lines!=NULL) {
    *unused_lines=p->unusedLines();
  }
  delete p;

  return true;
}


bool EndPointMap::save(const QString &filename,bool incl_names) const
{
  QString tempname=filename+"-temp";
  FILE *f=NULL;

  if((f=fopen(tempname.toUtf8(),"w"))==NULL) {
    return false;
  }
  save(f,incl_names);
  fclose(f);
  if(rename(tempname.toUtf8(),filename.toUtf8())!=0) {
    unlink(tempname.toUtf8());
    return false;
  }

  return true;
}


void EndPointMap::save(FILE *f,bool incl_names) const
{
  fprintf(f,"[Global]\n");
  fprintf(f,"RouterType=%s\n",
	 (const char *)EndPointMap::routerTypeString(map_router_type).toUtf8());
  fprintf(f,"RouterName=%s\n",(const char *)map_router_name.toUtf8());
  fprintf(f,"RouterNumber=%d\n",map_router_number+1);
  fprintf(f,"\n");
  for(int i=0;i<EndPointMap::LastType;i++) {
    EndPointMap::Type type=(EndPointMap::Type)i;
    for(int j=0;j<map_host_addresses[type].size();j++) {
      fprintf(f,"[%s%d]\n",
	      (const char *)EndPointMap::typeString(type).toUtf8(),j+1);
      if(map_host_addresses[type].at(j).isNull()) {
	fprintf(f,"HostAddress=0.0.0.0\n");
      }
      else {
	fprintf(f,"HostAddress=%s\n",
	      (const char *)map_host_addresses[type].at(j).toString().toUtf8());
      }
      fprintf(f,"Slot=%d\n",map_slots[type].at(j)+1);
      if(incl_names) {
	fprintf(f,"Name=%s\n",
		(const char *)map_names[type].at(j).toUtf8());
      }
      else {
	fprintf(f,"; Name=%s\n",
		(const char *)map_names[type].at(j).toUtf8());
      }
      fprintf(f,"\n");
    }
  }

  //
  // Snapshots
  //
  for(int i=0;i<map_snapshots.size();i++) {
    fprintf(f,"[Snapshot%d]\n",i+1);
    fprintf(f,"Name=%s\n",(const char *)map_snapshots.at(i)->name().toUtf8());
    for(int j=0;j<map_snapshots.at(i)->routeQuantity();j++) {
      fprintf(f,"Route%dOutput=%d\n",j+1,map_snapshots.at(i)->routeOutput(j));
      fprintf(f,"Route%dInput=%d\n",j+1,map_snapshots.at(i)->routeInput(j));
    }
  }
}


bool EndPointMap::loadSet(QMap<int,EndPointMap *> *maps,QStringList *msgs)
{
  QDir dir(ENDPOINTMAP_MAP_DIRECTORY);
  msgs->clear();

  QStringList unused_lines;
  QStringList filter;
  filter.push_back(ENDPOINTMAP_MAP_FILTER);
  QStringList mapfiles=
    dir.entryList(filter,QDir::Files|QDir::Readable,QDir::Name);
  for(int i=0;i<mapfiles.size();i++) {
    EndPointMap *map=new EndPointMap();
    QString pathname=dir.path()+"/"+mapfiles.at(i);
    if(map->load(pathname,&unused_lines)) {
      if(unused_lines.size()>0) {
	msgs->clear();
	msgs->push_back("malformed/unused lines found in \""+pathname+"\":");
	for(int j=0;j<unused_lines.size();j++) {
	  msgs->push_back("  "+unused_lines.at(j));
	}
	return false;
      }
      for(int j=0;j<map->snapshotQuantity();j++) {
	for(int k=j+1;k<map->snapshotQuantity();k++) {
	  if(map->snapshot(j)->name()==map->snapshot(k)->name()) {
	    msgs->clear();
	    msgs->push_back("duplicate snapshot name \""+
			    map->snapshot(j)->name()+
			    "\" in \""+pathname+"\"");
	    return false;
	  }
	}
	for(int k=0;k<map->snapshot(j)->routeQuantity();k++) {
	  if(map->snapshot(j)->routeOutput(k)>=
	     map->quantity(EndPointMap::Output)) {
	    msgs->clear();
	    msgs->push_back(QString::asprintf("invalid output \"%d\"",map->snapshot(j)->routeOutput(k))+" in snapshot \""+map->snapshot(j)->name()+"\" in \""+
			    pathname+"\"");
	    return false;
	  }
	  if(map->snapshot(j)->routeInput(k)>=
	     map->quantity(EndPointMap::Input)) {
	    msgs->clear();
	    msgs->push_back(QString::asprintf("invalid input \"%d\"",map->snapshot(j)->routeInput(k))+" in snapshot \""+map->snapshot(j)->name()+"\" in \""+
			    pathname+"\"");
	    return false;
	  }
	}
      }
      for(QMap<int,EndPointMap *>::const_iterator it=maps->begin();
	  it!=maps->end();it++) {
	if(it.key()==map->routerNumber()) {
	  msgs->clear();
	  msgs->push_back(QString("duplicate SA router number ")+
			  QString::asprintf("\"%d\" ",map->routerNumber()+1)+
			  "in maps \""+it.value()->routerName()+"\" and \""+
			  map->routerName()+"\"");
	  return false;
	}
	if(it.value()->routerName()==map->routerName()) {
	  msgs->clear();
	  msgs->push_back("duplicate SA router name \""+map->routerName()+"\"");
	  return false;
	}
      }
      (*maps)[map->routerNumber()]=map;
      msgs->push_back("loaded SA map from \""+dir.path()+"/"+mapfiles.at(i)+
		      "\" "+QString::asprintf("[%d:",map->routerNumber()+1)+
		      map->routerName()+"]");
    }
  }
  return true;
}


QString EndPointMap::routerTypeString(EndPointMap::RouterType type)
{
  QString ret="Unknown";

  switch(type) {
  case EndPointMap::AudioRouter:
    ret="Audio";
    break;

  case EndPointMap::GpioRouter:
    ret="GPIO";
    break;

  case EndPointMap::LastRouter:
    break;
  }

  return ret;
}


QString EndPointMap::typeString(Type type)
{
  QString ret="Unknown";

  switch(type) {
  case EndPointMap::Input:
    ret="Input";
    break;

  case EndPointMap::Output:
    ret="Output";
    break;

  case EndPointMap::LastType:
    break;
  }

  return ret;
}
