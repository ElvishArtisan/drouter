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

#include <QJsonArray>
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
  j_time_format="hh:mm:ss";
  j_date_format="dddd, MMMM d yyyy";

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


void DRJParser::setModelFont(const QFont &font)
{
  j_model_font=font;
  j_router_model->setFont(font);
  for(QMap<int,DRActionListModel *>::const_iterator it=j_action_models.begin();
      it!=j_action_models.end();it++) {
    it.value()->setFont(font);
  }
  for(QMap<int,DREndPointListModel *>::const_iterator it=j_input_models.begin();
      it!=j_input_models.end();it++) {
    it.value()->setFont(font);
  }
  for(QMap<int,DREndPointListModel *>::const_iterator it=
	j_output_models.begin();it!=j_output_models.end();it++) {
    it.value()->setFont(font);
  }
}


void DRJParser::setModelPalette(const QPalette &pal)
{
  for(QMap<int,DRActionListModel *>::const_iterator it=j_action_models.begin();
      it!=j_action_models.end();it++) {
    it.value()->setPalette(pal);
  }
}


QString DRJParser::timeFormat() const
{
  return j_time_format;
}


void DRJParser::setTimeFormat(const QString &fmt)
{
  if(fmt!=j_time_format) {
    for(QMap<int,DRActionListModel *>::const_iterator it=j_action_models.begin();
	it!=j_action_models.end();it++) {
      it.value()->setTimeFormat(fmt);
    }
    j_time_format=fmt;
  }
}


QString DRJParser::dateFormat() const
{
  return j_date_format;
}


void DRJParser::setDateFormat(const QString &fmt)
{
  if(fmt!=j_date_format) {
    j_date_format=fmt;
  }
}


QList<int> DRJParser::routerFilter() const
{
  return j_router_filter;
}


void DRJParser::setRouterFilter(const QList<int> routers)
{
  j_router_filter=routers;
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


DRSnapshotListModel *DRJParser::snapshotModel(int router) const
{
  return j_snapshot_models.value(router);
}


DRActionListModel *DRJParser::actionModel(int router) const
{
  return j_action_models.value(router);
}


bool DRJParser::isConnected() const
{
  return j_connected;
}


bool DRJParser::gpioSupported(int router) const
{
  return j_gpio_supporteds.value(router);
}


int DRJParser::outputCrosspoint(int router,int output) const
{
  return j_output_xpoints[router][output];
}


void DRJParser::setOutputCrosspoint(int router,int output,int input)
{
  QVariantMap fields;

  fields["router"]=router;
  fields["destination"]=output;
  fields["source"]=input;

  SendCommand("activateroute",fields);
}


QString DRJParser::gpiState(int router,int input) const
{
  return j_gpi_states[router][input];
}


void DRJParser::setGpiState(int router,int input,const QString &code,int msec)
{
  QVariantMap fields;

  fields["router"]=router;
  fields["source"]=input;
  fields["code"]=code;
  fields["duration"]=msec;

  SendCommand("triggergpi",fields);
}


QString DRJParser::gpoState(int router,int output) const
{
  return j_gpo_states[router][output];
}


void DRJParser::setGpoState(int router,int output,const QString &code,int msec)
{
  QVariantMap fields;

  fields["router"]=router;
  fields["destination"]=output;
  fields["code"]=code;
  fields["duration"]=msec;

  SendCommand("triggergpo",fields);
}


void DRJParser::activateSnapshot(int router,const QString &snapshot)
{
  QVariantMap fields;

  fields["router"]=router;
  fields["snapshot"]=snapshot;

  SendCommand("activatesnap",fields);
}


void DRJParser::saveAction(int router,QVariantMap fields)
{
  fields["router"]=router;
  fields["time"]=fields.value("time").toString()+"Z";
  SendCommand("actionedit",fields);
}


void DRJParser::removeAction(int id)
{
  QVariantMap fields;
  fields["id"]=id;
  SendCommand("actiondelete",fields);
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

  case DRJParser::TimeError:
    ret="invalid time value";
    break;

  case DRJParser::DatabaseError:
    ret="database error";
    break;

  case DRJParser::LastError:
    break;
  }

  return ret;
}


