// actiontorture.cpp
//
// Generate a torture test for Actions in the database
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

#include <stdio.h>
#include <stdlib.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>

#include <sy5/sycmdswitch.h>

#include <drouter/drsqlquery.h>

#include "actiontorture.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  QString db_hostname="localhost";
  QString db_username="drouter";
  QString db_password="drouter";
  QString db_dbname="drouter";

  if(getenv("DROUTER_TEST_HOSTNAME")!=NULL) {
    db_hostname=getenv("DROUTER_TEST_HOSTNAME");
  }

  SyCmdSwitch *cmd=new SyCmdSwitch("actiontorture",VERSION,ACTIONTORTURE_USAGE);
  for(int i=0;i<(cmd->keys());i++) {
    if(cmd->key(i)=="--hostname") {
      db_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--username") {
      db_username=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--password") {
      db_password=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--dbname") {
      db_dbname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"unknown argument \"%s\"\n",cmd->key(i).toUtf8().constData());
      exit(1);
    }
  }

  //
  // Connect to Database
  //
  QSqlDatabase db=QSqlDatabase::addDatabase("QMYSQL3");
  db.setHostName(db_hostname);
  db.setDatabaseName(db_dbname);
  db.setUserName(db_username);
  db.setPassword(db_password);
  if(!db.open()) {
    fprintf(stderr,"actiontorture: unable to open database [%s]\n",
	    db.lastError().driverText().toUtf8().constData());
    exit(1);
  }  

  QTime time(13,0,0);
  QTime end_time(13,59,59);
  QTime prev_time=time;
  int srcnum=0;
  
  QString sql=QString("delete from PERM_SA_ACTIONS");
  DRSqlQuery::apply(sql);
  
  sql="lock tables `PERM_SA_ACTIONS` write";
  DRSqlQuery::apply(sql);

  while((time<end_time)&&(time>=prev_time)) {
    //    int pt=QTime(0,0,0).secsTo(time);
    for(int i=0;i<8;i++) {
      sql=QString("insert into `PERM_SA_ACTIONS` set ")+
	"`TIME`='"+time.toString("hh:mm:ss")+"',"+
	"`SUN`='Y',"+
	"`MON`='Y',"+
	"`TUE`='Y',"+
	"`WED`='Y',"+
	"`THU`='Y',"+
	"`FRI`='Y',"+
	"`SAT`='Y',"+
	"`IS_ACTIVE`='Y',"+
	"`ROUTER_NUMBER`=0,"+
	QString::asprintf("`DESTINATION_NUMBER`=%d,",i)+
	//	QString::asprintf("`SOURCE_NUMBER`=%d,",pt)+
	QString::asprintf("`SOURCE_NUMBER`=%d,",srcnum%8)+
	"`COMMENT`='Torture Test'";
      DRSqlQuery::apply(sql);
    }
    prev_time=time;
    time=time.addSecs(2);
    srcnum++;
  }

  sql="unlock tables";
  DRSqlQuery::apply(sql);

  exit(0);
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();

  return a.exec();
}
