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

#include <QPixmap>

#include "dractionlistmodel.h"

//
// Icons
//
#include "../../icons/input-16x16.xpm"
#include "../../icons/output-16x16.xpm"
#include "../../icons/xpoint-16x16.xpm"

DRActionListModel::DRActionListModel(int router,QObject *parent)
  : QAbstractTableModel(parent)
{
  d_router_number=router;
  d_sort_column=0;
  d_sort_order=Qt::AscendingOrder;

  //
  // Column Attributes
  //
  unsigned left=Qt::AlignLeft|Qt::AlignVCenter;
  unsigned center=Qt::AlignCenter;
  unsigned right=Qt::AlignRight|Qt::AlignVCenter;

  d_headers.push_back(tr("Comment"));            // 00
  d_alignments.push_back(left);
  d_keys.push_back("comment");
  d_icons.push_back(QPixmap(xpoint_16x16_xpm));

  d_headers.push_back(tr("Time"));               // 01
  d_alignments.push_back(center);
  d_keys.push_back("time");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Sun"));                // 02
  d_alignments.push_back(center);
  d_keys.push_back("sunday");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Mon"));                // 03
  d_alignments.push_back(center);
  d_keys.push_back("monday");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Tue"));                // 04
  d_alignments.push_back(center);
  d_keys.push_back("tuesday");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Wed"));                // 05
  d_alignments.push_back(center);
  d_keys.push_back("wednesday");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Thu"));                // 06
  d_alignments.push_back(center);
  d_keys.push_back("thursday");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Fri"));                // 07
  d_alignments.push_back(center);
  d_keys.push_back("friday");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Sat"));                // 08
  d_alignments.push_back(center);
  d_keys.push_back("saturday");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Src"));                // 09
  d_alignments.push_back(right);
  d_keys.push_back("source");
  d_icons.push_back(QPixmap(input_16x16_xpm));

  d_headers.push_back(tr("Source Name"));        // 10
  d_alignments.push_back(left);
  d_keys.push_back("sourceName");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Dst"));                // 11
  d_alignments.push_back(right);
  d_keys.push_back("destination");
  d_icons.push_back(QPixmap(output_16x16_xpm));

  d_headers.push_back(tr("Destination Name"));   // 12
  d_alignments.push_back(left);
  d_keys.push_back("destinationName");
  d_icons.push_back(QVariant());

  d_headers.push_back(tr("Id"));                 // 13
  d_alignments.push_back(right);
  d_keys.push_back("id");
  d_icons.push_back(QVariant());
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
  d_font_metrics=new QFontMetrics(d_font);
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
  return d_sorts.size();
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
  //  int row=index.row();
  int row=d_sorts.at(index.row());

  if(row<d_sorts.size()) {
    switch((Qt::ItemDataRole)role) {
    case Qt::DisplayRole:
      return d_texts.at(row).at(col);

    case Qt::DecorationRole:
      return d_icons.at(col);

    case Qt::TextAlignmentRole:
      return d_alignments.at(col);

    case Qt::FontRole:
      return d_font;

    case Qt::SizeHintRole:
      return QSize(ITEM_WIDTH_PADDING+
		   d_font_metrics->width(d_texts.at(row).at(col).toString()),
		   ITEM_HEIGHT);
      break;

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
  return d_ids.at(d_sorts.at(rownum));
}


QVariantMap DRActionListModel::rowMetadata(int rownum) const
{
  return d_metadatas.at(d_sorts.at(rownum));
}


void DRActionListModel::setRowMetadata(const QVariantMap &fields)
{
  int id=fields.value("id").toInt();
  int rownum=d_sorts.indexOf(d_ids.indexOf(id));

  if(rownum>=0) {
    UpdateRow(rownum,fields);
    emit dataChanged(index(rownum,0),index(rownum,10),
		     QVector<int>(1,Qt::DisplayRole));
  }
}


int DRActionListModel::rowNumber(int id) const
{
  return d_sorts.indexOf(d_ids.indexOf(id));
}


void DRActionListModel::addAction(const QVariantMap &fields)
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
  // Do we have this loaded already?
  //
  int id=fields.value("id").toInt();
  int rownum1=d_ids.indexOf(id);
  int rownum=d_sorts.indexOf(rownum1);
  if(rownum>=0) {
    //
    // Update existing line
    //
    UpdateRow(rownum1,fields);
    emit dataChanged(index(rownum,0),index(rownum,columnCount()));
    sort(d_sort_column,d_sort_order);
    return;
  }

  //
  // Add new line
  //
  QList<QVariant> row;
  for(int i=0;i<d_headers.size();i++) {
    row.push_back(QString());
  }
  beginInsertRows(QModelIndex(),d_sorts.size(),d_sorts.size());
  d_sorts.push_back(d_sorts.size());
  d_texts.push_back(row);
  d_ids.push_back(fields.value("id").toInt());
  d_metadatas.push_back(fields);
  UpdateRow(d_sorts.size()-1,fields);
  endInsertRows();

  sort(d_sort_column,d_sort_order);
}


void DRActionListModel::removeAction(int id)
{
  //  int rownum=d_sorts.indexOf(d_ids.indexOf(id));

  int rownum1=d_ids.indexOf(id);
  //  printf("rownum1: %d\n",rownum1);
  int rownum=d_sorts.indexOf(rownum1);
  //  printf("rownum2: %d\n",rownum);


  if(rownum>=0) {
    beginRemoveRows(QModelIndex(),rownum1,rownum1);
    d_texts.removeAt(rownum);
    d_metadatas.removeAt(rownum);
    d_ids.removeAt(rownum);
    d_sorts.removeAt(rownum);

    for(int i=0;i<d_sorts.size();i++) {
      if(d_sorts.at(i)>=rownum) {
	d_sorts[i]=d_sorts.at(i)-1;
      }
    }

    endRemoveRows();
  }
}


void DRActionListModel::sort(int col,Qt::SortOrder order)
{
  //  printf("DRActionListModel::sort(col: %d,order: %u)\n",col,order);
  d_sort_column=col;
  d_sort_order=order;

  //
  // FIXME: Use a more performant sort algorithm here!
  //
  bool changed=true;

  if(order==Qt::DescendingOrder) {
    while(changed) {
      changed=false;
      for(int i=1;i<d_sorts.size();i++) {
	if(d_texts.at(d_sorts.at(i)).at(col)>
	   d_texts.at(d_sorts.at(i-1)).at(col)) {
	  int index=d_sorts.at(i);
	  d_sorts[i]=d_sorts.at(i-1);
	  d_sorts[i-1]=index;
	  changed=true;
	}
      }
    }
  }

  if(order==Qt::AscendingOrder) {
    while(changed) {
      changed=false;
      for(int i=1;i<d_sorts.size();i++) {
	if(d_texts.at(d_sorts.at(i)).at(col)<
	   d_texts.at(d_sorts.at(i-1)).at(col)) {
	  int index=d_sorts.at(i);
	  d_sorts[i]=d_sorts.at(i-1);
	  d_sorts[i-1]=index;
	  changed=true;
	}
      }
    }
  }
  emit dataChanged(index(0,0),index(rowCount()-1,columnCount()-1),
  		   QVector<int>(1,Qt::DisplayRole));
}


void DRActionListModel::UpdateRow(int rownum,const QVariantMap &fields)
{
  d_texts[rownum][0]=fields.value("comment").toString();
  d_texts[rownum][1]=fields.value("time").toTime().toString("hh:mm:ss");
  d_texts[rownum][2]=DowMarker(fields.value("sunday").toBool(),tr("Sun"));
  d_texts[rownum][3]=DowMarker(fields.value("monday").toBool(),tr("Mon"));
  d_texts[rownum][4]=DowMarker(fields.value("tuesday").toBool(),tr("Tue"));
  d_texts[rownum][5]=DowMarker(fields.value("wednesday").toBool(),tr("Wed"));
  d_texts[rownum][6]=DowMarker(fields.value("thursday").toBool(),tr("Thu"));
  d_texts[rownum][7]=DowMarker(fields.value("friday").toBool(),tr("Fri"));
  d_texts[rownum][8]=DowMarker(fields.value("saturday").toBool(),tr("Sat"));
  d_texts[rownum][9]=QString::asprintf("%d",fields.value("source").toInt());
  d_texts[rownum][10]=fields.value("sourceName").toString();
  if(fields.value("sourceHostName").toString().isEmpty()) {
    d_texts[rownum][10]=d_texts.at(rownum).at(10).toString()+
      " "+tr("ON")+" "+fields.value("sourceHostAddress").toString();
  }
  else {
    d_texts[rownum][10]=d_texts.at(rownum).at(10).toString()+
      " "+tr("ON")+" "+fields.value("sourceHostName").toString();
  }
  d_texts[rownum][11]=
    QString::asprintf("%d",fields.value("destination").toInt());
  d_texts[rownum][12]=fields.value("destinationName").toString();
  if(fields.value("destinationHostName").toString().isEmpty()) {
    d_texts[rownum][12]=d_texts.at(rownum).at(10).toString()+
      " "+tr("ON")+" "+fields.value("destinationHostAddress").toString();
  }
  else {
    d_texts[rownum][12]=d_texts.at(rownum).at(10).toString()+
      " "+tr("ON")+" "+fields.value("destinationHostName").toString();
  }
  d_texts[rownum][13]=QString::asprintf("%d",fields.value("id").toInt());
  d_metadatas[rownum]=fields;
}


QString DRActionListModel::DowMarker(bool state,const QString &marker) const
{
  if(state) {
    return marker;
  }
  return QString("");
}
