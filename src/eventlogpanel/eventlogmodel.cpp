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

EventLogModel::EventLogModel(QObject *parent)
  : QAbstractTableModel(parent)
{
  d_max_id=0;

  //
  // Load Icons
  //
  d_event_comment_icon=QVariant(QPixmap(event_comment_16x16_xpm));
  d_event_fail_icon=QVariant(QPixmap(event_fail_16x16_xpm));
  d_event_route_icon=QVariant(QPixmap(event_route_16x16_xpm));

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
      /*
      if(col==1) {
	return d_bold_font;
      }
      */
      return d_font;

    case Qt::TextColorRole:
      /*
      if(col==1) {
	return d_group_colors.at(row);
      }
      */
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


QModelIndex EventLogModel::refresh()
{
  QModelIndex ret;
  QList<QVariant> texts; 
  SqlQuery *q=NULL;
  QString sql=sqlFields()+" where "+
    "`STATUS`!='O' && "+
    QString::asprintf("`PERM_SA_EVENTS`.`ID`>%d ",d_max_id)+
    "order by `PERM_SA_EVENTS`.`ID` ";
  beginResetModel();
  //  d_line_ids.clear();
  //  d_texts.clear();
  //  d_icons.clear();
  q=new SqlQuery(sql);
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
    SqlQuery *q=new SqlQuery(sql);
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


void EventLogModel::updateModel()
{
  QList<QVariant> texts; 
  SqlQuery *q=NULL;
  QString sql=sqlFields()+" where "+
    "`STATUS`!='O' && "+
    QString::asprintf("`PERM_SA_EVENTS`.`ID`>%d ",d_max_id)+
    "order by `PERM_SA_EVENTS`.`ID` ";
  beginResetModel();
  d_line_ids.clear();
  //  d_group_colors.clear();
  d_texts.clear();
  d_icons.clear();
  q=new SqlQuery(sql);
  while(q->next()) {
    d_line_ids.push_back(-1);
    //    d_group_colors.push_back(QVariant());
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
    SqlQuery *q=new SqlQuery(sql);
    if(q->first()) {
      updateRow(line,q);
    }
    delete q;
  }
}


void EventLogModel::updateRow(int row,SqlQuery *q)
{
  QList<QVariant> texts;

  // Icon
  if(q->value(1).toString()=="Y") {
    if(q->value(2).toString()=="C") {  // Comment
      d_icons[row]=d_event_comment_icon;
    }
    if(q->value(2).toString()=="R") {  // Route
      d_icons[row]=d_event_route_icon;
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

  // Comment
  texts.push_back(q->value(4));

  d_texts[row]=texts;
}


QString EventLogModel::sqlFields() const
{
  QString sql=QString("select ")+
    "`ID`,"+        // 00
    "`STATUS`,"+    // 01
    "`TYPE`,"+      // 02
    "`DATETIME`,"+  // 03
    "`COMMENT` "+   // 04
    "from `PERM_SA_EVENTS` ";

    return sql;
}
