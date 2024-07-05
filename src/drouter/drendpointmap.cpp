// drendpointmap.cpp
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

#include "drendpointmap.h"

DRSnapshot::DRSnapshot(const QString &name)
{
  snap_name=name;
}


QString DRSnapshot::name() const
{
  return snap_name;
}


void DRSnapshot::setName(const QString &str)
{
  snap_name=str;
}


int DRSnapshot::routeQuantity() const
{
  return snap_inputs.size();
}


int DRSnapshot::routeInput(int n) const
{
  return snap_inputs.at(n);
}


int DRSnapshot::routeOutput(int n) const
{
  return snap_outputs.at(n);
}


void DRSnapshot::addRoute(int output,int input)
{
  snap_outputs.push_back(output);
  snap_inputs.push_back(input);
}




DREndPointMap::DREndPointMap()
{
  map_router_type=DREndPointMap::AudioRouter;
  map_router_name="Livewire";
  map_router_number=0;
}


DREndPointMap::RouterType DREndPointMap::routerType() const
{
  return map_router_type;
}


void DREndPointMap::setRouterType(DREndPointMap::RouterType type)
{
  map_router_type=type;
}


QString DREndPointMap::routerName() const
{
  return map_router_name;
}


void DREndPointMap::setRouterName(const QString &str)
{
  map_router_name=str;
}


int DREndPointMap::routerNumber() const
{
  return map_router_number;
}


void DREndPointMap::setRouterNumber(int num)
{
  map_router_number=num;
}


int DREndPointMap::quantity(DREndPointMap::Type type) const
{
  return map_host_addresses[type].size();
}


QHostAddress DREndPointMap::hostAddress(DREndPointMap::Type type,int n) const
{
  if(n<map_host_addresses[type].size()) {
    return map_host_addresses[type].at(n);
  }
  return QHostAddress();
}


void DREndPointMap::setHostAddress(DREndPointMap::Type type,int n,const QHostAddress &addr)
{
  map_host_addresses[type][n]=addr;
}


void DREndPointMap::setHostAddress(DREndPointMap::Type type,int n,const QString &addr)
{
  map_host_addresses[type][n].setAddress(addr);
}


int DREndPointMap::slot(DREndPointMap::Type type,int n) const
{
  if(n<map_slots[type].size()) {
    return map_slots[type].at(n);
  }
  return -1;
}


void DREndPointMap::setSlot(DREndPointMap::Type type,int n,int slot)
{
  map_slots[type][n]=slot;
}


QString DREndPointMap::name(DREndPointMap::Type type,int n,const QString &orig_name) const
{
  QString ret;

  if(n<0) {
    ret=QObject::tr("OFF");
  }
  else {
    if(n<map_names[type].size()) {
      ret=map_names[type].at(n);
    }
    if(ret.isEmpty()) {
      ret=orig_name;
    }
  }

  return ret;
}


bool DREndPointMap::nameIsCustom(Type type,int n) const
{
  if(n<0) {
    return true;
  }
  if(n<map_name_is_customs[type].size()) {
    return map_name_is_customs[type].at(n);
  }
  return false;
}


void DREndPointMap::setName(DREndPointMap::Type type,int n,const QString &str)
{
  map_names[type][n]=str;
}


int DREndPointMap::endPoint(Type type,const QHostAddress &hostaddr,int slot) const
{
  for(int i=0;i<map_host_addresses[type].size();i++) {
    if((map_host_addresses[type].at(i)==hostaddr)&&
       (map_slots[type].at(i)==slot)) {
      return i;
    }
  }
  return -1;
}


int DREndPointMap::endPoint(Type type,const QString &hostaddr,int slot) const
{
  return endPoint(type,QHostAddress(hostaddr),slot);
}


void DREndPointMap::insert(DREndPointMap::Type type,int n,const QHostAddress &host_addr,int slot,
			 const QString &name)
{
  map_host_addresses[type].insert(n,host_addr);
  map_slots[type].insert(n,slot);
  map_names[type].insert(n,name);
  map_name_is_customs[type].insert(n,true);
}


void DREndPointMap::insert(DREndPointMap::Type type,int n,const QString &host_addr,int slot,
			 const QString &name)
{
  map_host_addresses[type].insert(n,QHostAddress(host_addr));
  map_slots[type].insert(n,slot);
  map_names[type].insert(n,name);
  map_name_is_customs[type].insert(n,true);
}


void DREndPointMap::erase(DREndPointMap::Type type,int n)
{
  map_host_addresses[type].erase(map_host_addresses[type].begin()+n);
  map_slots[type].erase(map_slots[type].begin()+n);
}


int DREndPointMap::snapshotQuantity() const
{
  return map_snapshots.size();
}


DRSnapshot *DREndPointMap::snapshot(int n) const
{
  return map_snapshots.at(n);
}


DRSnapshot *DREndPointMap::snapshot(const QString &name)
{
  for(int i=0;i<map_snapshots.size();i++) {
    if(map_snapshots.at(i)->name()==name) {
      return map_snapshots.at(i);
    }
  }
  return NULL;
}


