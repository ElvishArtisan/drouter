// eventlogmodel.cpp
//
// Data model for Drouter event log lines
//
//   (C) Copyright 2021-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QDateTime>
#include <QPixmap>

#include "eventlogmodel.h"

//
// Icons
//
#include "../../icons/event-fail-16x16.xpm"
#include "../../icons/event-comment-16x16.xpm"
#include "../../icons/event-route-16x16.xpm"
#include "../../icons/event-snapshot-16x16.xpm"

EventLogModel::EventLogModel(QObject *parent)
  : QAbstractTableModel(parent)
{
  d_max_id=0;
  d_show_attributes=EventLogModel::NumberAttribute|EventLogModel::NameAttribute;

  //
  // Load Icons
  //
  d_event_comment_icon=QVariant(QPixmap(event_comment_16x16_xpm));
  d_event_fail_icon=QVariant(QPixmap(event_fail_16x16_xpm));
  d_event_route_icon=QVariant(QPixmap(event_route_16x16_xpm));
  d_event_snapshot_icon=QVariant(QPixmap(event_snapshot_16x16_xpm));

  //
  // Column Attributes
  //
  unsigned left=Qt::AlignLeft|Qt::AlignVCenter;
  unsigned center=Qt::AlignCenter;
  //  unsigned right=Qt::AlignRight|Qt::AlignVCenter;

  d_headers.push_back(tr("Date/Time"));  // 00
  d_alignments.push_back(center);

  d_headers.push_back(tr("Comment"));    // 01
  d_alignments.push_back(left);

  updateModel();
}


EventLogModel::~EventLogModel()
{
}


QPalette EventLogModel::palette()
{
  return d_palette;
}


void EventLogModel::setPalette(const QPalette &pal)
{
  d_palette=pal;
}


void EventLogModel::setFont(const QFont &font)
{
  d_font=font;
  d_bold_font=font;
  d_bold_font.setWeight(QFont::Bold);
}


int EventLogModel::columnCount(const QModelIndex &parent) const
{
  return d_headers.size();
}


int EventLogModel::rowCount(const QModelIndex &parent) const
{
  return d_texts.size();
}


QVariant EventLogModel::headerData(int section,Qt::Orientation orient,
				    int role) const
{
  if((orient==Qt::Horizontal)&&(role==Qt::DisplayRole)) {
    return d_headers.at(section);
  }
  return QVariant();
}


QVariant EventLogModel::data(const QModelIndex &index,int role) const
{
  QString str;
  int col=index.column();
  int row=index.row();

  if(row<d_texts.size()) {
    switch((Qt::ItemDataRole)role) {
    case Qt::DisplayRole:
      return d_texts.at(row).at(col);

    case Qt::DecorationRole:
      if(col==0) {
	return d_icons.at(row);
      }
      break;

    case Qt::TextAlignmentRole:
      return d_alignments.at(col);

    case Qt::FontRole:
      return d_font;

    case Qt::TextColorRole:
      // Nothing to do here!
      break;

    case Qt::BackgroundRole:
      // Nothing to do here!
      break;

    default:
      break;
    }
  }

  return QVariant();
}


QModelIndex EventLogModel::refresh()
{
  QModelIndex ret;
  QList<QVariant> texts; 
  DRSqlQuery *q=NULL;
  QString sql=sqlFields()+" where "+
    "`STATUS`!='O' && "+
    QString::asprintf("`PERM_SA_EVENTS`.`ID`>%d ",d_max_id)+
    "order by `PERM_SA_EVENTS`.`ID` ";
  beginResetModel();
  q=new DRSqlQuery(sql);
  while(q->next()) {
    d_line_ids.push_back(-1);
    d_texts.push_back(texts);
    d_icons.push_back(QVariant());
    updateRow(d_texts.size()-1,q);
  }
  delete q;
  endResetModel();

  return createIndex(d_texts.size()-1,0);
}


