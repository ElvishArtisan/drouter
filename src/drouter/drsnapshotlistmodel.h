// drsnapshotlistmodel.h
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

#ifndef DRSNAPSHOTLISTMODEL_H
#define DRSNAPSHOTLISTMODEL_H

#include <QAbstractListModel>
#include <QFont>
#include <QHostAddress>
#include <QMap>

class DRSnapshotListModel : public QAbstractListModel
{
 Q_OBJECT;
 public:
  DRSnapshotListModel(int router,QObject *parent);
  ~DRSnapshotListModel();
  int routerNumber() const;
  void setFont(const QFont &font);
  int rowCount(const QModelIndex &parent=QModelIndex()) const;
  QVariant headerData(int section,Qt::Orientation orient,
		      int role=Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const;
  QString name(int rownum) const;
  int rowNumber(const QString &name) const;
  void addSnapshot(const QString &name);

 private:
  QFont d_font;
  QStringList d_names;
  int d_router_number;
};


#endif  // DRSNAPSHOTLISTMODEL_H
