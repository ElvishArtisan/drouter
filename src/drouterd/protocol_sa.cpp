// protocol_sa.cpp
//
// Software Authority Protocol
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

#include <syslog.h>

#include <QDir>

#include <sy/syrouting.h>

#include "protocol_sa.h"

ProtocolSa::ProtocolSa(DRouter *router,QObject *parent)
  : Protocol(router,Protocol::ProtocolSa,parent)
{
  LoadMaps();

  sa_server=new ServerSa(this);
  connect(sa_server,SIGNAL(sendMatrixNames(int)),
	  this,SLOT(sendMatrixNamesSa(int)));
  connect(sa_server,SIGNAL(sendInputNames(int,unsigned)),
	  this,SLOT(sendInputNamesSa(int,unsigned)));
  connect(sa_server,SIGNAL(sendOutputNames(int,unsigned)),
	  this,SLOT(sendOutputNamesSa(int,unsigned)));
  connect(sa_server,SIGNAL(setRoute(int,unsigned,unsigned,unsigned)),
	  this,SLOT(setRouteSa(int,unsigned,unsigned,unsigned)));
  connect(sa_server,SIGNAL(sendRouteInfo(int,unsigned,unsigned)),
	  this,SLOT(sendRouteInfoSa(int,unsigned,unsigned)));
  connect(sa_server,SIGNAL(sendGpiInfo(int,unsigned,int)),
	  this,SLOT(sendGpiInfoSa(int,unsigned,int)));
  connect(sa_server,SIGNAL(sendGpoInfo(int,unsigned,int)),
	  this,SLOT(sendGpoInfoSa(int,unsigned,int)));
  connect(sa_server,
	  SIGNAL(setGpiState(int,unsigned,unsigned,int,const QString &)),
	  this,SLOT(setGpiStateSa(int,unsigned,unsigned,int,
				       const QString &)));
  connect(sa_server,
	  SIGNAL(setGpoState(int,unsigned,unsigned,int,const QString &)),
	  this,SLOT(setGpoStateSa(int,unsigned,unsigned,int,
				       const QString &)));

  sa_server->setReady(true);
}


void ProtocolSa::setReady(bool state)
{
  sa_server->setReady(state);
}


void ProtocolSa::inputNameChanged(int mid,unsigned input,const QString &name)
{
}


void ProtocolSa::gpiChanged(int mid,unsigned input,const QString &code)
{
  sa_server->
    send(QString().sprintf("GPIStat %d %u ",mid+1,input)+code.toLower()+
	 "\r\n");
  sa_server->send(">>");
}


void ProtocolSa::outputNameChanged(int mid,unsigned output,const QString &name)
{
}


void ProtocolSa::outputCrosspointChanged(int mid,unsigned output,unsigned input)
{
  sa_server->send(QString().sprintf("RouteStat %u %u %u False\r\n",
				     mid+1,output,input));
  sa_server->send(">>");
}


void ProtocolSa::gpoChanged(int mid,unsigned output,const QString &code)
{
  sa_server->
    send(QString().sprintf("GPOStat %d %u ",mid+1,output)+code.toLower()+
	 "\r\n");
  sa_server->send(">>");
}


void ProtocolSa::processChangedDestination(const SyNode &node,int slot,
					   const SyDestination &dst)
{
  int input;
  int output;
  int src_slot=-1;
  SyLwrpClient *src_lwrp;

  for(QMap<int,EndPointMap *>::const_iterator it=sa_maps.begin();
      it!=sa_maps.end();it++) {
    EndPointMap *map=it.value();
    if((output=map->endPoint(EndPointMap::Output,node.hostAddress(),slot))>=0) {
      if((src_lwrp=router()->
	  nodeBySrcStream(dst.streamAddress(),&src_slot))!=NULL) {
	if(src_slot<0) {  // Crosspoint is OFF
	}
	else {
	  if((input=map->endPoint(EndPointMap::Input,src_lwrp->hostAddress(),src_slot))<0) {
	  }
	  else {
	    sa_server->
	      send(QString().sprintf("RouteStat %u %u %u False\r\n",
				     map->routerNumber()+1,output+1,input+1));
	  }
	}
      }
    }
  }
}


