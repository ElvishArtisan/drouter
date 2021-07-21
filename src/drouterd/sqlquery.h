// sqlquery.h
//
// Database driver with automatic error reporting
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

#ifndef SQLQUERY_H
#define SQLQUERY_H

#include <QString>
#include <QSqlQuery>
#include <QVariant>

class SqlQuery : public QSqlQuery
{
 public:
  SqlQuery(const QString &query);
  int columns() const;
  QVariant value(int index) const;
  static QVariant run(const QString &sql,bool *ok=NULL);
  static bool apply(const QString &sql,QString *err_msg=NULL);
  static int rows(const QString &sql);
  static QString escape(const QString &str);

 private:
  int sql_columns;
};


#endif  // SQLQUERY_H
