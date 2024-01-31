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

#include "drendpointlistmodel.h"

DREndPointListModel::DREndPointListModel(int router,bool use_long_names,
				   QObject *parent)
  : QAbstractTableModel(parent)
{
  d_router_number=router;
  d_use_long_names=use_long_names;

  //
  // Column Attributes
  //
  unsigned left=Qt::AlignLeft|Qt::AlignVCenter;
  //unsigned center=Qt::AlignCenter;
  unsigned right=Qt::AlignRight|Qt::AlignVCenter;

  d_headers.push_back(tr("Id"));                // 00
  d_alignments.push_back(left);

  d_headers.push_back(tr("Number"));            // 01
  d_alignments.push_back(right);

  d_headers.push_back(tr("Name"));              // 02
  d_alignments.push_back(left);
}


DREndPointListModel::~DREndPointListModel()
{
}


void DREndPointListModel::setFont(const QFont &font)
{
  d_font=font;
}


int DREndPointListModel::columnCount(const QModelIndex &parent) const
{
  return d_headers.size();
}


int DREndPointListModel::rowCount(const QModelIndex &parent) const
{
  return d_texts.size();
}


QVariant DREndPointListModel::headerData(int section,Qt::Orientation orient,
				       int role) const
{
  if((orient==Qt::Horizontal)&&(role==Qt::DisplayRole)) {
    return d_headers.at(section);
  }
  return QVariant();
}


QVariant DREndPointListModel::data(const QModelIndex &index,int role) const
{
  QString str;
  int col=index.column();
  int row=index.row();

  if(row<d_texts.size()) {
    switch((Qt::ItemDataRole)role) {
    case Qt::DisplayRole:
      return d_texts.at(row).at(col);

    case Qt::DecorationRole:
      // Nothing to do!
      break;

    case Qt::TextAlignmentRole:
      return d_alignments.at(col);

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


int DREndPointListModel::endPointNumber(int rownum) const
{
  if(rownum<0) {
    return -1;
  }
  return d_numbers.at(rownum);
}


QMap<QString,QVariant> DREndPointListModel::endPointMetadata(int rownum)
{
  return d_metadatas.at(rownum);
}


int DREndPointListModel::rowNumber(int endpt) const
{
  return d_numbers.indexOf(endpt);
}


void DREndPointListModel::addEndPoint(const QMap<QString,QVariant> &fields)
{
  //
  // Sanity Checks
  //
  if(!fields.contains("number")) {
    syslog(LOG_WARNING,
	   "input on router %d missing required field \"number\", rejecting",
	   d_router_number);
    return;
  }
  int number=fields.value("number").toInt();

  if(!fields.contains("name")) {
    syslog(LOG_WARNING,
	   "input on router %d missing required field \"name\", rejecting",
	   d_router_number);
    return;
  }
  QString name=fields.value("name").toString();

  //
  // Insert It
  //
  d_raw_metadatas[number]=fields;

  //
  // Add Long Name Component
  //
  if(d_use_long_names&&fields.contains("hostName")) {
    d_raw_metadatas[number]["name"]=
      name+" "+tr("ON")+" "+fields.value("hostName").toString();
  }
}


void DREndPointListModel::finalize()
{
  //
  // Insert The Fields
  //
  beginInsertRows(QModelIndex(),0,d_raw_metadatas.size());
  for(QMap<int,QMap<QString,QVariant> >::const_iterator it=d_raw_metadatas.begin();
      it!=d_raw_metadatas.end();it++) {
    QList<QVariant> row;
    row.push_back(QString::asprintf("%d - %s",it.key(),
				    it.value().value("name").toString().
				    toUtf8().constData()));
    row.push_back(QString::asprintf("%d",it.key()));
    row.push_back(it.value().value("name").toString());
    d_texts.push_back(row);
    d_numbers.push_back(it.key());
    d_metadatas.push_back(it.value());
  }
  d_raw_metadatas.clear();
}
