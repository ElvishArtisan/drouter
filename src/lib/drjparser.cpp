// drjparser.cpp
//
// Parser for Protocol J Protocol
//
//   (C) Copyright 2016-2024 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
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

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include "drjparser.h"

DRJParser::DRJParser(bool use_long_names,QObject *parent)
  : QObject(parent)
{
  j_use_long_names=use_long_names;
  j_connected=false;
  j_reading_routers=false;
  j_reading_sources=false;
  j_reading_dests=false;
  j_reading_snapshots=false;
  j_current_router=-1;
  j_last_router=-1;
  j_prev_input=0;
  j_prev_output=0;
  j_last_xpoint_router=-1;
  j_last_xpoint_output=-1;
  j_accum_quoted=false;
  j_accum_level=0;

  //
  // The Socket
  //
  j_socket=NULL;

  //
  // The Models
  //
  j_router_model=new DRRouterListModel(this);

  //
  // Startup Timer
  //
  j_startup_timer=new QTimer(this);
  j_startup_timer->setSingleShot(true);
  connect(j_startup_timer,SIGNAL(timeout()),this,SLOT(startupData()));

  //
  // Watchdog Timers
  //
  j_holdoff_timer=new QTimer(this);
  j_holdoff_timer->setSingleShot(true);
  connect(j_holdoff_timer,SIGNAL(timeout()),
	  this,SLOT(holdoffReconnectData()));
}


DRJParser::~DRJParser()
{
  delete j_router_model;
}


QMap<int,QString> DRJParser::routers() const
{
  return j_router_names;
}


DRRouterListModel *DRJParser::routerModel() const
{
  return j_router_model;
}


DREndPointListModel *DRJParser::outputModel(int router) const
{
  return j_output_models.value(router);
}


DREndPointListModel *DRJParser::inputModel(int router) const
{
  return j_input_models.value(router);
}


bool DRJParser::isConnected() const
{
  return j_connected;
}


bool DRJParser::gpioSupported(int router) const
{
  return j_gpio_supporteds.value(router);
}


int DRJParser::inputQuantity(int router) const
{
  return j_input_quantities.value(router);
}


bool DRJParser::inputIsReal(int router,int input) const
{
  return j_input_is_reals.value(router).value(input,false);
}


QString DRJParser::inputNodeName(int router,int input) const
{
  return j_input_node_names[router][input];
}


QHostAddress DRJParser::inputNodeAddress(int router,int input) const
{
  return j_input_node_addresses[router][input];
}


int DRJParser::inputNodeSlotNumber(int router,int input) const
{
  return j_input_node_slot_numbers[router][input];
}


QString DRJParser::inputName(int router,int input) const
{
  return j_input_names[router][input];
}


QString DRJParser::inputLongName(int router,int input) const
{
  return j_input_long_names[router][input];
}


int DRJParser::inputSourceNumber(int router,int input) const
{
  return j_input_source_numbers[router][input];
}


QHostAddress DRJParser::inputStreamAddress(int router,int input) const
{
  return j_input_stream_addresses[router][input];
}


int DRJParser::outputQuantity(int router) const
{
  return j_output_quantities.value(router);
}


bool DRJParser::outputIsReal(int router,int output) const
{
  return j_output_is_reals[router][output];
}


QString DRJParser::outputNodeName(int router,int output) const
{
  return j_output_node_names[router][output];
}


QHostAddress DRJParser::outputNodeAddress(int router,int output) const
{
  return j_output_node_addresses[router][output];
}


int DRJParser::outputNodeSlotNumber(int router,int output) const
{
  return j_output_node_slot_numbers[router][output];
}


QString DRJParser::outputName(int router,int output) const
{
  return j_output_names[router][output];
}


QString DRJParser::outputLongName(int router,int output) const
{
  return j_output_long_names[router][output];
}


int DRJParser::outputCrosspoint(int router,int output) const
{
  return j_output_xpoints[router][output];
}


void DRJParser::setOutputCrosspoint(int router,int output,int input)
{
  SendCommand(QString::asprintf("ActivateRoute %d %d %d",router,output,input));
}


QString DRJParser::gpiState(int router,int input) const
{
  return j_gpi_states[router][input];
}


void DRJParser::setGpiState(int router,int input,const QString &code,int msec)
{
  if(msec<0) {
    SendCommand(QString::asprintf("TriggerGPI %d %d %s",router,input,
				  code.toUtf8().constData()));
  }
  else {
    SendCommand(QString::asprintf("TriggerGPI %d %d %s %d",router,input,
				  code.toUtf8().constData(),msec));
  }
}