void ProtocolSa::processChangedGpi(const SyNode &node,int slot,
				   const SyGpioBundle &gpi)
{
  int input;

  if(gpi.code().contains("L")||gpi.code().contains("H")) {
    for(QMap<int,EndPointMap *>::const_iterator it=sa_maps.begin();
	it!=sa_maps.end();it++) {
      EndPointMap *map=it.value();
      if((input=map->endPoint(EndPointMap::Input,node.hostAddress(),slot))>=0) {
	sa_server->send(QString().
			sprintf("GPIStat %u %d ",map->routerNumber()+1,input+1)+
			gpi.code().toLower()+"\r\n");
      }
    }
  }
}


void ProtocolSa::processChangedGpo(const SyNode &node,int slot,const SyGpo &gpo)
{
  int input;
  int output;

  //
  // Process GPO State
  //
  if(gpo.bundle()->code().contains("L")||gpo.bundle()->code().contains("H")) {
    for(QMap<int,EndPointMap *>::const_iterator it=sa_maps.begin();
	it!=sa_maps.end();it++) {
      EndPointMap *map=it.value();
      if((output=map->endPoint(EndPointMap::Output,node.hostAddress(),slot))>=
	 0) {
	sa_server->send(QString().
			sprintf("GPOStat %u %d ",
				map->routerNumber()+1,output+1)+
			gpo.bundle()->code().toLower()+"\r\n");
      }
    }
    return;
  }

  //
  // Process GPIO Route Change
  //
  for(QMap<int,EndPointMap *>::const_iterator it=sa_maps.begin();
      it!=sa_maps.end();it++) {
    EndPointMap *map=it.value();
    if((output=map->endPoint(EndPointMap::Output,node.hostAddress(),slot))>=0) {
      if((input=map->endPoint(EndPointMap::Input,gpo.sourceAddress(),
			      gpo.sourceSlot()))>=0) {
	sa_server->
	  send(QString().sprintf("RouteStat %u %u %u False\r\n",
				 map->routerNumber()+1,output+1,input+1));
      }
      else {
	sa_server->
	  send(QString().sprintf("RouteStat %u %u 0 False\r\n",
				 map->routerNumber()+1,output+1));
      }
    }
  }
}


void ProtocolSa::sendMatrixNamesSa(int id)
{
  sa_server->send("Begin RouterNames\r\n");
  for(QMap<int,EndPointMap *>::const_iterator it=sa_maps.begin();
      it!=sa_maps.end();it++) {
    sa_server->send(QString().sprintf("    %d ",it.value()->routerNumber()+1)+
		    it.value()->routerName()+"\r\n");
  }
  sa_server->send("End RouterNames\r\n");
  sa_server->send(">>",id);
}


