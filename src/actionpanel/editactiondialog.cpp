// editactiondialog.cpp
//
// Dialog for editing a Drouter action.
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
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

#include "editactiondialog.h"

EditActionDialog::EditActionDialog(DRJParser *parser,QWidget *parent)
  : QDialog(parent)
{
  d_parser=parser;
  d_router=-1;
  d_fields=NULL;

  setModal(true);

  QFont bold_font(font().family(),font().pixelSize(),QFont::Bold);

  //
  // Event Active
  //
  d_active_check=new QCheckBox(this);
  d_active_label=new QLabel(tr("Event Active"),this);
  d_active_label->setFont(bold_font);
  
  //
  // Time
  //
  d_time_label=new QLabel(tr("Time")+":",this);
  d_time_label->setFont(bold_font);
  d_time_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_time_edit=new QTimeEdit(this);
  d_time_edit->setDisplayFormat("hh:mm:ss");

  //
  // Day-Of-Week Selector
  //
  d_dow_selector=new DowSelector(this);

  //
  // Comment
  //
  d_comment_label=new QLabel(tr("Comment")+":",this);
  d_comment_label->setFont(bold_font);
  d_comment_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_comment_edit=new QLineEdit(this);

  //
  // Source Selector
  //
  d_source_label=new QLabel(tr("Source")+":",this);
  d_source_label->setFont(bold_font);
  d_source_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_source_box=new QComboBox(this);

  //
  // Destination Selector
  //
  d_destination_label=new QLabel(tr("Destination")+":",this);
  d_destination_label->setFont(bold_font);
  d_destination_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_destination_box=new QComboBox(this);

  //
  // OK Button
  //
  d_ok_button=new QPushButton(tr("OK"),this);
  d_ok_button->setFont(bold_font);
  connect(d_ok_button,SIGNAL(clicked()),this,SLOT(okData()));

  //
  // Cancel Button
  //
  d_cancel_button=new QPushButton(tr("Cancel"),this);
  d_cancel_button->setFont(bold_font);
  connect(d_cancel_button,SIGNAL(clicked()),this,SLOT(cancelData()));
}


QSize EditActionDialog::sizeHint() const
{
  return QSize(520,225);
}


int EditActionDialog::exec(int router,QVariantMap *fields)
{
  d_router=router;
  d_fields=fields;

  DREndPointListModel *imodel=d_parser->inputModel(router);
  DREndPointListModel *omodel=d_parser->outputModel(router);

  if(fields->size()==0) {  // New Action
    setWindowTitle("ActionPanel - "+tr("Edit Action"));
    d_id=-1;
    d_time_edit->setTime(QTime(0,0,0));
    d_comment_edit->setText(tr("[new action]"));
    d_source_box->setModel(imodel);
    d_source_box->
      setCurrentIndex(imodel->rowNumber(1));
    d_destination_box->setModel(omodel);
    d_destination_box->
      setCurrentIndex(omodel->rowNumber(1));
    d_dow_selector->clear();
  }
  else {
    setWindowTitle("ActionPanel - "+tr("Edit Action")+
		   QString::asprintf("[ID: %d]",fields->value("id").toInt()));
    d_id=fields->value("id").toInt();
    d_time_edit->setTime(fields->value("time").toTime());
    d_comment_edit->setText(fields->value("comment").toString());
    d_source_box->setModel(imodel);
    d_source_box->
      setCurrentIndex(imodel->rowNumber(fields->value("source").toInt()));
    d_destination_box->setModel(omodel);
    d_destination_box->
      setCurrentIndex(omodel->rowNumber(fields->value("destination").toInt()));
    d_dow_selector->fromAction(*fields);
  }

  d_fields=fields;

  return QDialog::exec();
}


void EditActionDialog::closeEvent(QCloseEvent *e)
{
  cancelData();
}


void EditActionDialog::resizeEvent(QResizeEvent *e)
{
  int w=size().width();
  int h=size().height();

  d_active_check->setGeometry(10,3,20,20);
  d_active_label->setGeometry(30,3,200,20);

  d_time_edit->setGeometry(w-110,3,100,20);
  d_time_label->setGeometry(w-215,3,100,20);

  d_comment_label->setGeometry(10,25,115,20);
  d_comment_edit->setGeometry(130,25,w-140,20);

  d_source_label->setGeometry(10,47,115,20);
  d_source_box->setGeometry(130,47,w-140,20);

  d_destination_label->setGeometry(10,69,115,20);
  d_destination_box->setGeometry(130,69,w-140,20);

  d_dow_selector->setGeometry(10,91,d_dow_selector->sizeHint().width(),
			      d_dow_selector->sizeHint().height());

  d_ok_button->setGeometry(w-180,h-60,80,50);
  d_cancel_button->setGeometry(w-90,h-60,80,50);
}


void EditActionDialog::okData()
{
  DREndPointListModel *imodel=d_parser->inputModel(d_router);
  DREndPointListModel *omodel=d_parser->outputModel(d_router);

  (*d_fields)["id"]=d_id;
  (*d_fields)["time"]=d_time_edit->time();
  (*d_fields)["comment"]=d_comment_edit->text();
  (*d_fields)["source"]=imodel->endPointNumber(d_source_box->currentIndex());
  (*d_fields)["sourceName"]=
    imodel->rowMetadata(d_source_box->currentIndex()).value("name");
  (*d_fields)["destination"]=
    omodel->endPointNumber(d_destination_box->currentIndex());
  (*d_fields)["destinationName"]=
    omodel->rowMetadata(d_destination_box->currentIndex()).value("name");
  d_dow_selector->toAction(*d_fields);

  done(true);
}


void EditActionDialog::cancelData()
{
  done(false);
}