QString DRJParser::gpoState(int router,int output) const
{
  return j_gpo_states[router][output];
}


void DRJParser::setGpoState(int router,int output,const QString &code,int msec)
{
  if(msec<0) {
    SendCommand(QString::asprintf("TriggerGPO %d %d %s",router,output,
				  code.toUtf8().constData()));
  }
  else {
    SendCommand(QString::asprintf("TriggerGPO %d %d %s %d",router,output,
				  code.toUtf8().constData(),msec));
  }
}


int DRJParser::snapshotQuantity(int router) const
{
  return j_snapshot_names[router].size();
}


QString DRJParser::snapshotName(int router,int n) const
{
  return j_snapshot_names[router].at(n);
}


void DRJParser::activateSnapshot(int router,const QString &snapshot)
{
  SendCommand(QString::asprintf("ActivateSnap %d ",router)+snapshot);
}


void DRJParser::connectToHost(const QString &hostname,uint16_t port,
			     const QString &username,const QString &passwd)
{
  MakeSocket();
  j_hostname=hostname;
  j_port=port;
  j_username=username;
  j_password=passwd;
  j_socket->connectToHost(hostname,port);
}


QString DRJParser::connectionStateString(ConnectionState cstate)
{
  QString ret=tr("Unknown")+QString::asprintf(" [%d]",cstate);

  switch(cstate) {
  case DRJParser::Ok:
    ret=tr("OK");
    break;

  case DRJParser::InvalidLogin:
    ret=tr("Invalid login");
    break;

  case DRJParser::WatchdogActive:
    ret=tr("Watchdog active");
    break;
  }

  return ret;
}


QString DRJParser::errorString(ErrorType err)
{
  QString ret=QString::asprintf("unknown error %u",err);

  switch(err) {
  case DRJParser::OkError:
    ret="ok";
    break;

  case DRJParser::JsonError:
    ret="JSON syntax error";
    break;

  case DRJParser::ParameterError:
    ret="command parameter error";
    break;

  case DRJParser::NoRouterError:
    ret="no such router";
    break;

  case DRJParser::NoSnapshotError:
    ret="no such snapshot";
    break;

  case DRJParser::NoSourceError:
    ret="no such source";
    break;

  case DRJParser::NoDestinationError:
    ret="no such destination";
    break;

  case DRJParser::NotGpioRouterError:
    ret="not a GPIO router";
    break;

  case DRJParser::NoCommandError:
    ret="no such command";
    break;

  case DRJParser::LastError:
    break;
  }

  return ret;
}


void DRJParser::connectedData()
{
  //  SendCommand("Login "+j_username+" "+j_password);
  j_accum.clear();
  j_accum_quoted=false;
  j_accum_level=0;
  SendCommand("routernames");
}


void DRJParser::connectionClosedData()
{
  j_connected=false;
  Clear();
  emit connected(false,DRJParser::WatchdogActive);
  j_holdoff_timer->start(DRJPARSER_HOLDOFF_INTERVAL);
}


void DRJParser::startupData()
{
  j_connected=true;
  emit connected(true,DRJParser::Ok);
}


void DRJParser::holdoffReconnectData()
{
  MakeSocket();
  j_socket->connectToHost(j_hostname,j_port);
}


void DRJParser::readyReadData()
{
  QByteArray data;

  data=j_socket->readAll();
  for(int i=0;i<data.length();i++) {
    switch(0xFFF&data[i]) {
    case '"':
      j_accum_quoted=!j_accum_quoted;
      j_accum+=data[i];
      break;

    case '{':
      if(!j_accum_quoted) {
	j_accum_level++;
      }
      j_accum+=data[i];
      break;

    case '}':
      j_accum+=data[i];
      if(!j_accum_quoted) {
	if(--j_accum_level==0) {
	  QJsonDocument jdoc=QJsonDocument::fromJson(j_accum);
	  if(jdoc.isNull()) {
	    emit parserError(DRJParser::JsonError,QString::fromUtf8(j_accum));
	  }
	  else {
	    DispatchMessage(jdoc);
	  }
	  j_accum.clear();
	}
      }
      break;

    default:
      j_accum+=data[i];
      break;
    }
  }
}


void DRJParser::errorData(QAbstractSocket::SocketError err)
{
  emit error(err);
  j_holdoff_timer->start(DRJPARSER_HOLDOFF_INTERVAL);
}


