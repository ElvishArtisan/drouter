// panelwidget.cpp
//
// Output panel widget for OutputPanel
//
//   (C) Copyright 2016-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <drouter/drendpointlistmodel.h>

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




PanelWidget::PanelWidget(DRJParser *parser,int router,int output,QWidget *parent)
  : QWidget(parent)
{
  QFont label_font("helvetica",16,QFont::Bold);
  label_font.setPixelSize(16);
  QFont take_font("helvetica",20,QFont::Bold);
  take_font.setPixelSize(20);
  QFont cancel_font("helvetica",14,QFont::Bold);
  cancel_font.setPixelSize(14);

  widget_blue_palette=palette();
  widget_blue_palette.setColor(QPalette::Window,Qt::blue);
  widget_blue_palette.setColor(QPalette::WindowText,Qt::white);
  widget_blue_stylesheet=QString("color:white;background-color:blue;");

  widget_red_palette=palette();
  widget_red_palette.setColor(QPalette::Window,Qt::red);
  widget_red_palette.setColor(QPalette::WindowText,Qt::white);
  widget_red_stylesheet=QString("color:white;background-color:red;");
  
  widget_parser=parser;
  widget_router=router;
  widget_output=output;
  widget_input=1;
  widget_xpoint_synced=false;

  widget_output_label=new QLabel(this);
  widget_output_label->setFont(label_font);
  widget_output_label->setAlignment(Qt::AlignCenter);
  widget_output_label->setDisabled(true);

  widget_input_box=new QComboBox(this); 
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
					DRJParser::ConnectionState cstate)
{
  if(state) {
    DREndPointListModel *omodel=widget_parser->outputModel(widget_router);
    DREndPointListModel *imodel=widget_parser->inputModel(widget_router);
    if(omodel==NULL) {
      QMessageBox::warning(this,"OutputPanel - "+tr("Error"),
	  tr("Router")+QString::asprintf(" %u ",widget_router)+
	  tr("does not exist!"));
      exit(256);
    }
    if(omodel->rowMetadata(omodel->rowNumber(widget_output)).contains("name")) {
      widget_output_label->setText(omodel->rowMetadata(omodel->
		       rowNumber(widget_output)).value("name").toString());
      widget_output_label->setEnabled(widget_xpoint_synced);
    }
    else {
      QMessageBox::warning(this,"OutputPanel - "+tr("Error"),
			   tr("Output")+QString::asprintf(" %u:%u ",
			   widget_router,widget_output)+
	  tr("does not exist!"));
      exit(256);
    }
    widget_input_box->setModel(imodel);
    widget_input_box->setEnabled(widget_xpoint_synced);

    changeOutputCrosspoint(widget_router,widget_output,
	   widget_parser->outputCrosspoint(widget_router,widget_output));
  }
  else {
    widget_input_box->setModel(NULL);
  }
}


void PanelWidget::tickClock(bool state)
{
  if(widget_input!=SelectedInput()) {
    if(state) {
      widget_take_button->
	setStyleSheet(widget_red_stylesheet);
      widget_take_button->setPalette(widget_red_palette);
      widget_cancel_button->setStyleSheet("");
      widget_cancel_button->setPalette(palette());
    }
    else {
      widget_take_button->setStyleSheet("");
      widget_take_button->setPalette(palette());
      widget_cancel_button->
	setStyleSheet(widget_blue_stylesheet);
      widget_cancel_button->setPalette(widget_blue_palette);
    }
  }
}


void PanelWidget::changeOutputCrosspoint(int router,int output,int input)
{
  if(router==widget_router) {
    if(output==widget_output) {

      widget_input_box->setCurrentIndex(widget_parser->
			inputModel(widget_router)->rowNumber(input));
      widget_input=input;
      widget_xpoint_synced=true;
      SetArmedState(false);
    }
  }
}


void PanelWidget::inputBoxActivatedData(int index)
{
  int input=SelectedInput();
  SetArmedState(widget_input!=input);
}


void PanelWidget::takeButtonClickedData()
{
  widget_parser->
    setOutputCrosspoint(widget_router,widget_output,SelectedInput());
}


void PanelWidget::cancelButtonClickedData()
{
  widget_input_box->setCurrentIndex(widget_parser->
		       inputModel(widget_router)->rowNumber(widget_input));
  SetArmedState(false);
}


void PanelWidget::resizeEvent(QResizeEvent *e)
{
  widget_output_label->setGeometry(0,0,size().width(),40);
  widget_input_box->setGeometry(0,40,size().width(),40);
  widget_take_button->setGeometry(0,85,size().width(),60);
  widget_cancel_button->setGeometry(0,150,size().width(),40);
}


int PanelWidget::SelectedInput() const
{
  return widget_parser->inputModel(widget_router)->
    endPointNumber(widget_input_box->currentIndex());
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
    widget_take_button->setPalette(palette());
    widget_cancel_button->setStyleSheet("");
    widget_cancel_button->setPalette(palette());
    widget_take_button->setDisabled(true);
    widget_cancel_button->setDisabled(true);
  }
}
