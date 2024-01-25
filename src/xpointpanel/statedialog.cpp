// statedialog.cpp
//
// Set state on a GPIO endpoint
//
//   (C) Copyright 2020-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <QFontMetrics>

#include <sy5/syconfig.h>

#include <drendpointmap.h>

#include "statedialog.h"

#define STATEDIALOG_CONTROL_WIDTH 185

StateDialog::StateDialog(int router,int endpt,DREndPointMap::Type gpio_type,
			 DRSaParser *parser,QWidget *parent)
  : QDialog(parent,Qt::Tool)
{
  d_router=router;
  d_endpoint=endpt+1;
  d_type=gpio_type;
  d_parser=parser;

  QFont bold_font(font().family(),font().pointSize(),QFont::Bold);

  d_name_label=new QLabel(this);
  d_name_label->setFont(bold_font);
  d_name_label->setAlignment(Qt::AlignCenter);

  d_state_edit=new QLineEdit(this);
  d_state_edit->setMaxLength(SWITCHYARD_GPIO_BUNDLE_SIZE);
  connect(d_state_edit,SIGNAL(textChanged(const QString &)),
	  this,SLOT(stateTextChangedData(const QString &)));
  connect(d_state_edit,SIGNAL(returnPressed()),
	  this,SLOT(stateReturnPressedData()));

  d_set_button=new QPushButton(tr("Set"),this);
  d_set_button->setFont(bold_font);
  connect(d_set_button,SIGNAL(clicked()),this,SLOT(setData()));

  d_reset_button=new QPushButton(tr("Reset"),this);
  d_reset_button->setFont(bold_font);
  connect(d_reset_button,SIGNAL(clicked()),this,SLOT(resetData()));

  switch(d_type) {
  case DREndPointMap::Input:
    connect(d_parser,SIGNAL(gpiStateChanged(int,int,const QString &)),
	    this,SLOT(gpioStateChangedData(int,int,const QString &)));
    d_name_label->setText(QString::asprintf("%d - ",endpt+1)+
			  d_parser->inputLongName(d_router,d_endpoint));
    setWindowTitle(tr("GPI State"));
    break;

  case DREndPointMap::Output:
    connect(d_parser,SIGNAL(gpoStateChanged(int,int,const QString &)),
	    this,SLOT(gpioStateChangedData(int,int,const QString &)));
    d_name_label->setText(QString::asprintf("%d - ",endpt+1)+
			  d_parser->outputLongName(d_router,d_endpoint));
    setWindowTitle(tr("GPO State"));
    break;

  case DREndPointMap::LastType:
    break;
  }
  resetData();

  QFontMetrics *fm=new QFontMetrics(d_name_label->font());
  if(fm->width(d_name_label->text())>STATEDIALOG_CONTROL_WIDTH) {
    d_width=fm->width(d_name_label->text())+20;
  }
  else {
    d_width=STATEDIALOG_CONTROL_WIDTH;
  }
  delete fm;
}


QSize StateDialog::sizeHint() const
{
  return QSize(d_width,50);
}


void StateDialog::closeEvent(QCloseEvent *e)
{
  hide();
}


void StateDialog::resizeEvent(QResizeEvent *e)
{
  int w=size().width();
  int x_margin=5+(w-STATEDIALOG_CONTROL_WIDTH)/2;

  d_name_label->setGeometry(0,2,w,20);

  //
  // Maintainer's Note:
  //   Be sure to update the value of the STATEDIALOG_CONTROL_WIDTH define
  //   appropriately when changing the layout of any of the following
  //   elements.
  //
  d_state_edit->setGeometry(x_margin,24,65,20);
  d_set_button->setGeometry(70+x_margin,22,50,24);
  d_reset_button->setGeometry(125+x_margin,22,50,24);
}


void StateDialog::stateTextChangedData(const QString &str)
{
  if(str.length()!=SWITCHYARD_GPIO_BUNDLE_SIZE) {
    d_set_button->setDisabled(true);
    d_reset_button->setDisabled(true);
    return;
  }
  for(int i=0;i<str.length();i++) {
    if((str.toLower().at(i)!=QChar('h'))&&
       (str.toLower().at(i)!=QChar('l'))&&
       (str.toLower().at(i)!=QChar('x'))) {
      d_set_button->setDisabled(true);
      d_reset_button->setDisabled(true);
      return;
    }
  }
  switch(d_type) {
  case DREndPointMap::Input:
    d_set_button->setDisabled(str==d_parser->gpiState(d_router,d_endpoint));
    d_reset_button->setDisabled(str==d_parser->gpiState(d_router,d_endpoint));
    break;

  case DREndPointMap::Output:
    d_set_button->setDisabled(str==d_parser->gpoState(d_router,d_endpoint));
    d_reset_button->setDisabled(str==d_parser->gpoState(d_router,d_endpoint));
    break;

  case DREndPointMap::LastType:
    break;
  }
}


void StateDialog::stateReturnPressedData()
{
  if(d_set_button->isEnabled()) {
    setData();
  }
}


void StateDialog::gpioStateChangedData(int router,int endpt,const QString &code)
{
  if((router==d_router)&&(endpt==d_endpoint)) {
    d_state_edit->setText(code);
    resetData();
  }
}


void StateDialog::setData()
{
  switch(d_type) {
  case DREndPointMap::Input:
    d_parser->setGpiState(d_router,d_endpoint,d_state_edit->text());
    break;

  case DREndPointMap::Output:
    d_parser->setGpoState(d_router,d_endpoint,d_state_edit->text());
    break;

  case DREndPointMap::LastType:
    break;
  }
}


void StateDialog::resetData()
{
  switch(d_type) {
  case DREndPointMap::Input:
    d_state_edit->setText(d_parser->gpiState(d_router,d_endpoint));
    break;

  case DREndPointMap::Output:
    d_state_edit->setText(d_parser->gpoState(d_router,d_endpoint));
    break;

  case DREndPointMap::LastType:
    break;
  }
  d_set_button->setDisabled(true);
  d_reset_button->setDisabled(true);
}