void ProtocolSa::sendInputNamesSa(int id,unsigned mid)
{
  EndPointMap *map=NULL;
  SyLwrpClient *lwrp=NULL;

  if((map=sa_maps.value(mid))==NULL) {
    sa_server->send("Error - Bay Does Not exist.\r\n",id);
    sa_server->send(">>",id);
    return;
  }
  sa_server->send(QString().sprintf("Begin SourceNames - %u\r\n",mid+1),id);
  for(int i=0;i<map->quantity(EndPointMap::Input);i++) {
    bool online=false;
    if((lwrp=router()->node(map->hostAddress(EndPointMap::Input,i)))!=NULL) {
      int slot=map->slot(EndPointMap::Input,i);
      if(sa_maps[mid]->routerType()==EndPointMap::AudioRouter) {
	if(slot<(int)lwrp->srcSlots()) {
	  QString name=lwrp->srcName(slot);
	  if(name.isEmpty()) {
	    name=QString().sprintf("SRC %u",slot+1);
	  }
	  sa_server->
	    send(QString().sprintf("    %u",i+1)+
		 "\t"+name+
		 "\t"+name+" ON "+lwrp->hostName()+
		 "\t"+lwrp->hostAddress().toString()+
		 "\t"+lwrp->hostName()+
		 "\t"+QString().sprintf("%u",slot+1)+
		 "\t"+QString().sprintf("%d",lwrp->srcNumber(slot))+
		 "\t"+lwrp->srcAddress(slot).toString()+
		 "\r\n",id);
	  online=true;
	}
      }
      if(sa_maps[mid]->routerType()==EndPointMap::GpioRouter) {
	if(slot<(int)lwrp->gpis()) {
	  QString name=QString().sprintf("GPI %u",slot+1);
	  sa_server->
	    send(QString().sprintf("    %u",i+1)+
		 "\t"+name+
		 "\t"+name+" ON "+lwrp->hostName()+
		 "\t"+lwrp->hostAddress().toString()+
		 "\t"+lwrp->hostName()+
		 "\t"+QString().sprintf("%u",slot+1)+
		 "\t"+lwrp->hostAddress().toString()+
		 QString().sprintf("/%d",slot+1)+
		 "\t0"+
		 "\r\n",id);
	  online=true;
	}
      }
    }
    if(!online) {
      sa_server->
	send(QString().sprintf("    %u",i+1)+
	     "\t[unavailable]"+
	     "\t[unavailable]"+
	     "\t"+map->hostAddress(EndPointMap::Input,i).toString()+
	     "\t[unavailable]"+
	     "\t"+QString().sprintf("%u",map->slot(EndPointMap::Input,i)+1)+
	     "\t0"+
	     "\t0.0.0.0"+
	     "\r\n",id);
    }
  }
  sa_server->send(QString().sprintf("End SourceNames - %u\r\n",mid+1),id);
  sa_server->send(">>",id);
}


void ProtocolSa::sendOutputNamesSa(int id,unsigned mid)
{
  EndPointMap *map=NULL;
  SyLwrpClient *lwrp=NULL;

  if((map=sa_maps.value(mid))==NULL) {
    sa_server->send("Error - Bay Does Not exist.\r\n",id);
    sa_server->send(">>",id);
    return;
  }
  sa_server->send(QString().sprintf("Begin DestNames - %u\r\n",mid+1),id);
  for(int i=0;i<map->quantity(EndPointMap::Output);i++) {
    bool online=false;
    if((lwrp=router()->node(map->hostAddress(EndPointMap::Output,i)))!=NULL) {
      int slot=map->slot(EndPointMap::Output,i);
      if(sa_maps[mid]->routerType()==EndPointMap::AudioRouter) {
	if(slot<(int)lwrp->dstSlots()) {
	  QString name=lwrp->dstName(slot);
	  if(name.isEmpty()) {
	    name=QString().sprintf("DST %u",slot+1);
	  }
	  sa_server->
	    send(QString().sprintf("    %u",i+1)+
		 "\t"+name+
		 "\t"+name+" ON "+lwrp->hostName()+
		 "\t"+lwrp->hostAddress().toString()+
		 "\t"+lwrp->hostName()+
		 "\t"+QString().sprintf("%d",slot+1)+
		 "\r\n",id);
	  online=true;
	}
      }
      if(sa_maps[mid]->routerType()==EndPointMap::GpioRouter) {
	if(slot<(int)lwrp->gpos()) {
	  QString name=lwrp->gpo(slot)->name();
	  if(name.isEmpty()) {
	    name=QString().sprintf("GPO %u",slot+1);
	  }
	  sa_server->
	    send(QString().sprintf("    %u",i+1)+
		 "\t"+name+
		 "\t"+name+" ON "+lwrp->hostName()+
		 "\t"+lwrp->hostAddress().toString()+
		 "\t"+lwrp->hostName()+
		 "\t"+QString().sprintf("%d",slot+1)+
		 "\r\n",id);
	  online=true;
	}
      }
    }
    if(!online) {
      sa_server->
	send(QString().sprintf("    %u",i+1)+
	     "\t[unavailable]"+
	     "\t[unavailable]"+
	     "\t"+map->hostAddress(EndPointMap::Output,i).toString()+
	     "\t[unavailable]"+
	     "\t"+QString().sprintf("%u",map->slot(EndPointMap::Output,i)+1)+
	     "\r\n",id);
    }
  }
  sa_server->send(QString().sprintf("End DestNames - %u\r\n",mid+1),id);
  sa_server->send(">>",id);
}


