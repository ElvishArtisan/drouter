// drinputlistmodel.cpp
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

#include "drinputlistmodel.h"

DRInputListModel::DRInputListModel(int router,QObject *parent)
  : QAbstractTableModel(parent)
{
  d_router_number=router;

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

  d_headers.push_back(tr("Host Description"));  // 03
  d_alignments.push_back(left);

  d_headers.push_back(tr("Host Address"));      // 04
  d_alignments.push_back(left);

  d_headers.push_back(tr("Host Name"));         // 05
  d_alignments.push_back(left);

  d_headers.push_back(tr("Slot"));              // 06
  d_alignments.push_back(right);

  d_headers.push_back(tr("Source Number"));     // 07
  d_alignments.push_back(right);

  d_headers.push_back(tr("Stream Address"));    // 08
  d_alignments.push_back(right);
}


DRInputListModel::~DRInputListModel()
{
}


void DRInputListModel::setFont(const QFont &font)
{
  d_font=font;
}


int DRInputListModel::columnCount(const QModelIndex &parent) const
{
  return d_headers.size();
}


int DRInputListModel::rowCount(const QModelIndex &parent) const
{
  return d_texts.size();
}


QVariant DRInputListModel::headerData(int section,Qt::Orientation orient,
				       int role) const
{
  if((orient==Qt::Horizontal)&&(role==Qt::DisplayRole)) {
    return d_headers.at(section);
  }
  return QVariant();
}


QVariant DRInputListModel::data(const QModelIndex &index,int role) const
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


int DRInputListModel::inputNumber(int rownum) const
{
  return d_numbers.at(rownum);
}


int DRInputListModel::rowNumber(int input) const
{
  return d_numbers.indexOf(input);
}


void DRInputListModel::addInput(int number,const QString &name,
				const QString &desc,
				const QHostAddress &host_addr,
				const QString &hostname,int slot,
				int srcnum,const QHostAddress &s_addr)
{
  int index=0;

  //
  // Find the insertion position
  //
  for(int i=0;i<d_numbers.size();i++) {
    if(number>d_numbers.at(i)) {
      index=i;
      break;
    }
  }

  //
  // Insert It
  //
  beginInsertRows(QModelIndex(),index,index);
  d_numbers.insert(index,number);
  QList<QVariant> row;
  row.push_back(QString::asprintf("%d - %s",number,name.toUtf8().constData()));
  row.push_back(QString::asprintf("%d",number));
  row.push_back(name);
  row.push_back(desc);
  row.push_back(host_addr.toString());
  row.push_back(hostname);
  row.push_back(QString::asprintf("%d",1+slot));
  row.push_back(QString::asprintf("%d",srcnum));
  row.push_back(s_addr.toString());

  d_texts.insert(index,row);
  endInsertRows();
}
