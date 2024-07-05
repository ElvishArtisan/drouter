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
#include <QPalette>
#include <QStringList>
#include <QTime>
#include <QVector>

#include <drouter/drendpointlistmodel.h>

#define ITEM_WIDTH_PADDING 30
#define ITEM_HEIGHT 30

class DRActionListModel : public QAbstractTableModel
{
 Q_OBJECT;
 public:
  DRActionListModel(int router,QObject *parent);
  ~DRActionListModel();
  int routerNumber() const;
  void setFont(const QFont &font);
  void setPalette(const QPalette &pal);
  QString timeFormat() const;
  void setTimeFormat(const QString &fmt);
  int columnCount(const QModelIndex &parent=QModelIndex()) const;
  int rowCount(const QModelIndex &parent=QModelIndex()) const;
  QVariant headerData(int section,Qt::Orientation orient,
		      int role=Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const;
  int id(int rownum) const;
  QVariantMap rowMetadata(int rownum) const;
  int rowNumber(int id) const;
  void addAction(const QVariantMap &fields);
  void removeAction(int id);
  void sort(int col,Qt::SortOrder order=Qt::AscendingOrder);

 public slots:
  void updateNextActions(int router,const QList<int> &action_ids);

 private:
  void UpdateRowMetadata(const QVariantMap &fields);
  void UpdateRow(int linenum,const QVariantMap &fields);
  QString DowMarker(bool state,const QString &marker) const;
  //
  // Column Fields
  //
  QList<QVariant> d_headers;
  QList<QVariant> d_icons;
  QStringList d_keys;
  QList<QVariant> d_alignments;

  //
  // Row Fields
  //
  QList<QList<QVariant> > d_texts;
  QList<int> d_ids;
  QList<QVariantMap> d_metadatas;
  QList<bool> d_actives;
  QList<int> d_sorts;

  int d_router_number;
  QList<int> d_next_ids;
  QFont d_font;
  QPalette d_palette;
  QVector<int> d_active_roles;
  QFontMetrics *d_font_metrics;
  int d_sort_column;
  Qt::SortOrder d_sort_order;
  QString d_time_format;
};


#endif  // DRACTIONLISTMODEL_H