void ProtocolSa::setRouteSa(int id,unsigned mid,unsigned input,
				 unsigned output)
{
  //
  // 'input' is 1-based here, *not* 0-based!  '0' means 'off'
  //
  EndPointMap *map=NULL;

  //
  // Get the map
  //
  if((map=sa_maps.value(mid))==NULL) {
    sa_server->send("Error - Router Does Not exist.\r\n",id);
    sa_server->send(">>",id);
    return;
  }
  if(output>=(unsigned)map->quantity(EndPointMap::Output)) {
    sa_server->send("Error - Output Does Not exist.\r\n",id);
    return;
  }
  if(input>(unsigned)map->quantity(EndPointMap::Input)) {
    sa_server->send("Error - Input Does Not exist.\r\n",id);
    return;
  }

  if(map->routerType()==EndPointMap::AudioRouter) {
    //
    // Get the destination
    //
    QHostAddress dst_hostaddr=map->hostAddress(EndPointMap::Output,output);
    int dst_slot=map->slot(EndPointMap::Output,output);
    SyLwrpClient *dst_lwrp=router()->node(dst_hostaddr);
    if((dst_lwrp==NULL)||(dst_slot<0)||(dst_slot>=(int)dst_lwrp->dstSlots())) {
      sa_server->send("Error - Output Does Not exist.\r\n",id);
      return;
    }

    //
    // Get the source
    //
    if(input==0) {
      if(router()->clearCrosspoint(dst_hostaddr,dst_slot)) {
	sa_server->send("Route Change Initiated\r\n",id);
      }
      return;
    }
    else {
      QHostAddress src_hostaddr=map->hostAddress(EndPointMap::Input,input-1);
      int src_slot=map->slot(EndPointMap::Input,input-1);
      SyLwrpClient *src_lwrp=router()->node(src_hostaddr);
      if((src_lwrp==NULL)||(src_slot<0)||(src_slot>=(int)src_lwrp->srcSlots())) {
	if(router()->clearCrosspoint(dst_hostaddr,dst_slot)) {
	  sa_server->send("Route Change Initiated\r\n",id);
	  return;
	}
      }
      if(router()->setCrosspoint(dst_hostaddr,dst_slot,src_hostaddr,src_slot)) {
	sa_server->send("Route Change Initiated\r\n",id);
      }
    }
  }
  if(map->routerType()==EndPointMap::GpioRouter) {
    //
    // Get the GPO
    //
    QHostAddress gpo_hostaddr=map->hostAddress(EndPointMap::Output,output);
    int gpo_slot=map->slot(EndPointMap::Output,output);
    SyLwrpClient *gpo_lwrp=router()->node(gpo_hostaddr);
    if((gpo_lwrp==NULL)||(gpo_slot<0)||(gpo_slot>=(int)gpo_lwrp->gpos())) {
      sa_server->send("Error - Output Does Not exist.\r\n",id);
      return;
    }

    //
    // Get the source
    //
    if(input==0) {
      if(router()->clearGpioCrosspoint(gpo_hostaddr,gpo_slot)) {
	sa_server->send("Route Change Initiated\r\n",id);
      }
      return;
    }
    else {
      QHostAddress gpi_hostaddr=map->hostAddress(EndPointMap::Input,input-1);
      int gpi_slot=map->slot(EndPointMap::Input,input-1);
      SyLwrpClient *gpi_lwrp=router()->node(gpi_hostaddr);
      if((gpi_lwrp==NULL)||(gpi_slot<0)||(gpi_slot>=(int)gpi_lwrp->gpis())) {
	if(router()->clearGpioCrosspoint(gpi_hostaddr,gpi_slot)) {
	  sa_server->send("Route Change Initiated\r\n",id);
	  return;
	}
      }
      if(router()->setGpioCrosspoint(gpo_hostaddr,gpo_slot,
				     gpi_hostaddr,gpi_slot)) {
	sa_server->send("Route Change Initiated\r\n",id);
      }
    }
  }
}