void EventLogModel::refresh(const QModelIndex &row)
{
  if(row.row()<d_texts.size()) {
    QString sql=sqlFields()+
      "where "+
      QString::asprintf("`PERM_SA_EVENTS`.`ID`=%d",d_line_ids.at(row.row()));
    DRSqlQuery *q=new DRSqlQuery(sql);
    if(q->first()) {
      updateRow(row.row(),q);
      emit dataChanged(createIndex(row.row(),0),
		       createIndex(row.row(),columnCount()));
    }
    delete q;
  }
}


void EventLogModel::refresh(int line_id)
{
  for(int i=0;i<d_texts.size();i++) {
    if(d_line_ids.at(i)==line_id) {
      updateRowLine(i);
      return;
    }
  }
}


int EventLogModel::showAttributes() const
{
  return d_show_attributes;
}


void EventLogModel::setShowAttributes(int attrs)
{
  if(attrs!=d_show_attributes) {
    d_show_attributes=attrs;
    d_max_id=0;
    updateModel();
  }
}


void EventLogModel::updateModel()
{
  QList<QVariant> texts; 
  DRSqlQuery *q=NULL;
  QString sql=sqlFields()+" where "+
    "`STATUS`!='O' && "+
    QString::asprintf("`PERM_SA_EVENTS`.`ID`>%d ",d_max_id)+
    "order by `PERM_SA_EVENTS`.`ID` ";
  beginResetModel();
  d_line_ids.clear();
  d_texts.clear();
  d_icons.clear();
  q=new DRSqlQuery(sql);
  while(q->next()) {
    d_line_ids.push_back(-1);
    d_texts.push_back(texts);
    d_icons.push_back(QVariant());
    updateRow(d_texts.size()-1,q);
  }
  delete q;
  endResetModel();
}


void EventLogModel::updateRowLine(int line)
{
  if(line<d_texts.size()) {
    QString sql=sqlFields()+
      "where "+
      QString::asprintf("`PERM_SA_EVENTS`.`ID`=%d",d_line_ids.at(line));
    DRSqlQuery *q=new DRSqlQuery(sql);
    if(q->first()) {
      updateRow(line,q);
    }
    delete q;
  }
}


void EventLogModel::updateRow(int row,DRSqlQuery *q)
{
  QString str;
  QList<QVariant> texts;

  // Icon
  d_icons.push_back(QVariant());
  if(q->value(1).toString()=="Y") {
    switch(q->value(2).toString().at(0).cell()) {
    case 'C':  // Comment
      d_icons[row]=d_event_comment_icon;
      break;

    case 'R':  // Route
      d_icons[row]=d_event_route_icon;
      break;

    case 'S':  // Snapshot
      d_icons[row]=d_event_snapshot_icon;
      break;

    default:
      d_icons[row]=d_event_fail_icon;
    }
  }
  else {
    d_icons[row]=d_event_fail_icon;
  }

  // ID
  if(q->value(0).toInt()>d_max_id) {
    d_max_id=q->value(0).toInt();
  }
  d_line_ids[row]=q->value(0).toInt();

  // Date/Time
  texts.push_back(q->value(3).toDateTime().toString("yyyy-MM-dd hh:mm:ss"));

  switch(q->value(2).toString().at(0).cell()) {
  case 'R':  // Route
    if(q->value(1).toString()=="Y") {
      texts.push_back(Fmt(tr("Route taken"),Qt::black,false)+" - "+RouteString(q));
    }
    else {
      texts.push_back(Fmt(tr("Route failed"),Qt::red,true)+" - "+RouteString(q));
    }
    break;

  case 'C':  // Comment
    texts.push_back(q->value(7));
    break;

  case 'S':  // Snapshot
    str=q->value(7).toString()+" "+tr("by")+" ";
    if(!q->value(4).isNull()) {
      str+=Fmt(q->value(4).toString(),Qt::blue,true)+"@";
    }
    str+=Fmt(q->value(5).toString(),Qt::blue,true);
    texts.push_back(str);
    break;

  default:
    texts.push_back(tr("ERROR: invalid event type")+" \""+
		    q->value(2).toString()+"\"");
    break;
  }

  d_texts[row]=texts;
}


