// routeengine.cpp
//
// Run Route Actions
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

#include <syslog.h>

#include <drouter/drsqlquery.h>

#include "routeengine.h"

RouteAction::RouteAction(DRSqlQuery *q)
{
  update(q);
}


int RouteAction::id() const
{
  return d_id;
}


QTime RouteAction::time() const
{
  return d_time;
}


bool RouteAction::dayOfWeek(int dow) const
{
  return d_day_of_week[dow-1];
}


int RouteAction::routerNumber() const
{
  return d_router_number;
}


int RouteAction::destinationNumber() const
{
  return d_destination_number;
}


int RouteAction::sourceNumber() const
{
  return d_source_number;
}


QString RouteAction::comment() const
{
  return d_comment;
}


void RouteAction::update(DRSqlQuery *q)
{
  d_id=q->value(0).toInt();
  d_time=q->value(1).toTime();
  d_router_number=q->value(2).toInt();
  d_destination_number=q->value(3).toInt();
  d_source_number=q->value(4).toInt();
  d_comment=q->value(5).toString();
  for(int i=0;i<7;i++) {
    d_day_of_week[i]=q->value(i+6).toString()=="Y";
  }
}


QString RouteAction::sqlFields()
{
  return QString("select ")+
    "`PERM_SA_ACTIONS`.`ID`,"+                  // 00
    "`PERM_SA_ACTIONS`.`TIME`,"+                // 01
    "`PERM_SA_ACTIONS`.`ROUTER_NUMBER`,"+       // 02
    "`PERM_SA_ACTIONS`.`DESTINATION_NUMBER`,"+  // 03
    "`PERM_SA_ACTIONS`.`SOURCE_NUMBER`,"+       // 04
    "`PERM_SA_ACTIONS`.`COMMENT`,"+             // 05
    "`PERM_SA_ACTIONS`.`MON`,"+                 // 06
    "`PERM_SA_ACTIONS`.`TUE`,"+                 // 07
    "`PERM_SA_ACTIONS`.`WED`,"+                 // 08
    "`PERM_SA_ACTIONS`.`THU`,"+                 // 09
    "`PERM_SA_ACTIONS`.`FRI`,"+                 // 10
    "`PERM_SA_ACTIONS`.`SAT`,"+                 // 11
    "`PERM_SA_ACTIONS`.`SUN` "+                 // 12
    "from `PERM_SA_ACTIONS` ";
}




RouteEngine::RouteEngine(QObject *parent)
  : QObject(parent)
{
  d_time_engine=new TimeEngine(this);
  connect(d_time_engine,SIGNAL(timeout(int)),this,SLOT(actionData(int)));
}


bool RouteEngine::load()
{
  QString sql=RouteAction::sqlFields();
  DRSqlQuery *q=new DRSqlQuery(sql);
  while(q->next()) {
    d_route_actions[q->value(0).toInt()]=new RouteAction(q);
    d_time_engine->addEvent(q->value(0).toInt(),q->value(1).toTime());
  }
  syslog(LOG_DEBUG,"route engine: loaded %d actions",q->size());
  delete q;

  return true;
}


void RouteEngine::refresh(int action_id)
{
  RouteAction *raction=NULL; 
  QString sql=RouteAction::sqlFields()+"where "+
    QString::asprintf("`ID`=%d ",action_id);
  DRSqlQuery *q=new DRSqlQuery(sql);
  if(q->first()) {
    if((raction=d_route_actions.value(action_id))==NULL) {
      raction=new RouteAction(q);  // Add new action
      d_route_actions[action_id]=raction;
      d_time_engine->addEvent(action_id,raction->time());
      syslog(LOG_DEBUG,"route engine: added new action, id: %d",action_id);
    }
    else {
      d_time_engine->removeEvent(action_id);  // Update existing action
      raction->update(q);
      d_time_engine->addEvent(action_id,raction->time());
      syslog(LOG_DEBUG,"route engine: updated action, id: %d",action_id);
    }
  }
  else {
    d_time_engine->removeEvent(action_id);  // Remove deleted action
    delete raction;
    d_route_actions.remove(action_id);
    syslog(LOG_DEBUG,"route engine: removed action, id: %d",action_id);
  }
  delete q;
}


void RouteEngine::actionData(int action_id)
{
  QDateTime now=QDateTime::currentDateTimeUtc();
  RouteAction *raction=d_route_actions.value(action_id);

  if(raction!=NULL) {
    if(raction->dayOfWeek(now.date().dayOfWeek())) {
      emit setCrosspoint(raction->routerNumber(),raction->destinationNumber(),
			 raction->sourceNumber());
      syslog(LOG_DEBUG,"route engine: ran action, id: %d",action_id);
    }
  }
}