void ProtocolSa::sendRouteInfoSa(int id,unsigned mid,unsigned output)
{
  //
  // 'output' is 1-based here, *not* 0-based!  '0' means 'all outputs'
  //
  EndPointMap *map=NULL;

  //
  // Get the map
  //
  if((map=sa_maps.value(mid))==NULL) {
    sa_server->send("Error - Router Does Not exist.\r\n",id);
    sa_server->send(">>",id);
    return;
  }
  if(output>(unsigned)map->quantity(EndPointMap::Output)) {
    sa_server->send("Error - Output Does Not exist.\r\n",id);
    return;
  }

  if(output==0) {
    for(int i=0;i<map->quantity(EndPointMap::Output);i++) {
      sa_server->
	send(QString().sprintf("RouteStat %u %u %u False\r\n",
			       mid+1,i+1,GetCrosspointInput(map,i)+1),id);
    }
  }
  else {
    sa_server->send(QString().sprintf("RouteStat %u %u %u False\r\n",
		    mid+1,output,GetCrosspointInput(map,output-1)+1),id);
  }
}


void ProtocolSa::sendGpiInfoSa(int id,unsigned mid,int input)
{
  SyGpioBundle *gpi;
  EndPointMap *map;

  if((map=sa_maps.value(mid))==NULL) {
    sa_server->send("Error - Router Does Not exist.\r\n",id);
    sa_server->send(">>",id);
    return;
  }
  if(map->routerType()!=EndPointMap::GpioRouter) {
    sa_server->send(">>",id);
    return;
  }
  sa_server->send(">>",id);
  if(input<0) {
    for(int i=0;i<map->quantity(EndPointMap::Input);i++) {
      if((gpi=router()->gpi(map->hostAddress(EndPointMap::Input,i),
			    map->slot(EndPointMap::Input,i)))!=NULL) {
	sa_server->send(QString().sprintf("GPIStat %u %d ",mid+1,i+1)+
			gpi->code().toLower()+"\r\n",id);
      }
    }
  }
  else {
    if((gpi=router()->gpi(map->hostAddress(EndPointMap::Input,input-1),
			  map->slot(EndPointMap::Input,input-1)))!=NULL) {
      sa_server->send(QString().sprintf("GPIStat %u %d ",mid+1,input)+
		      gpi->code().toLower()+"\r\n",id);
    }
  }
}


void ProtocolSa::sendGpoInfoSa(int id,unsigned mid,int output)
{
  SyGpo *gpo;
  EndPointMap *map;

  if((map=sa_maps.value(mid))==NULL) {
    sa_server->send("Error - Router Does Not exist.\r\n",id);
    sa_server->send(">>",id);
    return;
  }
  if(map->routerType()!=EndPointMap::GpioRouter) {
    sa_server->send(">>",id);
    return;
  }
  sa_server->send(">>",id);
  if(output<0) {
    for(int i=0;i<map->quantity(EndPointMap::Output);i++) {
      if((gpo=router()->gpo(map->hostAddress(EndPointMap::Output,i),
			    map->slot(EndPointMap::Output,i)))!=NULL) {
	sa_server->send(QString().sprintf("GPOStat %u %d ",mid+1,i+1)+
			gpo->bundle()->code().toLower()+"\r\n",id);
      }
    }
  }
  else {
    if((gpo=router()->gpo(map->hostAddress(EndPointMap::Output,output-1),
			  map->slot(EndPointMap::Output,output-1)))!=NULL) {
      sa_server->send(QString().sprintf("GPOStat %u %d ",mid+1,output)+
		      gpo->bundle()->code().toLower()+"\r\n",id);
    }
  }
}


void ProtocolSa::setGpiStateSa(int id,unsigned mid,unsigned input,
				     int msecs,const QString &code)
{
  SyLwrpClient *gpi_lwrp;
  EndPointMap *map;

  if((map=sa_maps.value(mid))==NULL) {
    sa_server->send("Error - Router Does Not exist.\r\n",id);
    sa_server->send(">>",id);
    return;
  }
  if(map->routerType()!=EndPointMap::GpioRouter) {
    sa_server->send(">>",id);
    return;
  }
  if((gpi_lwrp=router()->node(map->hostAddress(EndPointMap::Input,input)))!=
     NULL) {
    sa_server->send(">>",id);
    gpi_lwrp->setGpiCode(map->slot(EndPointMap::Input,input),code);
  }
}