void DRJParser::Clear()
{
  j_router_names.clear();
  j_input_node_names.clear();
  j_input_node_slot_numbers.clear();
  j_input_node_addresses.clear();
  j_input_names.clear();
  j_input_long_names.clear();
  j_input_source_numbers.clear();
  j_input_stream_addresses.clear();
  j_output_node_names.clear();
  j_output_node_addresses.clear();
  j_output_node_slot_numbers.clear();
  j_output_names.clear();
  j_output_long_names.clear();
  j_output_xpoints.clear();
  j_gpi_states.clear();
  j_gpo_states.clear();
  j_gpio_supporteds.clear();
  j_snapshot_names.clear();
}


void DRJParser::DispatchMessage(const QJsonDocument &jdoc)
{
  QString cmd;

  if(jdoc.object().contains("routernames")) {
    QJsonObject jo0=jdoc.object().value("routernames").toObject();

    for(QJsonObject::const_iterator it=jo0.begin();it!=jo0.end();it++) {
      QJsonObject jo1=it.value().toObject();

      j_router_model->addRouter(jo1.value("number").toInt(),
				jo1.value("name").toString(),
				jo1.value("type").toString());
      int router=jo1.value("number").toInt();
      /*
      j_router_names[router]=jo1.value("name").toString();
      */

      j_input_models[router]=new DREndPointListModel(router,j_use_long_names,this);
      /*
      j_input_quantities[router]=0;
      j_input_is_reals[router]=QMap<int,bool>();
      j_input_names[router]=QMap<int,QString>();
      j_input_long_names[router]=QMap<int,QString>();
      j_input_source_numbers[router]=QMap<int,int>();
      j_input_stream_addresses[router]=QMap<int,QHostAddress>();
      */
      j_output_models[router]=
	new DREndPointListModel(router,j_use_long_names,this);
      /*
      j_output_quantities[router]=0;
      j_output_is_reals[router]=QMap<int,bool>();
      j_output_names[router]=QMap<int,QString>();
      j_output_long_names[router]=QMap<int,QString>();
      */
      j_output_xpoints[router]=QMap<int,int>();

      /*
      j_gpi_states[router]=QMap<int,QString>();
      j_gpo_states[router]=QMap<int,QString>();
      j_gpio_supporteds[router]=false;

      j_snapshot_names[router]=QStringList();
      */
      cmd=QString::asprintf("sourcenames %d\r\n",router);
      j_socket->write(cmd.toUtf8());

      cmd=QString::asprintf("destnames %d\r\n",router);
      j_socket->write(cmd.toUtf8());
      /*
      cmd=QString::asprintf("snapshots %d\r\n",router);
      j_socket->write(cmd.toUtf8());

      cmd=QString::asprintf("gpistat %d\r\n",router);
      j_socket->write(cmd.toUtf8());

      cmd=QString::asprintf("gpostat %d\r\n",router);
      j_socket->write(cmd.toUtf8());
      */
      cmd=QString::asprintf("routestat %d\r\n",router);
      j_socket->write(cmd.toUtf8());
    }
    j_socket->write("ping\r\n");
  }

  if(jdoc.object().contains("sourcenames")) {
    QJsonObject jo0=jdoc.object().value("sourcenames").toObject();
    int router=jo0.value("router").toInt();
    DREndPointListModel *imod=j_input_models.value(router);
    if(imod!=NULL) {
      for(QJsonObject::const_iterator it=jo0.begin();it!=jo0.end();it++) {
	if(it.key().left(6)=="source") {
	  QJsonObject jo1=it.value().toObject();
	  QStringList keys=jo1.keys();
	  QMap<QString,QVariant> fields;
	  for(int i=0;i<keys.size();i++) {
	    fields[keys.at(i)]=jo1.value(keys.at(i)).toVariant();
	  }
	  imod->addEndPoint(fields);
	}
      }
    }
    imod->finalize();
  }

  if(jdoc.object().contains("destnames")) {
    QJsonObject jo0=jdoc.object().value("destnames").toObject();
    int router=jo0.value("router").toInt();
    DREndPointListModel *omod=j_output_models.value(router);
    if(omod!=NULL) {
      for(QJsonObject::const_iterator it=jo0.begin();it!=jo0.end();it++) {
	if(it.key().left(11)=="destination") {
	  QJsonObject jo1=it.value().toObject();
	  QStringList keys=jo1.keys();
	  QMap<QString,QVariant> fields;
	  for(int i=0;i<keys.size();i++) {
	    fields[keys.at(i)]=jo1.value(keys.at(i)).toVariant();
	  }
	  omod->addEndPoint(fields);
	}
      }
    }
    omod->finalize();
  }

  if(jdoc.object().contains("snapshots")) {
    QJsonObject jo0=jdoc.object().value("snapshots").toObject();
    int router=jo0.value("snapshots").toInt();
    for(QJsonObject::const_iterator it=jo0.begin();it!=jo0.end();it++) {
      if(it.key().left(8)=="snapshot") {
	QJsonObject jo1=it.value().toObject();
	j_snapshot_names[router].push_back(jo1.value("name").toString());
      }
    }
  }

  if(jdoc.object().contains("gpistat")) {
    QJsonObject jo0=jdoc.object().value("gpistat").toObject();
    int router=jo0.value("router").toInt();
    int input=jo0.value("source").toInt();
    QString code=jo0.value("code").toString();

    j_gpi_states[router][input]=code;
    if(j_connected) {
      emit gpiStateChanged(router,input,code);
    }
  }

  if(jdoc.object().contains("gpostat")) {
    QJsonObject jo0=jdoc.object().value("gpostat").toObject();
    int router=jo0.value("router").toInt();
    int output=jo0.value("destination").toInt();
    QString code=jo0.value("code").toString();

    j_gpo_states[router][output]=code;
    if(j_connected) {
      emit gpoStateChanged(router,output,code);
    }
  }

  if(jdoc.object().contains("routestat")) {
    QJsonObject jo0=jdoc.object().value("routestat").toObject();
    int router=jo0.value("router").toInt();
    int output=jo0.value("destination").toInt();
    int input=jo0.value("source").toInt();

    j_output_xpoints[router][output]=input;
    if(j_connected) {
      emit outputCrosspointChanged(router,output,input);
    }
  }

  if(jdoc.object().contains("pong")) {
    j_connected=true;
    emit connected(true,DRJParser::Ok);
  }
}

