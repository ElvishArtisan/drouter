// panelwidget.cpp
//
// Output panel widget for OutputPanel
//
//   (C) Copyright 2016-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QtAlgorithms>
#include <QMessageBox>

#include "panelwidget.h"

PanelInput::PanelInput(unsigned num,const QString &name)
{
  panel_number=num;
  panel_name=name;
}


unsigned PanelInput::number() const
{
  return panel_number;
}


QString PanelInput::name() const
{
  return panel_name;
}


bool PanelInput::operator<(const PanelInput &other) const
{
  return panel_name<other.name();
}




PanelWidget::PanelWidget(SaParser *parser,int router,int output,QWidget *parent)
  : QWidget(parent)
{
  QFont label_font("helvetica",16,QFont::Bold);
  label_font.setPixelSize(16);
  QFont take_font("helvetica",20,QFont::Bold);
  take_font.setPixelSize(20);
  QFont cancel_font("helvetica",14,QFont::Bold);
  cancel_font.setPixelSize(14);

  widget_parser=parser;
  widget_router=router;
  widget_output=output;
  widget_input=1;
  widget_xpoint_synced=false;

  widget_output_label=new QLabel(this);
  widget_output_label->setFont(label_font);
  widget_output_label->setAlignment(Qt::AlignCenter);
  widget_output_label->setDisabled(true);

  widget_input_box=new ComboBox(this); 
  widget_input_box->setDisabled(true);
  connect(widget_input_box,SIGNAL(activated(int)),
	  this,SLOT(inputBoxActivatedData(int)));

  widget_take_button=new QPushButton(tr("Take"),this);
  widget_take_button->setFont(take_font);
  widget_take_button->setDisabled(true);
  connect(widget_take_button,SIGNAL(clicked()),
	  this,SLOT(takeButtonClickedData()));

  widget_cancel_button=new QPushButton(tr("Cancel"),this);
  widget_cancel_button->setFont(cancel_font);
  widget_cancel_button->setDisabled(true);
  connect(widget_cancel_button,SIGNAL(clicked()),
	  this,SLOT(cancelButtonClickedData()));
}


PanelWidget::~PanelWidget()
{
  delete widget_cancel_button;
  delete widget_take_button;
  delete widget_input_box;
  delete widget_output_label;
}


QSize PanelWidget::sizeHint() const
{
  return QSize(PANELWIDGET_WIDTH,PANELWIDGET_HEIGHT);
}


QSizePolicy PanelWidget::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


void PanelWidget::changeConnectionState(bool state,
					SaParser::ConnectionState cstate)
{
  if(state) {
    if(!widget_parser->routers().contains(widget_router+1)) {
      QMessageBox::warning(this,"OutputPanel - "+tr("Error"),
	  tr("Router")+QString::asprintf(" %u ",widget_router+1)+
	  tr("does not exist!"));
      exit(256);
    }
    if(!widget_parser->outputIsReal(widget_router,widget_output+1)) {
      QMessageBox::warning(this,"OutputPanel - "+tr("Error"),
	  tr("Output")+QString::asprintf(" %u ",widget_output+1)+
	  tr("does not exist!"));
      exit(256);
    }
  }
  else {
    widget_input_names.clear();
    widget_input_box->clear();
  }
  widget_output_label->setEnabled(widget_xpoint_synced);
  widget_input_box->setEnabled(widget_xpoint_synced);
}


void PanelWidget::updateInputNames()
{
  QList<PanelInput> inputs;

  for(int i=0;i<widget_parser->inputQuantity(widget_router);i++) {
    if(widget_parser->inputIsReal(widget_router,i+1)) {
      inputs.push_back(PanelInput(i,widget_parser->inputName(widget_router,i+1)));
    }
  }
  qSort(inputs);

  //
  // Populate the List Box
  //
  widget_input_names.clear();
  widget_input_box->clear();
  widget_input_names[0]=tr("--- OFF ---");
  widget_input_box->insertItem(0,tr("--- OFF ---"),-1);
  for(int i=0;i<inputs.size();i++) {
    widget_input_names[inputs[i].number()]=inputs[i].name();
    widget_input_box->insertItem(i,inputs[i].name(),inputs[i].number());
  }
}


void PanelWidget::updateOutputNames()
{
  QString name=widget_parser->outputName(widget_router,widget_output+1);
  if(name.isEmpty()) {
    name=tr("Output")+QString::asprintf(" %d",widget_output+1);
  }
  widget_output_label->setText(name);
}


void PanelWidget::tickClock(bool state)
{
  if(widget_input!=widget_input_box->currentItemData().toInt()) {
    if(state) {
      widget_take_button->
	setStyleSheet("color: #FFFFFF;background-color: #FF0000;");
      widget_cancel_button->setStyleSheet("");
    }
    else {
      widget_take_button->setStyleSheet("");
      widget_cancel_button->
	setStyleSheet("color: #FFFFFF;background-color: #0000FF;");
    }
  }
}


void PanelWidget::changeOutputCrosspoint(int router,int output,int input)
{
  //  printf("changeOutputCrosspoint(%d,%d,%d)\n",router,output,input);

  if(router==widget_router) {
    if((output-1)==widget_output) {
      if(!widget_input_box->setCurrentItemData(input-1)) {
	if(input<-1) {
	  widget_input_box->
	    insertItem(widget_input_box->count(),tr("** UNKNOWN **"),input-1);
	}
	else {
	  widget_input_box->insertItem(widget_input_box->count(),
				       "* "+widget_input_names[input-1],input);
	}
	widget_input_box->setCurrentItemData(input-1);
      }
      widget_input=input-1;
      widget_xpoint_synced=true;
      SetArmedState(false);
      if(input>0) {
	for(int i=0;i<widget_input_box->count();i++) {
	  if(widget_input_box->itemData(i).toInt()<-1) {
	    widget_input_box->removeItem(i);
	  }
	}
      }
    }
  }
}


void PanelWidget::inputBoxActivatedData(int index)
{
  int input=widget_input_box->currentItemData().toInt();
  SetArmedState(widget_input!=input);
}


void PanelWidget::takeButtonClickedData()
{
  widget_parser->
    setOutputCrosspoint(widget_router,widget_output+1,
			widget_input_box->currentItemData().toInt()+1);
}


void PanelWidget::cancelButtonClickedData()
{
  widget_input_box->setCurrentItemData(widget_input);
  SetArmedState(false);
}


void PanelWidget::resizeEvent(QResizeEvent *e)
{
  widget_output_label->setGeometry(0,0,size().width(),40);
  widget_input_box->setGeometry(0,40,size().width(),40);
  widget_take_button->setGeometry(0,85,size().width(),60);
  widget_cancel_button->setGeometry(0,150,size().width(),40);
}


void PanelWidget::SetArmedState(bool state)
{
  widget_output_label->setEnabled(widget_xpoint_synced);
  widget_input_box->setEnabled(widget_xpoint_synced);
  if(state) {
    widget_take_button->setEnabled(true);
    widget_cancel_button->setEnabled(true);
  }
  else {
    widget_take_button->setStyleSheet("");
    widget_cancel_button->setStyleSheet("");
    widget_take_button->setDisabled(true);
    widget_cancel_button->setDisabled(true);
  }
}
