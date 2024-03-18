// logger_front.cpp
//
// Front end for the event logger in drouterd(8)
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

#include <drouter/drsqlquery.h>

#include "logger_front.h"

LoggerFront::LoggerFront(QObject *parent)
  : QObject(parent)
{
}


LoggerFront::~LoggerFront()
{
}


void LoggerFront::addRouteEvent(const QHostAddress &addr,
				const QString &username,
				int router,int output,int input)
{
  QString sql=QString("insert into `PERM_SA_EVENTS` set ")+
    "`DATETIME`=now(),"+
    "`TYPE`='R',"+
    "`ORIGINATING_ADDRESS`='"+addr.toString()+"',"+
    QString::asprintf("`ROUTER_NUMBER`=%d,",router)+
    QString::asprintf("`DESTINATION_NUMBER`=%d,",output)+
    QString::asprintf("`SOURCE_NUMBER`=%d,",input);
  if(username.isEmpty()) {
    sql+="`USERNAME`=NULL";
  }
  else {
    sql+="`USERNAME`='"+DRSqlQuery::escape(username)+"'";
  }
  d_event_lookups
    [QHostInfo::lookupHost(addr.toString(),
     this,SLOT(routeHostLookupFinishedData(const QHostInfo &)))]=
    DRSqlQuery::run(sql).toInt();
}


void LoggerFront::addSnapEvent(const QHostAddress &addr,
			       const QString &username,
			       int router,const QString &name)
{
  QString sql=QString("insert into `PERM_SA_EVENTS` set ")+
    "`DATETIME`=now(),"+
    "`STATUS`='Y',"+
    "`TYPE`='S',"+
    "`ORIGINATING_ADDRESS`='"+addr.toString()+"',"+
    QString::asprintf("`ROUTER_NUMBER`=%d,",router)+
    "`COMMENT`='"+tr("Executing snapshot")+" "+
    DRSqlQuery::escape("<strong>"+name+"</strong>")+" - "+
    tr("Router")+": "+QString::asprintf("<strong>%d</strong>",1+router)+"',";
  if(username.isEmpty()) {
    sql+="`USERNAME`=NULL";
  }
  else {
    sql+="`USERNAME`='"+DRSqlQuery::escape(username)+"'";
  }
  d_event_lookups
    [QHostInfo::lookupHost(addr.toString(),
     this,SLOT(snapHostLookupFinishedData(const QHostInfo &)))]=
    DRSqlQuery::run(sql).toInt();
}


void LoggerFront::writeCommentEvent(const QString &str)
{
  QString sql;

  sql=QString("insert into `PERM_SA_EVENTS` set ")+
    "`TYPE`='C',"+
    "`DATETIME`=now(),"+
    "`STATUS`='Y',"+
    "`COMMENT`='"+DRSqlQuery::escape(str)+"'";
  int evt_id=DRSqlQuery::run(sql).toInt();
  emit eventAdded(evt_id);
}


void LoggerFront::routeHostLookupFinishedData(const QHostInfo &info)
{
  QString sql=QString("update `PERM_SA_EVENTS` set ")+
    "`HOSTNAME`='"+DRSqlQuery::escape(info.hostName())+"' where "+
    QString::asprintf("`ID`=%d",d_event_lookups.value(info.lookupId()));
  DRSqlQuery::apply(sql);
  d_event_lookups.remove(info.lookupId());
}


void LoggerFront::snapHostLookupFinishedData(const QHostInfo &info)
{
  QString sql=QString("update `PERM_SA_EVENTS` set ")+
    "`HOSTNAME`='"+DRSqlQuery::escape(info.hostName())+"' where "+
    QString::asprintf("`ID`=%d",d_event_lookups.value(info.lookupId()));
  DRSqlQuery::apply(sql);
  emit eventAdded(info.lookupId());
  d_event_lookups.remove(info.lookupId());
}