QString DRJParser::eventTypeString(EventType type)
{
  QString ret="unknown";

  switch(type) {
  case DRJParser::CommentEvent:
    ret="comment";
    break;

  case DRJParser::RouteEvent:
    ret="route";
    break;

  case DRJParser::SnapshotEvent:
    ret="snapshot";
    break;

  case DRJParser::UnknownEvent:
    break;
  }

  return ret;
}


DRJParser::EventType typeFromString(const QString &str)
{
  if(str.trimmed().toLower()=="comment") {
    return DRJParser::CommentEvent;
  }
  if(str.trimmed().toLower()=="route") {
    return DRJParser::RouteEvent;
  }
  if(str.trimmed().toLower()=="snapshot") {
    return DRJParser::SnapshotEvent;
  }

  return DRJParser::UnknownEvent;
}


void DRJParser::connectedData()
{
  j_accum.clear();
  j_accum_quoted=false;
  j_accum_level=0;

  SendCommand("routernames",QVariantMap());
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
	  //	  printf("RECV: %s\n",j_accum.constData());
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
  j_output_xpoints.clear();
  j_gpi_states.clear();
  j_gpo_states.clear();
  j_gpio_supporteds.clear();
}


void DRJParser::DispatchMessage(const QJsonDocument &jdoc)
{
  if(jdoc.object().contains("error")) {
    QVariantMap map=jdoc.object().value("error").toObject().toVariantMap();
    emit parserError((DRJParser::ErrorType)map.value("type").toUInt(),
		     map.value("description").toString());
  }

  if(jdoc.object().contains("routernames")) {
    QJsonObject jo0=jdoc.object();
    QJsonArray ja0=jo0.value("routernames").toArray();
    for(int i=0;i<ja0.size();i++) {
      QJsonObject jo1=ja0.at(i).toObject();
      if((j_router_filter.size()==0)||
	 (j_router_filter.contains(jo1.value("number").toInt()))) {
	j_router_model->addRouter(jo1.value("number").toInt(),
				  jo1.value("name").toString(),
				  jo1.value("type").toString());
	int router=jo1.value("number").toInt();

	j_input_models[router]=
	  new DREndPointListModel(router,j_use_long_names,this);
	j_input_models.value(router)->setFont(j_model_font);

	j_output_models[router]=
	  new DREndPointListModel(router,j_use_long_names,this);
	j_output_models.value(router)->setFont(j_model_font);
	j_output_xpoints[router]=QMap<int,int>();
	
	j_snapshot_models[router]=new DRSnapshotListModel(router,this);
	j_snapshot_models.value(router)->setFont(j_model_font);

	DRActionListModel *amodel=new DRActionListModel(router,this);
	amodel->setFont(j_model_font);
	amodel->setTimeFormat(j_time_format);
	amodel->setInputsModel(j_input_models.value(router));
	amodel->setOutputsModel(j_output_models.value(router));
	j_action_models[router]=amodel;

	//  j_gpi_states[router]=QMap<int,QString>();
	//  j_gpo_states[router]=QMap<int,QString>();
	//  j_gpio_supporteds[router]=false;

	//  j_snapshot_names[router]=QStringList();
	QVariantMap router_fields;
	router_fields["router"]=router;

	SendCommand("sourcenames",router_fields);
	SendCommand("destnames",router_fields);
	SendCommand("snapshots",router_fields);
	SendCommand("actionlist",router_fields);

	// SendCommand("gpistat",router_fields);
	// SendCommand("gpostat",router_fields);

	SendCommand("routestat",router_fields);

	router_fields["sendUpdates"]=true;
	SendCommand("actionstat",router_fields);
      }
    }
    j_router_model->finalize();
    SendCommand("ping",QVariantMap());
  }

  if(jdoc.object().contains("sourcenames")) {
    QJsonObject jo0=jdoc.object().value("sourcenames").toObject();
    int router=jdoc.object().value("router").toInt();
    QJsonArray ja0=jdoc.object().value("sourcenames").toArray();
    DREndPointListModel *imod=j_input_models.value(router);
    if(imod!=NULL) {
      for(int i=0;i<ja0.size();i++) {
	QJsonObject jo1=ja0.at(i).toObject();
	QStringList keys=jo1.keys();
	QVariantMap fields;
	for(int i=0;i<keys.size();i++) {
	  fields[keys.at(i)]=jo1.value(keys.at(i)).toVariant();
	}
	imod->addEndPoint(fields);
      }
    }
    imod->finalize();
    return;
  }

  if(jdoc.object().contains("destnames")) {
    QJsonObject jo0=jdoc.object().value("destnames").toObject();
    int router=jdoc.object().value("router").toInt();
    QJsonArray ja0=jdoc.object().value("destnames").toArray();
    DREndPointListModel *omod=j_output_models.value(router);
    if(omod!=NULL) {
      for(int i=0;i<ja0.size();i++) {
	QJsonObject jo1=ja0.at(i).toObject();
	QStringList keys=jo1.keys();
	QVariantMap fields;
	for(int i=0;i<keys.size();i++) {
	  fields[keys.at(i)]=jo1.value(keys.at(i)).toVariant();
	}
	omod->addEndPoint(fields);
      }
    }
    omod->finalize();
    return;
  }

  if(jdoc.object().contains("snapshots")) {
    QJsonObject jo0=jdoc.object().value("snapshots").toObject();
    int router=jo0.value("router").toInt();
    DRSnapshotListModel *smodel=j_snapshot_models.value(router);
    if(smodel!=NULL) {
      for(QJsonObject::const_iterator it=jo0.begin();it!=jo0.end();it++) {
	if(it.key().left(8)=="snapshot") {
	  QJsonObject jo1=it.value().toObject();
	  if(smodel!=NULL) {
	    smodel->addSnapshot(jo1.value("name").toString());
	  }
	}
      }
    }
  }

  if(jdoc.object().contains("actionlist")) {
    QJsonObject jo0=jdoc.object().value("actionlist").toObject();
    int router=jo0.value("router").toInt();
    DRActionListModel *amod=j_action_models.value(router);
    if(amod!=NULL) {
      for(QJsonObject::const_iterator it=jo0.begin();it!=jo0.end();it++) {
	if(it.key().left(6)=="action") {
	  QJsonObject jo1=it.value().toObject();
	  QStringList keys=jo1.keys();
	  QVariantMap fields;
	  for(int i=0;i<keys.size();i++) {
	    fields[keys.at(i)]=jo1.value(keys.at(i)).toVariant();
	  }
	  amod->addAction(fields);
	}
      }
    }
    return;
  }

  if(jdoc.object().contains("actiondelete")) {
    QJsonObject jo0=jdoc.object().value("actiondelete").toObject();
    int id=jo0.value("id").toInt();
    if(id>0) {
      for(QMap<int,DRActionListModel *>::const_iterator it=
	    j_action_models.begin();it!=j_action_models.end();it++) {
	it.value()->removeAction(id);
      }
    }
    return;
  }

  if(jdoc.object().contains("actionstat")) {
    QJsonObject jo0=jdoc.object().value("actionstat").toObject();
    int router=jo0.value("router").toInt();
    DRActionListModel *amodel=actionModel(router);
    if(amodel!=NULL) {
      QJsonArray ja=jo0.value("nextId").toArray();
      QList<int> ids;
      for(int i=0;i<ja.size();i++) {
	ids.push_back(ja.at(i).toInt());
      }
      amodel->updateNextActions(router,ids);
    }
    return;
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


void DRJParser::SendCommand(const QString &verb,const QVariantMap &args)
{
  QJsonObject jo0;
  QJsonDocument jdoc;

  jo0.insert(verb,QJsonObject::fromVariantMap(args));
  jdoc.setObject(jo0);
  QString json=jdoc.toJson();

  j_socket->write(json.toUtf8());
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