/*
void DRJParser::DispatchCommand(QString cmd)
{
  //printf("RECV: %s\n",(const char *)cmd.toUtf8());

  bool ok=false;

  QStringList f0=cmd.replace(">>","").toLower().split(" ");

  //
  // Process Login
  //
  if(f0[0]=="login") {
    if((f0.size()==2)&&(f0[1]=="successful")) {
      SendCommand("RouterNames");
    }
    else {
      j_socket->deleteLater();
      j_socket=NULL;
      j_connected=false;
      Clear();
      emit connected(false,DRJParser::InvalidLogin);
    }
    return;
  }

  //
  // Check for delimiters
  //
  if(f0[0]=="begin") {
    if(f0.size()==2) {
      if(f0[1]=="routernames") {
	j_router_names.clear();
	j_reading_routers=true;
      }
    }
    if(f0.size()==4) {
      j_current_router=f0[3].toInt(&ok);
      if(ok) {
	if(f0[1]=="sourcenames") {
	  j_input_names[j_current_router].clear();
	  j_input_long_names[j_current_router].clear();
	  j_input_source_numbers[j_current_router].clear();
	  j_input_stream_addresses[j_current_router].clear();
	  j_prev_input=0;
	  j_reading_sources=true;
	}
	if(f0[1]=="destnames") {
	  j_output_names[j_current_router].clear();
	  j_output_long_names[j_current_router].clear();
	  j_prev_output=0;
	  j_reading_dests=true;
	}
	if(f0[1]=="snapshotnames") {
	  j_snapshot_names[j_current_router].clear();
	  j_reading_snapshots=true;
	}
      }
    }
    return;
  }
  if(f0[0]=="end") {
    if(f0.size()==2) {
      if(f0[1]=="routernames") {
	j_reading_routers=false;
	emit routerListChanged();
	for(QMap<int,QString>::const_iterator it=j_router_names.begin();
	    it!=j_router_names.end();it++) {
	  SendCommand(QString::asprintf("SourceNames %u",it.key()));
	  j_last_router=it.key();
	}
      }
    }
    if(f0.size()==4) {
      j_current_router=f0[3].toInt(&ok);
      if(ok) {
	if((f0[1]=="sourcenames")&&(j_current_router==j_last_router)) {
	  j_reading_sources=false;
	  emit inputListChanged();
	  for(QMap<int,QString>::const_iterator it=j_router_names.begin();
	      it!=j_router_names.end();it++) {
	    SendCommand(QString::asprintf("DestNames %u",it.key()));
	  }
	}
	if((f0[1]=="destnames")&&(j_current_router==j_last_router)) {
	  j_reading_dests=false;
	  emit outputListChanged();
	  for(QMap<int,QString>::const_iterator it=j_router_names.begin();
	      it!=j_router_names.end();it++) {
	    SendCommand(QString::asprintf("Snapshots %u",it.key()));
	  }
	}
	if((f0[1]=="snapshotnames")&&(j_current_router==j_last_router)) {
	  for(QMap<int,QString>::const_iterator it=j_router_names.begin();
	      it!=j_router_names.end();it++) {
	    if(j_output_names[it.key()].size()>0) {
	      SendCommand(QString::asprintf("RouteStat %u\r\n",it.key()));
	      j_last_xpoint_router=it.key();
	      QMap<int,QString>::const_iterator it2=
		j_output_names[it.key()].end();
	      it2--;
	      j_last_xpoint_output=it2.key();
	    }
	  }
	  j_reading_snapshots=false;
	  j_reading_xpoints=true;
	}
      }
    }
    return;
  }

  //
  // Populate Endpoint Names
  //
  if(j_reading_routers) {
    ReadRouterName(cmd);
    return;
  }
  if(j_reading_sources) {
    ReadSourceName(cmd);
    return;
  }
  if(j_reading_dests) {
    ReadDestName(cmd);
    return;
  }
  if(j_reading_snapshots) {
    ReadSnapshotName(cmd);
    return;
  }

  //
  // Process Crosspoint Changes
  //
  if((f0[0]=="routestat")&&(f0.size()==5)) {
    int router=f0[1].toUInt(&ok);
    if(ok) {
      int output=f0[2].toInt(&ok);
      if(ok) {
	int input=f0[3].toInt(&ok);
	if(ok) {
	  j_output_xpoints[router][output]=input;
	  emit outputCrosspointChanged(router,output,input);
	  if((router==j_last_xpoint_router)&&(output==j_last_xpoint_output)) {
	    j_last_xpoint_router=-1;
	    j_last_xpoint_output=-1;

	    for(QMap<int,QString>::const_iterator it=j_router_names.begin();
		it!=j_router_names.end();it++) {
	      SendCommand(QString::asprintf("GPIStat %d",it.key()));
	      SendCommand(QString::asprintf("GPOStat %d",it.key()));
	    }
	    j_startup_timer->start(DRJPARSER_STARTUP_INTERVAL);
	  }
	}
      }
    }
  }

  //
  // Process GPI State Changes
  //
  if((f0[0]=="gpistat")&&(f0.size()==4)) {
    int router=f0[1].toUInt(&ok);
    j_gpio_supporteds[router]=true;
    if(ok) {
      int input=f0[2].toInt(&ok);
      if(ok) {
	j_gpi_states[router][input]=f0[3];
	emit gpiStateChanged(router,input,f0[3]);
      }
    }
  }

  //
  // Process GPO State Changes
  //
  if((f0[0]=="gpostat")&&(f0.size()==4)) {
    int router=f0[1].toUInt(&ok);
    if(ok) {
      int output=f0[2].toInt(&ok);
      if(ok) {
	j_gpo_states[router][output]=f0[3];
	emit gpoStateChanged(router,output,f0[3]);
      }
    }
  }

  //  printf("CMD: %s\n",(const char *)cmd.toUtf8());
}
*/

