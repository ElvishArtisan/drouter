// logger_back.cpp
//
// Methods for finalizing provisional event log entries in drouterd(8)
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <QDateTime>

#include <drouter/drsqlquery.h>

#include "config.h"
#include "logger_back.h"

LoggerBack::LoggerBack(QMap<int,DREndPointMap *> *router_maps,QObject *parent)
  : QObject(parent)
{
  d_router_maps=router_maps;

  d_finalize_timer=new QTimer(this);
  d_finalize_timer->setSingleShot(false);
  connect(d_finalize_timer,SIGNAL(timeout()),this,SLOT(finalizeEventsData()));
}


LoggerBack::~LoggerBack()
{
  delete d_finalize_timer;
}


void LoggerBack::setWriteable(bool state)
{
  if(state) {
    d_finalize_timer->start(1000);
  }
  else {
    d_finalize_timer->stop();
  }
}


void LoggerBack::finalizeEventsData()
{
  QString sql;
  DRSqlQuery *q;
  DREndPointMap *map=NULL;

  sql=QString("select ")+
    "`ID`,"+                  // 00
    "`ROUTER_NUMBER`,"+       // 01
    "`DESTINATION_NUMBER`,"+  // 02
    "`SOURCE_NUMBER` "+       // 03
    "from `PERM_SA_EVENTS` where "+
    "`STATUS`='O' && "+
    "`DATETIME`<'"+QDateTime::currentDateTime().addSecs(-1).
    toString("yyyy-MM-dd hh:mm:ss")+"'";
  q=new DRSqlQuery(sql);
  while(q->next()) {
    if((map=d_router_maps->value(q->value(1).toInt()))==NULL) {
      FinalizeSARouteEvent(q->value(0).toInt(),false);
    }
    else {
      switch(d_router_maps->value(q->value(1).toInt())->routerType()) {
      case DREndPointMap::AudioRouter:
	FinalizeSAAudioRoute(q->value(0).toInt(),q->value(1).toInt(),
			     q->value(2).toInt(),q->value(3).toInt());
	break;

      case DREndPointMap::GpioRouter:
	FinalizeSAGpioRoute(q->value(0).toInt(),q->value(1).toInt(),
			    q->value(2).toInt(),q->value(3).toInt());
	break;

      case DREndPointMap::LastRouter:
	break;
      }
    }
  }
  delete q;
}


void LoggerBack::FinalizeSAAudioRoute(int event_id,int router,int output,
				      int input)
{
  QString sql;
  DRSqlQuery *q;

  if(input<0) {  // No route --i.e. destination is "OFF"
    sql=QString("select ")+
      "`STREAM_ADDRESS` "+  // 00
      "from `SA_DESTINATIONS` where "+
      QString::asprintf("`ROUTER_NUMBER`=%d && ",router)+
      QString::asprintf("`SOURCE_NUMBER`=%d",output);
    q=new DRSqlQuery(sql);
    if(q->first()) {
      FinalizeSARouteEvent(event_id,
			   q->value(0).toString()==DROUTER_NULL_STREAM_ADDRESS);
    }
    else {
      FinalizeSARouteEvent(event_id,false);
    }
    delete q;
  }
  else {
    sql=QString("select ")+
      "`SA_DESTINATIONS`.`ID` "+  // 00
      "from `SA_DESTINATIONS` left join `SA_SOURCES` "+
      "on `SA_DESTINATIONS`.`STREAM_ADDRESS`=`SA_SOURCES`.`STREAM_ADDRESS` && "+
      "`SA_SOURCES`.`ROUTER_NUMBER`=`SA_DESTINATIONS`.`ROUTER_NUMBER` "+
      "where "+
      QString::asprintf("`SA_DESTINATIONS`.`ROUTER_NUMBER`=%d && ",
			router)+
      QString::asprintf("`SA_SOURCES`.`SOURCE_NUMBER`=%d && ",
			input)+
      QString::asprintf("`SA_DESTINATIONS`.`SOURCE_NUMBER`=%d",
			output);
    //printf("finalize SQL: %s\n",sql.toUtf8().constData());
    q=new DRSqlQuery(sql);
    FinalizeSARouteEvent(event_id,q->first());
    delete q;
  }
}


