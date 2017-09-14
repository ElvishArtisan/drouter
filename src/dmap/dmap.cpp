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

#include <sy/sycmdswitch.h>
#include <sy/syconfig.h>
#include <sy/syprofile.h>

#include "dmap.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  bool ok=false;

  map_verbose=false;
  map_output_map="";
  map_node_password="";
  map_no_off_source=false;
  map_router_number=0;
  map_router_name="Livewire";
  map_max_nodes=0;
  map_current_id=0;
  map_retry_count=0;
  map_scan_only=false;
  map_scan_duration=DMAP_DEFAULT_SCAN_DURATION;

  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"dmap",VERSION,DMAP_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--max-nodes") {
      map_max_nodes=cmd->value(i).toInt(&ok);
      if((!ok)||(map_max_nodes<=0)) {
	fprintf(stderr,"dmap: invalid --max-nodes\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--node-password") {
      map_node_password=cmd->value(i);
      cmd->setProcessed(i,true);
    }
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
    if(cmd->key(i)=="--scan-duration") {
      map_scan_duration=cmd->value(i).toInt(&ok)*1000;
      if((!ok)||(map_scan_duration<0)) {
	fprintf(stderr,"dmap: invalid --scan-duration\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--scan-only") {
      map_scan_only=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--skip-node") {
      map_skip_node_addresses.
	push_back(QHostAddress(cmd->value(i)).toIPv4Address());
      if(map_skip_node_addresses.back()==0) {
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
      map_node_addresses.push_back(QHostAddress(cmd->value(i)).toIPv4Address());
      if(map_node_addresses.back()==0) {
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
    if(cmd->key(i)=="--verbose") {
      map_verbose=true;
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
  map_map->setRouterName(map_router_name);
  map_map->setRouterNumber(map_router_number);

  //
  // Advertising Socket
  //
  map_advert_socket=new SyMcastSocket(SyMcastSocket::ReadOnly,this);
  connect(map_advert_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));

  //
  // Timers
  //
  map_scan_timer=new QTimer(this);
  map_scan_timer->setSingleShot(true);
  connect(map_scan_timer,SIGNAL(timeout()),this,SLOT(scanTimeoutData()));

  map_garbage_timer=new QTimer(this);
  map_garbage_timer->setSingleShot(true);
  connect(map_garbage_timer,SIGNAL(timeout()),this,SLOT(garbageTimeoutData()));

  map_connection_timer=new QTimer(this);
  map_connection_timer->setSingleShot(true);
  connect(map_connection_timer,SIGNAL(timeout()),
	  this,SLOT(connectionTimeoutData()));

  //
  // Start the Scan
  //
  if(map_node_addresses.size()>0) {
    Verbose("--use-node value(s) given, skipping scan stage\n");
    startNodeProcessing();
  }
  else {
    map_advert_socket->bind(SWITCHYARD_ADVERTS_PORT);
    map_advert_socket->subscribe(SWITCHYARD_ADVERTS_ADDRESS);
    map_scan_timer->start(map_scan_duration);
    Verbose("Scanning for nodes...\n");
  }
}


void MainObject::startNodeProcessing()
{
  Verbose("Purging skipped node addresses...");
  PurgeSkippedNodes();
  Verbose("done.\n");
  Verbose("Sorting node addresses...");
  SortAddresses(map_node_addresses);
  Verbose("done.\n");
  if(map_node_addresses.size()==0) {
    Verbose("No nodes found.\n");
    exit(0);
  }
  if(map_scan_only) {
    Verbose("\n");
    DumpNodeList();
    exit(0);
  }
  else {
    Verbose("Probing nodes...\n");
    if(!map_no_off_source) {
      map_map->insert(EndPointMap::Src,0,QHostAddress(),-1);
    }
    map_current_id=0;
    map_connection_timer->start(0);
  }
}


void MainObject::readyReadData()
{
  QHostAddress addr;
  char data[1500];
  int n;

  if(map_advert_socket!=NULL) {
    while((n=map_advert_socket->readDatagram(data,1500,&addr))>0) {
      uint32_t addr_int=addr.toIPv4Address();
      bool found=false;
      for(unsigned i=0;i<map_node_addresses.size();i++) {
	found=found||(addr_int==map_node_addresses[i]);
      }
      if(!found) {
	Verbose("  detected node at: "+addr.toString()+"\n");
	if((map_max_nodes==0)||((int)map_node_addresses.size()<map_max_nodes)) {
	  map_node_addresses.push_back(addr_int);
	  if((map_max_nodes>0)&&((int)map_node_addresses.size()==map_max_nodes)) {
	    map_scan_timer->stop();
	    map_scan_timer->start(0);
	  }
	}
      }
    }
  }
}


void MainObject::scanTimeoutData()
{
  delete map_advert_socket;
  map_advert_socket=NULL;
  Verbose("Scan phase complete.\n");
  startNodeProcessing();
}


void MainObject::garbageTimeoutData()
{
  QString err_text;

  delete map_lwrp;
  if(map_current_id<map_node_addresses.size()) {  // Next Node
    map_connection_timer->start(0);
  }
  else {  // Done, dump the map and exit
    Verbose("\n");
    Verbose(QString().
	    sprintf("Found %u sources, %u destinations\n",
		    map_map->quantity(EndPointMap::Src),
		    map_map->quantity(EndPointMap::Dst)));
    map_map->save(map_output_map);
    /*
    if(!map_map->validate(&err_text)) {
      fprintf(stderr,"dmap: WARNING map failed to validate: %s\n",
	      (const char *)err_text.toUtf8());
      exit(256);
    }
    */
    exit(0);
  }
}


void MainObject::connectionTimeoutData()
{
  if(map_retry_count==0) {
    Verbose(QHostAddress(map_node_addresses[map_current_id]).toString()+": ");
  }
  else {
    Verbose("[retrying] ");
  }
  map_lwrp=new SyLwrpClient(map_current_id,this);
  connect(map_lwrp,SIGNAL(connected(unsigned,bool)),
	  this,SLOT(nodeConnectedData(unsigned,bool)));
  connect(map_lwrp,
	  SIGNAL(connectionError(unsigned,QAbstractSocket::SocketError)),
	  this,SLOT(nodeErrorData(unsigned,QAbstractSocket::SocketError)));
  map_lwrp->connectToHost(QHostAddress(map_node_addresses[map_current_id]),
			  SWITCHYARD_LWRP_PORT,map_node_password);
}


void MainObject::nodeConnectedData(unsigned id,bool state)
{
  if(state) {
    /*
    map_map->addNode(map_lwrp->hostAddress(),map_lwrp->deviceName());
    map_map->node(map_map->nodeQuantity()-1)->setPassword(map_node_password);
    */
    Verbose(QString().sprintf(" src: %u  dst: %u  gpis: %u  gpos: %u\n",
			      map_lwrp->srcSlots(),
			      map_lwrp->dstSlots(),
			      map_lwrp->gpis(),
			      map_lwrp->gpos()));
    for(unsigned i=0;i<map_lwrp->srcSlots();i++) {
      map_map->insert(EndPointMap::Src,map_map->quantity(EndPointMap::Src),
		      map_lwrp->hostAddress(),i);
    }
    for(unsigned i=0;i<map_lwrp->dstSlots();i++) {
      map_map->insert(EndPointMap::Dst,map_map->quantity(EndPointMap::Dst),
		      map_lwrp->hostAddress(),i);
    }
    map_retry_count=0;
    map_current_id++;
    map_garbage_timer->start(0);
  }
}


void MainObject::nodeErrorData(unsigned id,QAbstractSocket::SocketError err)
{
  QString msg=tr("unknown network error")+QString().sprintf(" [%d]",err);
  switch(err) {
  case QAbstractSocket::ConnectionRefusedError:
    msg=tr("connection refused");
    map_retry_count=DMAP_CONNECTION_RETRY_LIMIT;
    break;

  case QAbstractSocket::RemoteHostClosedError:
    msg=tr("remote host closed connection");
    map_retry_count++;
    break;

  case QAbstractSocket::HostNotFoundError:
    msg=tr("host not found");
    map_retry_count++;
    break;

  case QAbstractSocket::SocketAccessError:
    msg=tr("socket access error");
    map_retry_count++;
    break;

  case QAbstractSocket::SocketTimeoutError:
    msg=tr("timed out");
    map_retry_count++;
    break;

  case QAbstractSocket::DatagramTooLargeError:
    msg=tr("datagram too large");
    map_retry_count++;
    break;

  case QAbstractSocket::NetworkError:
    msg=tr("network error");
    map_retry_count++;
    break;

  case QAbstractSocket::AddressInUseError:
    msg=tr("address in use");
    map_retry_count++;
    break;

  default:
    map_retry_count++;
    break;
  }
  if(map_retry_count>=DMAP_CONNECTION_RETRY_LIMIT) {  // Move on to the next node
    Verbose(msg+"\n");
    map_retry_count=0;
    map_current_id++;
  }
  map_garbage_timer->start(0);
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
    map_node_addresses.push_back(addr.toIPv4Address());
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
    map_skip_node_addresses.push_back(addr.toIPv4Address());
  }
  fclose(f);

  return 0;
}


void MainObject::PurgeSkippedNodes()
{
  for(int i=(int)map_node_addresses.size()-1;i>=0;i--) {
    for(unsigned j=0;j<map_skip_node_addresses.size();j++) {
      if(map_node_addresses[i]==map_skip_node_addresses[j]) {
	map_node_addresses.erase(map_node_addresses.begin()+i);
      }
    }
  }
}


void MainObject::DumpNodeList()
{
  for(unsigned i=0;i<map_node_addresses.size();i++) {
    printf("%s\n",(const char *)QHostAddress(map_node_addresses[i]).
	   toString().toUtf8());
  }
}


void MainObject::SortAddresses(std::vector<uint32_t> &addrs)
{
  uint32_t addr;
  bool changed=true;

  if(addrs.size()>1) {
    while(changed) {
      changed=false;
      for(unsigned i=0;i<(addrs.size()-1);i++) {
	if((addrs[i])>(addrs[i+1])) {
	  addr=addrs[i];
	  addrs[i]=addrs[i+1];
	  addrs[i+1]=addr;
	  changed=true;
	}
      }
    }
  }
}

/*
void MainObject::LoadMatrix(int mtxnum)
{
  SyProfile *p=new SyProfile();
  p->setSource(LWPATH_CONF_FILE);

  if(p->stringValue("Matrices",QString().sprintf("Type%d",mtxnum+1)).toLower()!=
     "livewire") {
    fprintf(stderr,"dmap: matrix is not LiveWire\n");
    exit(256);
  }
  map_interface_address=
    p->addressValue("Matrices",QString().sprintf("IpAddress%d",mtxnum+1),
		    "0.0.0.0");
  delete p;
}
*/

void MainObject::Verbose(const QString &msg)
{
  if(map_verbose) {
    fprintf(stderr,"%s",(const char *)msg.toUtf8());
    fflush(stderr);
  }
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