void DRJParser::ReadRouterName(const QString &cmd)
{
  bool ok=false;
  QStringList f0=cmd.split(" ",QString::SkipEmptyParts);
  if(f0.size()>=2) {
    for(int i=2;i<f0.size();i++) {
      f0[1]+=" "+f0[i];
    }
    int router=f0.at(0).toInt(&ok);
    if(ok) {
      j_router_names[router]=f0.at(1);
    }
  }
}


void DRJParser::ReadSourceName(const QString &cmd)
{
  QStringList f0=cmd.split("\t");
  bool ok=false;
  int srcnum=0;
  int input=f0.at(0).toInt(&ok);
  QHostAddress addr;

  if(ok) {
    for(int i=j_prev_input+1;i<input;i++) {
      j_input_is_reals[j_current_router][i]=false;
    }
    if(f0.size()==8) {
      srcnum=f0[6].toInt(&ok);
    }
    if(f0.size()>=3) {
      QStringList f1=f0.at(2).split("ON");
      j_input_node_names[j_current_router][input]=f1.back().trimmed();
      if(addr.setAddress(f0.at(3))) {
	j_input_node_addresses[j_current_router][input]=addr;
      }
      int slot=f0.at(5).toUInt(&ok);
      if(ok) {
	j_input_node_slot_numbers[j_current_router][input]=slot-1;
      }
      if(f0[1].trimmed().isEmpty()) {
	if(j_gpio_supporteds[j_current_router]) {
	  j_input_names[j_current_router][input]=
	    tr("GPI")+QString::asprintf(" %d",input);
	  j_input_long_names[j_current_router][input]=
	    tr("GPI")+QString::asprintf(" %d",input)+f0[2];
	}
	else {
	  j_input_names[j_current_router][input]=
	    tr("Input")+QString::asprintf(" %d",input);
	  j_input_long_names[j_current_router][input]=
	    tr("Input")+QString::asprintf(" %d",input)+f0[2];
	}
      }
      else {
	j_input_names[j_current_router][input]=f0[1];
	j_input_long_names[j_current_router][input]=f0[2];
      }
      j_input_is_reals[j_current_router][input]=true;
      if(ok) {
	if(srcnum<=0) {
	  //	  j_input_long_names[j_current_router][input]=f0[2];
	  j_input_source_numbers[j_current_router][input]=-1;
	  j_input_stream_addresses[j_current_router][input]=QHostAddress();
	}
	else {
	  //	  j_input_long_names[j_current_router][input]=f0[2];
	  j_input_source_numbers[j_current_router][input]=f0.at(6).toInt();
	  QHostAddress addr;
	  if(addr.setAddress(f0.at(7))) {
	    j_input_stream_addresses[j_current_router][input]=addr;
	  }
	  else {
	    j_input_stream_addresses[j_current_router][input]=QHostAddress();
	  }
	}
      }
      else {
	j_input_long_names[j_current_router][input]=f0[2];
	j_input_source_numbers[j_current_router][input]=-1;
	j_input_stream_addresses[j_current_router][input]=QHostAddress();
      }
    }
    j_prev_input=input;
  }
}