void LoggerBack::FinalizeSAGpioRoute(int event_id,int router,int output,
				     int input)
{
  QString sql;
  DRSqlQuery *q;

  if(input<0) {  // No route --i.e. destination is "OFF"
    sql=QString("select ")+
      "`SOURCE_SLOT` "+  // 00
      "from `SA_GPOS` where "+
      QString::asprintf("`ROUTER_NUMBER`=%d && ",router)+
      QString::asprintf("`SOURCE_NUMBER`=%d && ",output)+
      "`SOURCE_SLOT`<0";
    q=new DRSqlQuery(sql);
    FinalizeSARouteEvent(event_id,q->first());
    delete q;
  }
  else {
    sql=QString("select ")+
      "`SA_GPOS`.`ID` "+  // 00
      "from `SA_GPOS` left join `SA_GPIS` "+
      "on `SA_GPOS`.`SOURCE_ADDRESS`=`SA_GPIS`.`HOST_ADDRESS` && "+
      "`SA_GPOS`.`SOURCE_SLOT`=`SA_GPIS`.`SLOT` && "+
      "`SA_GPIS`.`ROUTER_NUMBER`=`SA_GPOS`.`ROUTER_NUMBER` "+
      "where "+
      QString::asprintf("`SA_GPOS`.`ROUTER_NUMBER`=%d && ",
			router)+
      QString::asprintf("`SA_GPIS`.`SOURCE_NUMBER`=%d && ",
			input)+
      QString::asprintf("`SA_GPOS`.`SOURCE_NUMBER`=%d",
			output);
    q=new DRSqlQuery(sql);
    FinalizeSARouteEvent(event_id,q->first());
    delete q;
  }
}


void LoggerBack::FinalizeSARouteEvent(int event_id,bool status)
{
  QString comment="";
  QString sql;
  DRSqlQuery *q=NULL;
  QString router_name;
  QString input_name;
  QString output_name;

  sql=QString("select ")+
    "`STATUS`,"+              // 00
    "`ROUTER_NUMBER`,"+       // 01
    "`DESTINATION_NUMBER`,"+  // 02
    "`SOURCE_NUMBER`,"+       // 03
    "`USERNAME`,"+            // 04
    "`HOSTNAME`,"+            // 05
    "`ORIGINATING_ADDRESS` "+ // 06
    "from `PERM_SA_EVENTS` where "+
    QString::asprintf("`ID`=%d",event_id);
  q=new DRSqlQuery(sql);
  if(q->first()) {
    DREndPointMap *map=d_router_maps->value(q->value(1).toInt());
    if(map!=NULL) {
      router_name=map->routerName();
      output_name=map->name(DREndPointMap::Output,q->value(2).toInt());
      input_name=map->name(DREndPointMap::Input,q->value(3).toInt());
    }
  }
  delete q;

  if(status) {
    sql=QString("update `PERM_SA_EVENTS` set ")+
      "`STATUS`='Y',"+
      "`ROUTER_NAME`='"+DRSqlQuery::escape(router_name)+"',"+
      "`DESTINATION_NAME`='"+DRSqlQuery::escape(output_name)+"',"+
      "`SOURCE_NAME`='"+DRSqlQuery::escape(input_name)+"' "+
      QString::asprintf("where `ID`=%d",event_id);
  }
  else {
    sql=QString("update `PERM_SA_EVENTS` set ")+
      "`STATUS`='N',"+
      "`ROUTER_NAME`='"+DRSqlQuery::escape(router_name)+"',"+
      "`DESTINATION_NAME`='"+DRSqlQuery::escape(output_name)+"',"+
      "`SOURCE_NAME`='"+DRSqlQuery::escape(input_name)+"' "+
      QString::asprintf("where `ID`=%d",event_id);
  }
  DRSqlQuery::apply(sql);
  emit eventAdded(event_id);
}