void ProtocolSa::setGpoStateSa(int id,unsigned mid,unsigned output,
				     int msecs,const QString &code)
{
  SyLwrpClient *gpo_lwrp;
  EndPointMap *map;

  if((map=sa_maps.value(mid))==NULL) {
    sa_server->send("Error - Router Does Not exist.\r\n",id);
    sa_server->send(">>",id);
    return;
  }
  if(map->routerType()!=EndPointMap::GpioRouter) {
    sa_server->send(">>",id);
    return;
  }
  if((gpo_lwrp=router()->node(map->hostAddress(EndPointMap::Output,output)))!=
     NULL) {
    sa_server->send(">>",id);
    gpo_lwrp->setGpoCode(map->slot(EndPointMap::Output,output),code);
  }
}


void ProtocolSa::LoadMaps()
{
  QDir dir(SERVER_SA_CONFIG_DIR);

  QStringList filter;
  filter.push_back("*.conf");
  QStringList mapfiles=
    dir.entryList(filter,QDir::Files|QDir::Readable,QDir::Name);
  for(int i=0;i<mapfiles.size();i++) {
    EndPointMap *map=new EndPointMap();
    if(map->load(dir.path()+"/"+mapfiles.at(i))) {
      for(QMap<int,EndPointMap *>::const_iterator it=sa_maps.begin();
	  it!=sa_maps.end();it++) {
	if(it.key()==map->routerNumber()) {
	  fprintf(stderr,"drouterd: duplicate SA router number\n");
	  exit(1);
	}
	if(it.value()->routerName()==map->routerName()) {
	  fprintf(stderr,"drouterd: duplicate SA router name\n");
	  exit(1);
	}
      }
      sa_maps[map->routerNumber()]=map;
      syslog(LOG_DEBUG,"loaded SA map from \"%s/%s\" [%d:%s]",
	     (const char *)dir.path().toUtf8(),
	     (const char *)mapfiles.at(i).toUtf8(),
	     map->routerNumber()+1,
	     (const char *)map->routerName().toUtf8());
    }
  }
  syslog(LOG_INFO,"loaded %d SA map(s)",sa_maps.size());
}


int ProtocolSa::GetCrosspointInput(EndPointMap *map,int output) const
{
  SyLwrpClient *src_lwrp=NULL;
  SyGpo *gpo=NULL;

  if(map->routerType()==EndPointMap::AudioRouter) {
    //
    // Get the destination
    //
    QHostAddress dst_hostaddr=map->hostAddress(EndPointMap::Output,output);
    int dst_slot=map->slot(EndPointMap::Output,output);
    SyLwrpClient *dst_lwrp=router()->node(dst_hostaddr);
    if((dst_lwrp==NULL)||(dst_slot<0)||(dst_slot>=(int)dst_lwrp->dstSlots())) {
      return -1;
    }

    //
    // Get the source
    //
    int src_slot=0;
    QHostAddress src_strmaddr=dst_lwrp->dstAddress(dst_slot);
    if(src_strmaddr.isNull()) {  // OFF
      return -1;
    }
    else {
      if((src_lwrp=router()->nodeBySrcStream(src_strmaddr,&src_slot))==NULL) {
	return -1;  // OFF-LINE
      }
    }
    return map->endPoint(EndPointMap::Input,src_lwrp->hostAddress(),src_slot);
  }
  if(map->routerType()==EndPointMap::GpioRouter) {
    //
    // Get the destination
    //
    QHostAddress gpo_hostaddr=map->hostAddress(EndPointMap::Output,output);
    int gpo_slot=map->slot(EndPointMap::Output,output);
    SyLwrpClient *gpo_lwrp=router()->node(gpo_hostaddr);
    if((gpo_lwrp==NULL)||(gpo_slot<0)||(gpo_slot>=(int)gpo_lwrp->gpos())) {
      return -1;
    }
    if((gpo=gpo_lwrp->gpo(gpo_slot))!=NULL) {
      return map->
	endPoint(EndPointMap::Input,gpo->sourceAddress(),gpo->sourceSlot());
    }
  }
  return -1;
}
