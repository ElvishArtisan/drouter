// buttonwidget.cpp
//
// Button container for a single output.
//
//   (C) Copyright 2002-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QMessageBox>

#include "buttonwidget.h"

ButtonWidget::ButtonWidget(int router,int output,int columns,SaParser *parser,
			   bool arm_button,QWidget *parent)
  : QWidget(parent)
{
  panel_columns=columns;
  panel_output=output-1;
  panel_rows=1;
  panel_router=router;
  panel_parser=parser;
  panel_armed=false;

  //
  // Fonts
  //
  QFont title_font(font().family(),14,QFont::Bold);

  //
  // Title
  //
  panel_title_label=new QLabel(this);
  panel_title_label->setFont(title_font);
  panel_title_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

  //
  // Arm Button
  //
  if(arm_button) {
    panel_arm_button=new AutoPushButton(this);
    panel_arm_button->setText(tr("ARM"));
    connect(panel_arm_button,SIGNAL(clicked()),
	    this,SLOT(armButtonClickedData()));
  }
  else {
    panel_arm_button=NULL;
    panel_armed=true;
  }

  //
  // The SA Connection
  //
  connect(panel_parser,SIGNAL(connected(bool,SaParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,SaParser::ConnectionState)));
  connect(panel_parser,
	  SIGNAL(outputCrosspointChanged(int,int,int)),
	  this,SLOT(changeOutputCrosspoint(int,int,int)));
  panel_button_mapper=new QSignalMapper(this);
  connect(panel_button_mapper,SIGNAL(mapped(int)),
	  this,SLOT(buttonClickedData(int)));
}


ButtonWidget::~ButtonWidget()
{
}


QSize ButtonWidget::sizeHint() const
{
  int cols=panel_columns;
  if(panel_columns==0) {
    cols=panel_buttons.size();
    if(panel_arm_button!=NULL) {
      cols++;
    }
  }
  QSize sz(BUTTONWIDGET_CELL_WIDTH*cols-5,
	   22+BUTTONWIDGET_CELL_HEIGHT*panel_rows);
  return sz;
}


void ButtonWidget::buttonClickedData(int n)
{
  if(panel_armed) {
    panel_parser->setOutputCrosspoint(panel_router,panel_output+1,n+1);
    if(panel_arm_button!=NULL) {
      panel_arm_button->setStyleSheet("");
      panel_armed=false;
    }
  }
}


void ButtonWidget::armButtonClickedData()
{
  if(panel_armed) {
    panel_arm_button->setStyleSheet("");
    panel_armed=false;
  }
  else {
    panel_arm_button->
      setStyleSheet(QString("color: #FFFFFF;")+
		    "background-color: #FF0000;");
    panel_armed=true;
  }
}


void ButtonWidget::changeConnectionState(bool state,
				       SaParser::ConnectionState cstate)
{
  if(state) {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      delete it.value();
    }
    panel_buttons.clear();

    if(panel_parser->outputName(panel_router,panel_output+1).isEmpty()) {
      QMessageBox::
	warning(this,"ButtonPanel - "+tr("Error"),
		tr("Output")+
		QString::asprintf(" %d:%d ",panel_router,panel_output+1)+
		tr("does not exist!"));
      exit(1);
    }

    panel_title_label->
      setText(panel_parser->outputName(panel_router,panel_output+1));
    for(int i=0;i<panel_parser->inputQuantity(panel_router);i++) {
      if(panel_parser->inputIsReal(panel_router,i+1)) {
	panel_buttons[i]=new AutoPushButton(this);
	panel_buttons.value(i)->
	  setText(panel_parser->inputName(panel_router,i+1));
	panel_buttons.value(i)->show();
	connect(panel_buttons.value(i),SIGNAL(clicked()),
		panel_button_mapper,SLOT(map()));
	panel_button_mapper->setMapping(panel_buttons.value(i),i);
	if(panel_parser->outputCrosspoint(panel_router,panel_output+1)==(i+1)) {
	  panel_buttons.value(i)->setStyleSheet(BUTTONWIDGET_ACTIVE_STYLESHEET);
	}
      }
    }
    show();
  }
  else {
    hide();
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      delete it.value();
    }
    panel_buttons.clear();
    panel_rows=1;
  }

  int col=0;
  for(int i=0;i<panel_buttons.size();i++) {
    if((panel_columns>0)&&(col==panel_columns)) {
      panel_rows++;
      col=0;
    }
    col++;
  }
  if(panel_columns>1) {
    if((panel_arm_button!=NULL)&&((panel_buttons.size()%panel_columns)==0)) {
      panel_rows++;
    }
  }

  resize(sizeHint());
}


void ButtonWidget::changeOutputCrosspoint(int router,int output,int input)
{
  if((router==panel_router)&&((output-1)==panel_output)) {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      if((input-1)==it.key()) {
	it.value()->setStyleSheet(BUTTONWIDGET_ACTIVE_STYLESHEET);
      }
      else {
	it.value()->setStyleSheet("");
      }
    }
  }
}


void ButtonWidget::resizeEvent(QResizeEvent *e)
{
  int col=0;
  int row=0;

  panel_title_label->setGeometry(10,0,size().width(),20);
  
  if(panel_buttons.size()==0) {
    return;
  }
  if(panel_arm_button!=NULL) {
    panel_arm_button->setGeometry(0,22,BUTTONWIDGET_CELL_WIDTH-5,BUTTONWIDGET_CELL_HEIGHT-5);
    col++;
  }
  if(panel_columns==0) {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      it.value()->setGeometry(BUTTONWIDGET_CELL_WIDTH*col,
			      22,
			      BUTTONWIDGET_CELL_WIDTH-5,
			      BUTTONWIDGET_CELL_HEIGHT-5);
      col++;
    }
  }
  else {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      it.value()->setGeometry(BUTTONWIDGET_CELL_WIDTH*col,
			      22+BUTTONWIDGET_CELL_HEIGHT*row,
			      BUTTONWIDGET_CELL_WIDTH-5,
			      BUTTONWIDGET_CELL_HEIGHT-5);
      col++;
      if((panel_columns>0)&&(col>=panel_columns)) {
	col=0;
	row++;
      }
    }
  }
}
