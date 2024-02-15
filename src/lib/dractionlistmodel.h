// dractionlistmodel.h
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

#ifndef DRACTIONLISTMODEL_H
#define DRACTIONLISTMODEL_H

#include <QAbstractTableModel>
#include <QFont>
#include <QFontMetrics>
#include <QHostAddress>
#include <QMap>
#include <QStringList>
#include <QTime>

#include <drendpointlistmodel.h>

class DRActionListModel : public QAbstractTableModel
{
 Q_OBJECT;
 public:
  DRActionListModel(int router,QObject *parent);
  ~DRActionListModel();
  int routerNumber() const;
  void setFont(const QFont &font);
  void setInputsModel(DREndPointListModel *model);
  void setOutputsModel(DREndPointListModel *model);
  int columnCount(const QModelIndex &parent=QModelIndex()) const;
  int rowCount(const QModelIndex &parent=QModelIndex()) const;
  QVariant headerData(int section,Qt::Orientation orient,
		      int role=Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const;
  int id(int rownum) const;
  QMap<QString,QVariant> rowMetadata(int rownum);
  int rowNumber(int id) const;
  void addAction(const QMap<QString,QVariant> &fields);
  void finalize();

 private:
  QString DowMarker(bool state,const QString &marker) const;
  QFont d_font;
  QFontMetrics *d_font_metrics;
  QList<QVariant> d_headers;
  QList<QVariant> d_icons;
  QStringList d_keys;
  QList<QVariant> d_alignments;
  QList<QList<QVariant> > d_texts;
  QList<int> d_ids;
  QMap<QTime,QMap<QString,QVariant> > d_raw_metadatas;
  QList<QMap<QString,QVariant> > d_metadatas;
  int d_router_number;
  DREndPointListModel *d_inputs_model;
  DREndPointListModel *d_outputs_model;
};


#endif  // DRACTIONLISTMODEL_H
