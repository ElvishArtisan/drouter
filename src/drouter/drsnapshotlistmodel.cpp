// drendpointlistmodel.cpp
//
// Qt Model for a list of outputs
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

#include "drsnapshotlistmodel.h"

DRSnapshotListModel::DRSnapshotListModel(int router,QObject *parent)
  : QAbstractListModel(parent)
{
  d_router_number=router;
}


DRSnapshotListModel::~DRSnapshotListModel()
{
}


int DRSnapshotListModel::routerNumber() const
{
  return d_router_number;
}


void DRSnapshotListModel::setFont(const QFont &font)
{
  d_font=font;
}


int DRSnapshotListModel::rowCount(const QModelIndex &parent) const
{
  return d_names.size();
}


QVariant DRSnapshotListModel::headerData(int section,Qt::Orientation orient,
				       int role) const
{
  if((orient==Qt::Horizontal)&&(role==Qt::DisplayRole)) {
    return tr("Name");
  }
  return QVariant();
}


QVariant DRSnapshotListModel::data(const QModelIndex &index,int role) const
{
  QString str;
  int row=index.row();

  if(row<d_names.size()) {
    switch((Qt::ItemDataRole)role) {
    case Qt::DisplayRole:
      return d_names.at(row);

    case Qt::DecorationRole:
      // Nothing to do!
      break;

    case Qt::TextAlignmentRole:
      return Qt::AlignLeft;

    case Qt::FontRole:
      return d_font;

    case Qt::TextColorRole:
      // Nothing to do!
      break;

    case Qt::BackgroundRole:
      // Nothing to do!
      break;

    default:
      break;
    }
  }

  return QVariant();
}


QString DRSnapshotListModel::name(int rownum) const
{
  return d_names.at(rownum);
}


int DRSnapshotListModel::rowNumber(const QString &name) const
{
  return d_names.indexOf(name);
}


void DRSnapshotListModel::addSnapshot(const QString &name)
{
  beginInsertRows(QModelIndex(),d_names.size(),d_names.size());
  d_names.push_back(name);
  endInsertRows();
}
