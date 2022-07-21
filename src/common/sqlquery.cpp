// sqlquery.cpp
//
//   Database driver with error reporting
//
//   (C) Copyright 2007 Dan Mills <dmills@exponent.myzen.co.uk>
//   (C) Copyright 2018-2021 Fred Gleason <fredg@paravelsystems.com>
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

#include <netdb.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <QObject>
#include <QString>
#include <QTextCodec>
#include <QTranslator>
#include <QSqlError>
#include <QStringList>
#include <QVariant>

#include "sqlquery.h"

SqlQuery::SqlQuery (const QString &query):
  QSqlQuery(query)
{
  QSqlDatabase db;
  QString err_msg;
  sql_columns=0;
  /*
  if (!isActive() && reconnect) {
    db = QSqlDatabase::database();

    if (db.open()) {
      clear();
      exec(query);
      err_msg=QObject::tr("DB connection re-established");
    }
    else {
      err_msg=QObject::tr("Could not re-establish DB connection")+
      +"["+db.lastError().text()+"]";
    }

    fprintf(stderr,"%s\n",err_msg.toUtf8().constData());
    if(rda!=NULL) {
      rda->syslog(LOG_ERR,err_msg.toUtf8().constData());
    }
  }
  */
  if(isActive()) {
    //printf("QUERY: %s\n",(const char *)query.toUtf8());
    QStringList f0=query.split(" ");
    if(f0[0].toLower()=="select") {
      for(int i=1;i<f0.size();i++) {
	if(f0[i].toLower()=="from") {
	  QString fields;
	  for(int j=1;j<i;j++) {
	    fields+=f0[j];
	  }
	  QStringList f1=fields.split(",");
	  sql_columns=f1.size();
	  continue;
	}
      }
    }
  }
  else {
    err_msg=QObject::tr("invalid SQL or failed DB connection")+
      +"["+lastError().text()+"]: "+query;

    syslog(LOG_WARNING,"%s",err_msg.toUtf8().constData());
  }
}


int SqlQuery::columns() const
{
  return sql_columns;
}


QVariant SqlQuery::value(int index) const
{
  QVariant ret=QSqlQuery::value(index);

  if(!ret.isValid()) {
    syslog(LOG_WARNING,"for query: %s",executedQuery().toUtf8().constData());
  }

  return ret;
}


QVariant SqlQuery::run(const QString &sql,bool *ok)
{
  QVariant ret;

  SqlQuery *q=new SqlQuery(sql);
  if(ok!=NULL) {
    *ok=q->isActive();
  }
  ret=q->lastInsertId();
  delete q;

  return ret;
}


bool SqlQuery::apply(const QString &sql,QString *err_msg)
{
  bool ret=false;

  SqlQuery *q=new SqlQuery(sql);
  ret=q->isActive();
  if((err_msg!=NULL)&&(!ret)) {
    *err_msg="sql error: "+q->lastError().text()+" query: "+sql;
  }
  delete q;

  return ret;
}


int SqlQuery::rows(const QString &sql)
{
  int ret=0;

  SqlQuery *q=new SqlQuery(sql);
  ret=q->size();
  delete q;

  return ret;
}


QString SqlQuery::escape(const QString &str)
{
  QString res;

  for(int i=0;i<str.length();i++) {
    bool modified=false;
    if(str.at(i)==QChar('"')) {
      res+=QString("\\\"");
      modified=true;
    }

    if(str.at(i)==QChar('`')) {
      res+=QString("\\`");
      modified=true;
    }

    if(str.at(i)==QChar('\'')) {
      res+=QString("\\\'");
      modified=true;
    }

    if(str.at(i)==QChar('\\')) {
      res+=QString("\\");
      res+=QString("\\");
      modified=true;
    }

    if(!modified) {
      res+=str.at(i);
    }
  }

  return res;
}
