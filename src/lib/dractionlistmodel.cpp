// dractionlistmodel.cpp
//
// Qt Model for a list of Drouter actions
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

#include "dractionlistmodel.h"

DRActionListModel::DRActionListModel(int router,QObject *parent)
  : QAbstractTableModel(parent)
{
  d_router_number=router;

  //
  // Column Attributes
  //
  unsigned left=Qt::AlignLeft|Qt::AlignVCenter;
  unsigned center=Qt::AlignCenter;
  unsigned right=Qt::AlignRight|Qt::AlignVCenter;

  d_headers.push_back(tr("Time"));           // 00
  d_alignments.push_back(center);
  d_keys.push_back("time");

  d_headers.push_back(tr("Sun"));            // 01
  d_alignments.push_back(center);
  d_keys.push_back("sunday");

  d_headers.push_back(tr("Mon"));            // 02
  d_alignments.push_back(center);
  d_keys.push_back("monday");

  d_headers.push_back(tr("Tue"));            // 03
  d_alignments.push_back(center);
  d_keys.push_back("tuesday");

  d_headers.push_back(tr("Wed"));            // 04
  d_alignments.push_back(center);
  d_keys.push_back("wednesday");

  d_headers.push_back(tr("Thu"));            // 05
  d_alignments.push_back(center);
  d_keys.push_back("thursday");

  d_headers.push_back(tr("Fri"));            // 06
  d_alignments.push_back(center);
  d_keys.push_back("friday");

  d_headers.push_back(tr("Sat"));            // 07
  d_alignments.push_back(center);
  d_keys.push_back("saturday");

  d_headers.push_back(tr("Comment"));        // 08
  d_alignments.push_back(left);
  d_keys.push_back("comment");

  d_headers.push_back(tr("Source"));         // 09
  d_alignments.push_back(left);
  d_keys.push_back("source");

  d_headers.push_back(tr("Destination"));    // 10
  d_alignments.push_back(left);
  d_keys.push_back("destination");

  d_headers.push_back(tr("Id"));             // 11
  d_alignments.push_back(right);
  d_keys.push_back("id");
}


DRActionListModel::~DRActionListModel()
{
}


int DRActionListModel::routerNumber() const
{
  return d_router_number;
}


void DRActionListModel::setFont(const QFont &font)
{
  d_font=font;
}


void DRActionListModel::setInputsModel(DREndPointListModel *model)
{
  d_inputs_model=model;
}


void DRActionListModel::setOutputsModel(DREndPointListModel *model)
{
  d_outputs_model=model;
}


int DRActionListModel::columnCount(const QModelIndex &parent) const
{
  return d_headers.size();
}


int DRActionListModel::rowCount(const QModelIndex &parent) const
{
  return d_texts.size();
}


QVariant DRActionListModel::headerData(int section,Qt::Orientation orient,
				       int role) const
{
  if((orient==Qt::Horizontal)&&(role==Qt::DisplayRole)) {
    return d_headers.at(section);
  }
  return QVariant();
}


QVariant DRActionListModel::data(const QModelIndex &index,int role) const
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


int DRActionListModel::id(int rownum) const
{
  if(rownum<0) {
    return -1;
  }
  return d_ids.at(rownum);
}


QMap<QString,QVariant> DRActionListModel::actionMetadata(int rownum)
{
  return d_metadatas.at(rownum);
}


int DRActionListModel::rowNumber(int id) const
{
  return d_ids.indexOf(id);
}


void DRActionListModel::addAction(const QMap<QString,QVariant> &fields)
{
  //
  // Sanity Checks
  //
  for(int i=0;i<d_keys.size();i++) {
    if(!fields.contains(d_keys.at(i))) {
      syslog(LOG_WARNING,
	     "action on router %d missing required field \"%s\", rejecting",
	     d_router_number,d_keys.at(i).toUtf8().constData());
      return;
    }
  }
  QTime time=fields.value("time").toTime();
  if(!time.isValid()) {
  }

  //
  // Insert It
  //
  d_raw_metadatas[time]=fields;
}


void DRActionListModel::finalize()
{
  //
  // Insert The Fields
  //
  beginInsertRows(QModelIndex(),0,d_raw_metadatas.size());
  for(QMap<QTime,QMap<QString,QVariant> >::const_iterator it=d_raw_metadatas.begin();
      it!=d_raw_metadatas.end();it++) {
    QList<QVariant> row;
    row.push_back(it.value().value("time").toTime().toString("hh:mm:ss"));
    row.push_back(DowMarker(it.value().value("sunday").toBool(),tr("Sun")));
    row.push_back(DowMarker(it.value().value("monday").toBool(),tr("Mon")));
    row.push_back(DowMarker(it.value().value("tuesday").toBool(),tr("Tue")));
    row.push_back(DowMarker(it.value().value("wednesday").toBool(),tr("Wed")));
    row.push_back(DowMarker(it.value().value("thursday").toBool(),tr("Thu")));
    row.push_back(DowMarker(it.value().value("friday").toBool(),tr("Fri")));
    row.push_back(DowMarker(it.value().value("saturday").toBool(),tr("Sat")));
    row.push_back(it.value().value("comment"));
    int input=1+it.value().value("source").toInt();
    row.push_back(QString::asprintf("%d - %s",
			   input,
			   d_inputs_model->endPointMetadata(input).
			     value("name").toString().toUtf8().constData()));
    int output=1+it.value().value("destination").toInt();
    row.push_back(QString::asprintf("%d - %s",
			   output,
			   d_outputs_model->endPointMetadata(output).
			     value("name").toString().toUtf8().constData()));
    row.push_back(QString::asprintf("%d",it.value().value("id").toInt()));
    d_texts.push_back(row);

    d_ids.push_back(it.value().value("id").toInt());
    d_metadatas.push_back(it.value());
  }
  d_raw_metadatas.clear();
  endInsertRows();
}


QString DRActionListModel::DowMarker(bool state,const QString &marker) const
{
  if(state) {
    return marker;
  }
  return QString("");
}