QString EventLogModel::sqlFields() const
{
  QString sql=QString("select ")+
    "`ID`,"+                   // 00
    "`STATUS`,"+               // 01
    "`TYPE`,"+                 // 02
    "`DATETIME`,"+             // 03
    "`USERNAME`,"+             // 04
    "`HOSTNAME`,"+             // 05
    "`ORIGINATING_ADDRESS`,"+  // 06
    "`COMMENT`,"+              // 07
    "`ROUTER_NUMBER`,"+        // 08
    "`ROUTER_NAME`,"+          // 09
    "`SOURCE_NUMBER`,"+        // 10
    "`SOURCE_NAME`,"+          // 11
    "`DESTINATION_NUMBER`,"+   // 12
    "`DESTINATION_NAME` "+     // 13
    "from `PERM_SA_EVENTS` ";

    return sql;
}


QString EventLogModel::RouteString(DRSqlQuery *q) const
{
  QString ret;
  QColor router_color;
  QString router_name=RouteParameter(q->value(9),&router_color);

  QColor input_color;
  QString input_name=RouteParameter(q->value(11),&input_color);

  QColor output_color;
  QString output_name=RouteParameter(q->value(13),&output_color);

  //
  // Route attributes
  //
  switch(d_show_attributes) {
  case EventLogModel::NumberAttribute:
    ret=tr("Router")+": "+Fmt(1+q->value(8).toInt(),router_color,true)+" "+
      tr("Dest")+": "+Fmt(1+q->value(12).toInt(),output_color,true)+" "+
      tr("Source")+": "+Fmt(1+q->value(10).toInt(),input_color,true);
    break;

  case EventLogModel::NameAttribute:
    ret=tr("Router")+": "+Fmt(router_name,router_color,true)+" "+
      tr("Dest")+": "+Fmt(output_name,output_color,true)+" "+
      tr("Source")+": "+Fmt(input_name,input_color,true);
    break;

    case EventLogModel::NumberAttribute|EventLogModel::NameAttribute:
      ret=tr("Router")+": "+Fmt(router_name,router_color,true)+"["+Fmt(1+q->value(8).toInt(),router_color,true)+"] "+
	tr("Dest")+": "+Fmt(output_name,output_color,true)+"["+Fmt(1+q->value(12).toInt(),output_color,true)+"] "+
	tr("Source")+": "+Fmt(input_name,input_color,true)+"["+Fmt(1+q->value(10).toInt(),input_color,true)+"]";
      break;
  }

  //
  // Connection attributes
  //
  ret+=" "+tr("by")+" ";
  if(!q->value(4).isNull()) {
    ret+=Fmt(q->value(4).toString(),Qt::blue,true)+"@";
  }
  ret+=Fmt(q->value(5).toString(),Qt::blue,true);

  return ret;
}


QString EventLogModel::RouteParameter(const QVariant &name,QColor *color) const
{
  QString ret=tr("NOT FOUND");
  *color=Qt::red;

  if(!name.toString().isEmpty()) {
    ret=name.toString();
    *color=Qt::black;
  }

  return ret;
}


QString EventLogModel::Fmt(const QString &str,const QColor &col,bool bold) const
{
  QString ret=str;

  if(col.isValid()) {
    ret="<font color=\""+col.name()+"\">"+
      str+
      "</font>";
  }
  if(bold) {
    ret="<strong>"+ret+"</strong>";
  }

  return ret;
}


QString EventLogModel::Fmt(int num,const QColor &col,bool bold) const
{
  return Fmt(QString::asprintf("%d",num),col,bold);
}
