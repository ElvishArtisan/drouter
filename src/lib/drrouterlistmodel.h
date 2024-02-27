// drrouterlistmodel.h
//
// Qt Model for a list of Routers
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

#ifndef DRROUTERLISTMODEL_H
#define DRROUTERLISTMODEL_H

#include <QAbstractTableModel>
#include <QFont>

#include "drendpointmap.h"

class DRRouterListModel : public QAbstractTableModel
{
 Q_OBJECT;
 public:
  DRRouterListModel(QObject *parent=0);
  ~DRRouterListModel();
  void setFont(const QFont &font);
  int columnCount(const QModelIndex &parent=QModelIndex()) const;
  int rowCount(const QModelIndex &parent=QModelIndex()) const;
  QVariant headerData(int section,Qt::Orientation orient,
		      int role=Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const;
  int routerNumber(int rownum) const;
  DREndPointMap::RouterType routerType(int rownum) const;
  int rowNumber(int router) const;
  void addRouter(int number,const QString &name,const QString &rtype);
  void finalize();

 private:
  QFont d_font;
  QList<QVariant> d_headers;
  QList<QVariant> d_alignments;
  QList<QList<QVariant> > d_texts;
  QList<int> d_numbers;
  QList<QVariant> d_icons;
  QList<DREndPointMap::RouterType> d_router_types;
  QMap<int,QVariantMap> d_raw_metadatas;
};


#endif  // DRROUTERLISTMODEL_H
