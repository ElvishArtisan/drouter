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


QDateTime RouteAction::nextRunsAt(const QDateTime &now) const
{
  QDateTime datetime(now.date(),d_time);

  //
  // Rest of Today
  //
  //  printf("DOW: %d  ACTIVE: %d\n",datetime.date().dayOfWeek(),
  //	 d_day_of_week[datetime.date().dayOfWeek()-1]);
  if(d_day_of_week[datetime.date().dayOfWeek()-1]) {
    if(now.time()<d_time) {
      return QDateTime(datetime.date(),d_time);
    }
  }

  //
  // Rest of the Week
  for(int i=0;i<7;i++) {
    datetime=datetime.addDays(1);
    if(d_day_of_week[datetime.date().dayOfWeek()-1]) {
      return QDateTime(datetime.date(),d_time);
    }
  }

  return QDateTime();
}


void RouteAction::update(DRSqlQuery *q)
{
  d_id=q->value(0).toInt();
  d_time=q->value(2).toTime();
  d_router_number=q->value(3).toInt();
  d_destination_number=q->value(4).toInt();
  d_source_number=q->value(5).toInt();
  d_comment=q->value(6).toString();
  for(int i=0;i<7;i++) {
    d_day_of_week[i]=q->value(i+7).toString()=="Y";
  }
}


QString RouteAction::sqlFields()
{
  return QString("select ")+
    "`PERM_SA_ACTIONS`.`ID`,"+                  // 00
    "`PERM_SA_ACTIONS`.`IS_ACTIVE`,"+           // 01
    "`PERM_SA_ACTIONS`.`TIME`,"+                // 02
    "`PERM_SA_ACTIONS`.`ROUTER_NUMBER`,"+       // 03
    "`PERM_SA_ACTIONS`.`DESTINATION_NUMBER`,"+  // 04
    "`PERM_SA_ACTIONS`.`SOURCE_NUMBER`,"+       // 05
    "`PERM_SA_ACTIONS`.`COMMENT`,"+             // 06
    "`PERM_SA_ACTIONS`.`MON`,"+                 // 07
    "`PERM_SA_ACTIONS`.`TUE`,"+                 // 08
    "`PERM_SA_ACTIONS`.`WED`,"+                 // 09
    "`PERM_SA_ACTIONS`.`THU`,"+                 // 10
    "`PERM_SA_ACTIONS`.`FRI`,"+                 // 11
    "`PERM_SA_ACTIONS`.`SAT`,"+                 // 12
    "`PERM_SA_ACTIONS`.`SUN` "+                 // 13
    "from `PERM_SA_ACTIONS` ";
}




RouteEngine::RouteEngine(QObject *parent)
  : QObject(parent)
{
  d_logger=new LoggerFront(this);

  d_time_engine=new TimeEngine(this);
  connect(d_time_engine,SIGNAL(timeout(int)),this,SLOT(actionData(int)));
}


QList<int> RouteEngine::nextActions(int router) const
{
  return d_next_action_ids.value(router);
}


bool RouteEngine::load()
{
  QList<int> routers;
  QString sql=RouteAction::sqlFields()+
    "where "+
    "`PERM_SA_ACTIONS`.`IS_ACTIVE`='Y' ";
    DRSqlQuery *q=new DRSqlQuery(sql);
  while(q->next()) {
    d_route_actions[q->value(0).toInt()]=new RouteAction(q);
    d_time_engine->addEvent(q->value(0).toInt(),q->value(2).toTime());
    if(!routers.contains(q->value(3).toInt())) {
      routers.push_back(q->value(3).toInt());
    }
  }

  //
  // Calculate Next Events
  //
  for(int i=0;i<routers.size();i++) {
    d_next_action_ids[routers.at(i)]=GetNextActionIds(routers.at(i));
    emit nextActionsChanged(routers.at(i),
			    d_next_action_ids.value(routers.at(i)));
  }

  syslog(LOG_DEBUG,"route engine: loaded %d actions",q->size());
  delete q;

  return true;
}


void RouteEngine::refresh(int action_id)
{
  int router=-1;
  RouteAction *raction=NULL; 
  QString sql=RouteAction::sqlFields()+"where "+
    QString::asprintf("`ID`=%d && ",action_id)+
    "`PERM_SA_ACTIONS`.`IS_ACTIVE`='Y' ";
  DRSqlQuery *q=new DRSqlQuery(sql);
  if(q->first()) {
    if((raction=d_route_actions.value(action_id))==NULL) {
      //
      // Add New Action
      //
      raction=new RouteAction(q);
      router=raction->routerNumber();
      d_route_actions[action_id]=raction;
      d_time_engine->addEvent(action_id,raction->time());
      syslog(LOG_DEBUG,"route engine: added new action, id: %d",action_id);
    }
    else {
      //
      // Update Existing Action
      //
      d_time_engine->removeEvent(action_id);
      raction->update(q);
      router=raction->routerNumber();
      d_time_engine->addEvent(action_id,raction->time());
      syslog(LOG_DEBUG,"route engine: updated action, id: %d",action_id);
    }
  }
  else {
    //
    // Remove Inactive/Deleted Action
    //
    if((raction=d_route_actions.value(action_id))!=NULL) {
      router=raction->routerNumber();
      d_time_engine->removeEvent(action_id);
      d_next_action_ids.remove(router);
      delete raction;
      d_route_actions.remove(action_id);
      syslog(LOG_DEBUG,"route engine: removed action, id: %d",action_id);
    }
  }
  delete q;

  d_next_action_ids[router]=GetNextActionIds(router);
  emit nextActionsChanged(router,d_next_action_ids.value(router));
}


void RouteEngine::actionData(int action_id)
{
  QDateTime now=QDateTime::currentDateTime();
  RouteAction *raction=d_route_actions.value(action_id);

  if(raction!=NULL) {
    if(raction->dayOfWeek(now.date().dayOfWeek())) {
      d_next_action_ids[raction->routerNumber()]=
	GetNextActionIds(raction->routerNumber());
      emit setCrosspoint(raction->routerNumber(),raction->destinationNumber(),
			 raction->sourceNumber());
      emit nextActionsChanged(raction->routerNumber(),
			      d_next_action_ids.value(raction->routerNumber()));
      d_logger->addRouteEvent(QHostAddress(),"Action Engine",
			      raction->routerNumber(),
			      raction->destinationNumber(),
			      raction->sourceNumber());
      syslog(LOG_DEBUG,"route engine: ran action, id: %d",action_id);
    }
  }
}


QList<int> RouteEngine::GetNextActionIds(int router) const
{
  int64_t interval=0x0FFFFFFFFFFFFFFF;
  QDateTime now=QDateTime::currentDateTime();
  QList<int> ids;

  for(QMap<int,RouteAction *>::const_iterator it=d_route_actions.begin();
      it!=d_route_actions.end();it++) {
    if(it.value()->routerNumber()==router) {
      /*
      printf("ID: %d  Time: %s  Next Run: %s\n",
	     it.value()->id(),
	     it.value()->time().toString("hh:mm:ss").toUtf8().constData(),
	     it.value()->nextRunsAt(now).toString("ddd @ hh:mm:ss").toUtf8().constData());
      */
      QDateTime next=it.value()->nextRunsAt(now);
      if(next.isValid()) {
	int64_t new_interval=now.msecsTo(next);
	if(new_interval<interval) {
	  interval=new_interval;
	  ids.clear();
	  ids.push_back(it.key());
	}
	else {
	  if(new_interval==interval) {
	    ids.push_back(it.key());
	  }
	}
      }
    }
  }
  return ids;
}