bool DREndPointMap::load(const QString &filename,QStringList *unused_lines)
{
  bool ok=false;

  SyProfile *p=new SyProfile();
  if(!p->setSource(filename)) {
    delete p;
    return false;
  }
  
  for(int i=0;i<DREndPointMap::LastType;i++) {
    DREndPointMap::Type type=(DREndPointMap::Type)i;
    int count=0;
    QHostAddress addr;
    bool ok=false;
    map_host_addresses[type].clear();
    map_slots[type].clear();

    QString name=p->stringValue("Global","RouterType").toLower();
    map_router_type=DREndPointMap::AudioRouter;
    for(int i=0;i<DREndPointMap::LastRouter;i++) {
      DREndPointMap::RouterType rtype=(DREndPointMap::RouterType)i;
      if(DREndPointMap::routerTypeString(rtype).toLower()==name) {
	map_router_type=rtype;
      }
    }
    map_router_name=p->stringValue("Global","RouterName","Livewire");
    map_router_number=p->intValue("Global","RouterNumber",1)-1;

    addr=p->addressValue(DREndPointMap::typeString(type)+
	     QString::asprintf("%d",count+1),"HostAddress",QHostAddress(),&ok);
    while(ok) {
      map_host_addresses[type].push_back(addr);
      map_slots[type].push_back(p->intValue(DREndPointMap::typeString(type)+
	       QString::asprintf("%d",count+1),"Slot")-1);
      map_names[type].push_back(p->stringValue(DREndPointMap::typeString(type)+
	       QString::asprintf("%d",count+1),"Name","",&ok));
      map_name_is_customs[type].push_back(ok);
      count++;
      addr=p->addressValue(DREndPointMap::typeString(type)+
	       QString::asprintf("%d",count+1),"HostAddress",QHostAddress(),&ok);
    }
  }

  //
  // Snapshots
  //
  QString name;
  int snap=0;
  QString section=QString::asprintf("Snapshot%d",snap+1);

  for(int i=0;i<map_snapshots.size();i++) {
    delete map_snapshots.at(i);
  }
  map_snapshots.clear();

  name=p->stringValue(section,"Name","",&ok);
  while(ok) {
    map_snapshots.push_back(new DRSnapshot(name));
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


bool DREndPointMap::save(const QString &filename,bool incl_names) const
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


void DREndPointMap::save(FILE *f,bool incl_names) const
{
  fprintf(f,"[Global]\n");
  fprintf(f,"RouterType=%s\n",
	 (const char *)DREndPointMap::routerTypeString(map_router_type).toUtf8());
  fprintf(f,"RouterName=%s\n",(const char *)map_router_name.toUtf8());
  fprintf(f,"RouterNumber=%d\n",map_router_number+1);
  fprintf(f,"\n");
  for(int i=0;i<DREndPointMap::LastType;i++) {
    DREndPointMap::Type type=(DREndPointMap::Type)i;
    for(int j=0;j<map_host_addresses[type].size();j++) {
      fprintf(f,"[%s%d]\n",
	      (const char *)DREndPointMap::typeString(type).toUtf8(),j+1);
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


bool DREndPointMap::loadSet(QMap<int,DREndPointMap *> *maps,QStringList *msgs)
{
  QDir dir(DRENDPOINTMAP_MAP_DIRECTORY);
  msgs->clear();

  QStringList unused_lines;
  QStringList filter;
  filter.push_back(DRENDPOINTMAP_MAP_FILTER);
  QStringList mapfiles=
    dir.entryList(filter,QDir::Files|QDir::Readable,QDir::Name);
  for(int i=0;i<mapfiles.size();i++) {
    DREndPointMap *map=new DREndPointMap();
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
	  if(map->snapshot(j)->routeOutput(k)>
	     map->quantity(DREndPointMap::Output)) {
	    msgs->clear();
	    msgs->push_back(QString::asprintf("invalid output \"%d\"",map->snapshot(j)->routeOutput(k))+" in snapshot \""+map->snapshot(j)->name()+"\" in \""+
			    pathname+"\"");
	    return false;
	  }
	  if(map->snapshot(j)->routeInput(k)>
	     map->quantity(DREndPointMap::Input)) {
	    msgs->clear();
	    msgs->push_back(QString::asprintf("invalid input \"%d\"",map->snapshot(j)->routeInput(k))+" in snapshot \""+map->snapshot(j)->name()+"\" in \""+
			    pathname+"\"");
	    return false;
	  }
	}
      }
      for(QMap<int,DREndPointMap *>::const_iterator it=maps->begin();
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


QString DREndPointMap::routerTypeString(DREndPointMap::RouterType type)
{
  QString ret="Unknown";

  switch(type) {
  case DREndPointMap::AudioRouter:
    ret="Audio";
    break;

  case DREndPointMap::GpioRouter:
    ret="GPIO";
    break;

  case DREndPointMap::LastRouter:
    break;
  }

  return ret;
}


QString DREndPointMap::typeString(Type type)
{
  QString ret="Unknown";

  switch(type) {
  case DREndPointMap::Input:
    ret="Input";
    break;

  case DREndPointMap::Output:
    ret="Output";
    break;

  case DREndPointMap::LastType:
    break;
  }

  return ret;
}
