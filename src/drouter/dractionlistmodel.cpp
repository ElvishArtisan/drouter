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
  d_time_format="hh:mm:ss";

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

  //
  // Active Roles
  //
  d_active_roles.push_back(Qt::DisplayRole);
  d_active_roles.push_back(Qt::SizeHintRole);
  d_active_roles.push_back(Qt::TextColorRole);
  d_active_roles.push_back(Qt::BackgroundRole);
  /*
   * We return these, but they should never change
   *
  d_active_roles.push_back(Qt::DecorationRole);
  d_active_roles.push_back(Qt::AlignmentRole);
  d_active_roles.push_back(Qt::FontRole);
  */
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


void DRActionListModel::setPalette(const QPalette &pal)
{
  d_palette=pal;
}


void DRActionListModel::setInputsModel(DREndPointListModel *model)
{
  d_inputs_model=model;
}


void DRActionListModel::setOutputsModel(DREndPointListModel *model)
{
  d_outputs_model=model;
}


QString DRActionListModel::timeFormat() const
{
  return d_time_format;
}


void DRActionListModel::setTimeFormat(const QString &fmt)
{
  if(fmt!=d_time_format) {
    for(int i=0;i<d_metadatas.size();i++) {
      d_texts[i][1]=
	QTime::fromString(d_metadatas.at(i).value("time").
			  toString(),"hh:mm:ss").toString(d_time_format);
    }
    d_time_format=fmt;
    emit dataChanged(index(0,1),index(rowCount()-1,1),
		     QVector<int>(1,Qt::DisplayRole));
  }
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
	  d_font_metrics->horizontalAdvance(d_texts.at(row).at(col).toString()),
		   ITEM_HEIGHT);
      break;

    case Qt::TextColorRole:
      if(!d_actives.at(row)) {
	return d_palette.color(QPalette::Disabled,QPalette::WindowText);
      }
      break;

    case Qt::BackgroundRole:
      if(d_next_ids.contains(d_ids.at(row))) {
	return QColor(Qt::yellow);
      }
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


int DRActionListModel::rowNumber(int id) const
{
  return d_sorts.indexOf(d_ids.indexOf(id));
}


