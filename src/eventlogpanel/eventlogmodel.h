// eventlogmodel.h
//
// Data model for Drouter event log lines
//
//   (C) Copyright 2021-2022 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef EVENTLOGMODEL_H
#define EVENTLOGMODEL_H

#include <QAbstractTableModel>
#include <QFont>
#include <QList>
#include <QPalette>

#include "sqlquery.h"

class EventLogModel : public QAbstractTableModel
{
  Q_OBJECT
 public:
  enum EndpointAttributes {NumberAttribute=1,NameAttribute=2};
  EventLogModel(QObject *parent=0);
  ~EventLogModel();
  QPalette palette();
  void setPalette(const QPalette &pal);
  void setFont(const QFont &font);
  int columnCount(const QModelIndex &parent=QModelIndex()) const;
  int rowCount(const QModelIndex &parent=QModelIndex()) const;
  QVariant headerData(int section,Qt::Orientation orient,
		      int role=Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const;
  int lineId(const QModelIndex &row) const;
  QModelIndex refresh();
  void refresh(const QModelIndex &index);
  void refresh(int line_id);
  int showAttributes() const;

 public slots:
  void setShowAttributes(int attrs);

 protected:
  void updateModel();
  void updateRowLine(int line);
  void updateRow(int row,SqlQuery *q);
  QString sqlFields() const;

 private:
  QString RouteString(SqlQuery *q) const;
  QString RouteParameter(const QVariant &name,QColor *color) const;
  QString Fmt(const QString &str,const QColor &col,bool bold) const;
  QString Fmt(int num,const QColor &col,bool bold) const;
  int d_show_attributes;
  QPalette d_palette;
  QFont d_font;
  QFont d_bold_font;
  QVariant d_event_fail_icon;
  QVariant d_event_comment_icon;
  QVariant d_event_route_icon;
  QVariant d_event_snapshot_icon;
  QList<QVariant> d_headers;
  QList<QVariant> d_alignments;
  QList<QList<QVariant> > d_texts;
  QList<QVariant> d_icons;
  QList<int> d_line_ids;
  int d_max_id;
};


#endif  // EVENTLOGMODEL_H
