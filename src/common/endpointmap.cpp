// endpointmap.cpp
//
// Map integers to DRouter endpoints.
//
// (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#include <sy/syprofile.h>

#include "endpointmap.h"

EndPointMap::EndPointMap()
{
  map_router_name="Livewire";
  map_router_number=0;
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
  return map_host_addresses[type].at(n);
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
  return map_slots[type].at(n);
}


void EndPointMap::setSlot(EndPointMap::Type type,int n,int slot)
{
  map_slots[type][n]=slot;
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


void EndPointMap::insert(EndPointMap::Type type,int n,const QHostAddress &host_addr,int slot)
{
  map_host_addresses[type].insert(n,host_addr);
  map_slots[type].insert(n,slot);
}


void EndPointMap::insert(EndPointMap::Type type,int n,const QString &host_addr,int slot)
{
  map_host_addresses[type].insert(n,QHostAddress(host_addr));
  map_slots[type].insert(n,slot);
}


void EndPointMap::erase(EndPointMap::Type type,int n)
{
  map_host_addresses[type].erase(map_host_addresses[type].begin()+n);
  map_slots[type].erase(map_slots[type].begin()+n);
}


bool EndPointMap::load(const QString &filename)
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

    map_router_name=p->stringValue("Global","RouterName","Livewire");
    map_router_number=p->intValue("Global","RouterNumber",1)-1;

    addr=p->addressValue(EndPointMap::typeString(type)+
	     QString().sprintf("%d",count+1),"HostAddress",QHostAddress(),&ok);
    while(ok) {
      map_host_addresses[type].push_back(addr);
      map_slots[type].push_back(p->intValue(EndPointMap::typeString(type)+
	       QString().sprintf("%d",count+1),"Slot")-1);
      count++;
      addr=p->addressValue(EndPointMap::typeString(type)+
	       QString().sprintf("%d",count+1),"HostAddress",QHostAddress(),&ok);
    }
  }
  delete p;
  return true;
}


bool EndPointMap::save(const QString &filename) const
{
  QString tempname=filename+"-temp";
  FILE *f=NULL;

  if((f=fopen(tempname.toUtf8(),"w"))==NULL) {
    return false;
  }
  save(f);
  fclose(f);
  if(rename(tempname.toUtf8(),filename.toUtf8())!=0) {
    unlink(tempname.toUtf8());
    return false;
  }

  return true;
}


void EndPointMap::save(FILE *f) const
{
  fprintf(f,"[Global]\n");
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
      fprintf(f,"\n");
    }
  }
}


QString EndPointMap::typeString(Type type)
{
  QString ret="Unknown";

  switch(type) {
  case EndPointMap::Src:
    ret="Source";
    break;

  case EndPointMap::Dst:
    ret="Destination";
    break;

  case EndPointMap::LastType:
    break;
  }

  return ret;
}