void DRActionListModel::addAction(const QVariantMap &fields)
{
  //  printf("%p: DRActionListMode::addAction(id=%d)\n",this,fields.value("id").toInt());

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
  if(d_ids.contains(id)) {
    UpdateRowMetadata(fields);
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
  d_ids.push_back(id);
  d_metadatas.push_back(fields);
  d_actives.push_back(true);
  UpdateRow(d_sorts.size()-1,fields);
  endInsertRows();

  sort(d_sort_column,d_sort_order);
}


void DRActionListModel::removeAction(int id)
{
  int linenum=d_ids.indexOf(id);
  int rownum=d_sorts.indexOf(linenum);

  if(rownum>=0) {
    beginRemoveRows(QModelIndex(),rownum,rownum);
    d_texts.removeAt(linenum);
    d_metadatas.removeAt(linenum);
    d_ids.removeAt(linenum);
    d_actives.removeAt(linenum);
    d_sorts.removeAt(rownum);
    for(int i=0;i<d_sorts.size();i++) {
      if(d_sorts.at(i)>=linenum) {
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
  // FIXME: Bubble sort. Seriously?
  //
  bool changed=true;

  if(order==Qt::DescendingOrder) {
    while(changed) {
      changed=false;
      for(int i=1;i<d_sorts.size();i++) {
	if(d_texts.at(d_sorts.at(i)).at(col).toString()>
	   d_texts.at(d_sorts.at(i-1)).at(col).toString()) {
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
	if(d_texts.at(d_sorts.at(i)).at(col).toString()<
	   d_texts.at(d_sorts.at(i-1)).at(col).toString()) {
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


void DRActionListModel::updateNextActions(int router,
					  const QList<int> &action_ids)
{
  //
  // Invalidate Old IDs
  //
  while(d_next_ids.size()>0) {
    int rownum=d_sorts.indexOf(d_ids.indexOf(d_next_ids.first()));
    d_next_ids.removeFirst();
    emit dataChanged(index(rownum,0),index(rownum,columnCount()-1),
		     QVector<int>(1,Qt::BackgroundRole));
  }

  //
  // Assert New IDs
  //
  for(int i=0;i<action_ids.size();i++) {
    d_next_ids.push_back(action_ids.at(i));
    int rownum=d_sorts.indexOf(d_ids.indexOf(action_ids.at(i)));
    emit dataChanged(index(rownum,0),index(rownum,columnCount()-1),
		     QVector<int>(1,Qt::BackgroundRole));
  }
}


void DRActionListModel::UpdateRowMetadata(const QVariantMap &fields)
{
  printf("DRActionListMode::UpdateRowMetadata(id=%d)\n",fields.value("id").toInt());
  int linenum=-1;
  int id=fields.value("id").toInt();

  for(int i=0;i<d_metadatas.size();i++) {
    if(d_metadatas.at(i).value("id").toInt()==id) {
      linenum=i;
      break;
    }
  }
  if(linenum<0) {
    syslog(LOG_WARNING,"attempted to update non-existing action id %d",id);
    return;
  }
  int rownum=d_sorts.indexOf(linenum);
  d_metadatas[linenum]=fields;
  UpdateRow(linenum,fields);
  emit dataChanged(index(rownum,0),index(rownum,d_headers.size()-1),
		   d_active_roles);
}


void DRActionListModel::UpdateRow(int linenum,const QVariantMap &fields)
{
  d_texts[linenum][0]=fields.value("comment").toString();
  d_texts[linenum][1]=fields.value("time").toTime().toString(d_time_format);
  d_texts[linenum][2]=DowMarker(fields.value("sunday").toBool(),tr("Sun"));
  d_texts[linenum][3]=DowMarker(fields.value("monday").toBool(),tr("Mon"));
  d_texts[linenum][4]=DowMarker(fields.value("tuesday").toBool(),tr("Tue"));
  d_texts[linenum][5]=DowMarker(fields.value("wednesday").toBool(),tr("Wed"));
  d_texts[linenum][6]=DowMarker(fields.value("thursday").toBool(),tr("Thu"));
  d_texts[linenum][7]=DowMarker(fields.value("friday").toBool(),tr("Fri"));
  d_texts[linenum][8]=DowMarker(fields.value("saturday").toBool(),tr("Sat"));
  d_texts[linenum][9]=QString::asprintf("%d",fields.value("source").toInt());
  d_texts[linenum][10]=fields.value("sourceName").toString();
  if(fields.value("sourceHostName").toString().isEmpty()) {
    d_texts[linenum][10]=d_texts.at(linenum).at(10).toString()+
      " "+tr("ON")+" "+fields.value("sourceHostAddress").toString();
  }
  else {
    d_texts[linenum][10]=d_texts.at(linenum).at(10).toString()+
      " "+tr("ON")+" "+fields.value("sourceHostName").toString();
  }
  d_texts[linenum][11]=
    QString::asprintf("%d",fields.value("destination").toInt());
  d_texts[linenum][12]=fields.value("destinationName").toString();
  if(fields.value("destinationHostName").toString().isEmpty()) {
    d_texts[linenum][12]=d_texts.at(linenum).at(10).toString()+
      " "+tr("ON")+" "+fields.value("destinationHostAddress").toString();
  }
  else {
    d_texts[linenum][12]=d_texts.at(linenum).at(10).toString()+
      " "+tr("ON")+" "+fields.value("destinationHostName").toString();
  }
  d_texts[linenum][13]=QString::asprintf("%d",fields.value("id").toInt());
  d_actives[linenum]=fields.value("isActive").toBool();
  d_metadatas[linenum]=fields;
}


QString DRActionListModel::DowMarker(bool state,const QString &marker) const
{
  if(state) {
    return marker;
  }
  return QString("");
}
