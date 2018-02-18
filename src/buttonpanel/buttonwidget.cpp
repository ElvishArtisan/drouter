// buttonwidget.cpp
//
// Button container for a single output.
//
//   (C) Copyright 2002-2018 Fred Gleason <fredg@paravelsystems.com>
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
			   QWidget *parent)
  : QWidget(parent)
{
  panel_columns=columns;
  panel_output=output-1;
  panel_rows=1;
  panel_router=router;
  panel_parser=parser;

  //
  // Fonts
  //
  QFont title_font("helvetica",14,QFont::Bold);

  //
  // Title
  //
  panel_title_label=new QLabel(this);
  panel_title_label->setFont(title_font);
  panel_title_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

  //
  // The SA Connection
  //
  //  panel_parser=new SaParser(this);
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
  }
  return QSize(BUTTONWIDGET_CELL_WIDTH*cols,
	       20+BUTTONWIDGET_CELL_HEIGHT*panel_rows);
}


void ButtonWidget::buttonClickedData(int n)
{
  //  printf("setOutputCrosspoint(%d,%d,%d)\n",panel_router,panel_output+1,n+1);
  panel_parser->setOutputCrosspoint(panel_router,panel_output+1,n+1);
}


void ButtonWidget::changeConnectionState(bool state,
				       SaParser::ConnectionState cstate)
{
  //  printf("changeConnectionState(%d)\n",state);
  if(state) {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      delete it.value();
    }
    panel_buttons.clear();

    if(panel_parser->outputName(panel_router,panel_output+1).isEmpty()) {
      QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			   tr("No such output!"));
      exit(1);
    }
    panel_title_label->setText(panel_parser->outputName(panel_router,panel_output+1)+" "+
			       tr("Switcher"));
    for(int i=0;i<panel_parser->inputQuantity(panel_router);i++) {
      if(!panel_parser->inputName(panel_router,i+1).isEmpty()) {
	panel_buttons[i]=new AutoPushButton(this);
	panel_buttons[i]->setText(panel_parser->inputName(panel_router,i+1));
	panel_buttons[i]->show();
	connect(panel_buttons[i],SIGNAL(clicked()),
		panel_button_mapper,SLOT(map()));
	panel_button_mapper->setMapping(panel_buttons[i],i);
	if(panel_parser->outputCrosspoint(panel_router,panel_output+1)==(i+1)) {
	  panel_buttons[i]->setStyleSheet(BUTTONWIDGET_ACTIVE_STYLESHEET);
	}
      }
    }
  }
  else {
    for(int i=0;i<panel_buttons.size();i++) {
      panel_buttons[i]->setDisabled(true);
    }
  }

  panel_rows=1;
  int col=0;
  for(int i=0;i<panel_buttons.size();i++) {
    if((panel_columns>0)&&(col==panel_columns)) {
      panel_rows++;
      col=0;
    }
    col++;
  }

  resize(sizeHint());
}


void ButtonWidget::changeOutputCrosspoint(int router,int output,int input)
{
  //  printf("changeOutputCrosspoint(%d,%d,%d)\n",router,output,input);
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
  if(panel_columns==0) {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      it.value()->setGeometry(0+BUTTONWIDGET_CELL_WIDTH*col,0+22,BUTTONWIDGET_CELL_WIDTH-10,BUTTONWIDGET_CELL_HEIGHT-10);
      col++;
    }
  }
  else {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      it.value()->setGeometry(0+BUTTONWIDGET_CELL_WIDTH*col,0+22+BUTTONWIDGET_CELL_HEIGHT*(row),
			      BUTTONWIDGET_CELL_WIDTH-10,BUTTONWIDGET_CELL_HEIGHT-10);
      col++;
      if((panel_columns>0)&&(col==panel_columns)) {
	col=0;
	row++;
      }
    }
  }
}