void DRJParser::ReadDestName(const QString &cmd)
{
  QStringList f0=cmd.split("\t");
  bool ok=false;
  int output=f0.at(0).toInt(&ok);
  QHostAddress addr;

  if(f0.size()>=3) {
    for(int i=j_prev_output+1;i<output;i++) {
      j_output_is_reals[j_current_router][i]=false;
    }
    if(ok) {
      QStringList f1=f0.at(2).split("ON");
      j_output_node_names[j_current_router][output]=f1.back().trimmed();
      j_output_names[j_current_router][output]=f0.at(1);
      j_output_is_reals[j_current_router][output]=true;
      j_output_long_names[j_current_router][output]=f0.at(2);
      if(f0.size()>=4) {
	if(addr.setAddress(f0.at(3))) {
	  j_output_node_addresses[j_current_router][output]=addr;
	}
	if(f0.size()>=6) {
	  int slot=f0.at(5).toUInt(&ok);
	  if(ok) {
	    j_output_node_slot_numbers[j_current_router][output]=slot-1;
	  }
	}
      }
    }
    j_prev_output=output;
  }
}


void DRJParser::ReadSnapshotName(const QString &cmd)
{
  j_snapshot_names[j_current_router].push_back(cmd.trimmed());
}


void DRJParser::BubbleSort(std::map<unsigned,QString> *names,
			    std::vector<unsigned> *ptrs)
{
  //
  // Reset Pointer Table
  //
  ptrs->clear();
  for(unsigned i=0;i<names->size();i++) {
    ptrs->push_back(i);
  }

  //
  // Sort
  //
  bool changed=true;
  while(changed) {
    changed=false;
    for(unsigned i=1;i<names->size();i++) {
      if((*names)[ptrs->at(i-1)]>(*names)[ptrs->at(i)]) {
	unsigned ptr=ptrs->at(i-1);
	ptrs->at(i-1)=ptrs->at(i);
	ptrs->at(i)=ptr;
	changed=true;
      }
    }
  }
}


void DRJParser::SendCommand(const QString &cmd)
{
  j_socket->write((cmd+"\r\n").toUtf8(),cmd.length()+2);
}


void DRJParser::MakeSocket()
{
  if(j_socket!=NULL) {
    delete j_socket;
  }
  j_socket=new QTcpSocket(this);
  connect(j_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(j_socket,SIGNAL(disconnected()),
	  this,SLOT(connectionClosedData()));
  connect(j_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(j_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
}

