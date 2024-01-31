// drendpointlistmodel.h
//
// Qt Model for a list of outputs.
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

#ifndef DRENDPOINTLISTMODEL_H
#define DRENDPOINTLISTMODEL_H

#include <QAbstractTableModel>
#include <QFont>
#include <QHostAddress>
#include <QMap>

class DREndPointListModel : public QAbstractTableModel
{
 Q_OBJECT;
 public:
  DREndPointListModel(int router,bool use_long_names,QObject *parent);
  ~DREndPointListModel();
  int routerNumber() const;
  void setFont(const QFont &font);
  int columnCount(const QModelIndex &parent=QModelIndex()) const;
  int rowCount(const QModelIndex &parent=QModelIndex()) const;
  QVariant headerData(int section,Qt::Orientation orient,
		      int role=Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const;
  int endPointNumber(int rownum) const;
  QMap<QString,QVariant> endPointMetadata(int rownum);
  int rowNumber(int endpt) const;
  void addEndPoint(const QMap<QString,QVariant> &fields);

 private:
  QFont d_font;
  QList<QVariant> d_headers;
  QList<QVariant> d_alignments;
  QList<QList<QVariant> > d_texts;
  QList<int> d_numbers;
  QList<QMap<QString,QVariant> > d_metadatas;
  int d_router_number;
  bool d_use_long_names;
};


#endif  // DRENDPOINTLISTMODEL_H
