// drrouterlistmodel.cpp
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

#include <QPixmap>

#include "drrouterlistmodel.h"

//
// Icons
//
#include "../../icons/audio-16x16.xpm"
#include "../../icons/gpio-16x16.xpm"

DRRouterListModel::DRRouterListModel(QObject *parent)
  : QAbstractTableModel(parent)
{
  //
  // Column Attributes
  //
  unsigned left=Qt::AlignLeft|Qt::AlignVCenter;
  //unsigned center=Qt::AlignCenter;
  //unsigned right=Qt::AlignRight|Qt::AlignVCenter;

  d_headers.push_back(tr("Id"));
  d_alignments.push_back(left);

  d_headers.push_back(tr("Type"));
  d_alignments.push_back(left);
}


DRRouterListModel::~DRRouterListModel()
{
}


void DRRouterListModel::setFont(const QFont &font)
{
  d_font=font;
}


int DRRouterListModel::columnCount(const QModelIndex &parent) const
{
  return d_headers.size();
}


int DRRouterListModel::rowCount(const QModelIndex &parent) const
{
  return d_texts.size();
}


QVariant DRRouterListModel::headerData(int section,Qt::Orientation orient,
				       int role) const
{
  if((orient==Qt::Horizontal)&&(role==Qt::DisplayRole)) {
    return d_headers.at(section);
  }
  return QVariant();
}


QVariant DRRouterListModel::data(const QModelIndex &index,int role) const
{
  QString str;
  int col=index.column();
  int row=index.row();

  if(row<d_texts.size()) {
    switch((Qt::ItemDataRole)role) {
    case Qt::DisplayRole:
      return d_texts.at(row).at(col);

    case Qt::DecorationRole:
      return d_icons.at(row);

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


int DRRouterListModel::routerNumber(int rownum) const
{
  return d_numbers.at(rownum);
}


DREndPointMap::RouterType DRRouterListModel::routerType(int rownum) const
{
  return d_router_types.at(rownum);
}


int DRRouterListModel::rowNumber(int router) const
{
  return d_numbers.indexOf(router);
}


void DRRouterListModel::addRouter(int number,const QString &name,
				  const QString &rtype)
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
  row.push_back(rtype);
  d_texts.insert(index,row);
  if(rtype.toLower()=="audio") {
    d_router_types.insert(index,DREndPointMap::AudioRouter);
    d_icons.insert(index,QPixmap(audio_16x16_xpm));
  }
  else {
    if(rtype.toLower()=="gpio") {
    d_router_types.insert(index,DREndPointMap::GpioRouter);
      d_icons.insert(index,QPixmap(gpio_16x16_xpm));
    }
    else {
      d_router_types.insert(index,DREndPointMap::LastRouter);
      d_icons.insert(index,QVariant());
    }
  }
  endInsertRows();
}
