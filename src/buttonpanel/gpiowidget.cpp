// gpiowidget.cpp
//
// Strip container for GPIO controls.
//
//   (C) Copyright 2020 Fred Gleason <fredg@paravelsystems.com>
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

#include "gpiowidget.h"
#include "statebutton.h"
#include "statelight.h"

GpioWidget::GpioWidget(const QStringList &types,const QList<QChar> &dirs,
		       const QList<int> &routers,const QList<int> &endpts,
		       const QStringList &legends,const QStringList &masks,
		       SaParser *parser,QWidget *parent)
  : QWidget(parent)
{
  c_parser=parser;
  c_hint_width=0;
  c_hint_height=0;

  //
  // Fonts
  //
  QFont title_font(font().family(),14,QFont::Bold);

  //
  // Title
  //
  c_title_label=new QLabel(this);
  c_title_label->setFont(title_font);
  c_title_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  c_title_label->hide();

  //
  // Create Widgets
  //
  for(int i=0;i<types.size();i++) {
    bool matched=false;
    if(types.at(i).right(5)=="light") {
      StateLight *w=NULL;
      w=new StateLight(routers.at(i),endpts.at(i),legends.at(i),masks.at(i),
		       dirs.at(i),c_parser,this);
      c_widgets.push_back(w);
      QString colorstr=types.at(i).left(types.at(i).length()-5);
      if(colorstr=="blue") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#0000FF");
	matched=true;
      }
      if(colorstr=="cyan") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#008888");
	matched=true;
      }
      if(colorstr=="green") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#008800");
	matched=true;
      }
      if(colorstr=="magenta") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#880088");
	matched=true;
      }
      if(colorstr=="red") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#CC0000");
	matched=true;
      }
      if(colorstr=="yellow") {
	w->setTextColor("#000000");
	w->setBackgroundColor("#FFFF00");
	matched=true;
      }
    }

    if(types.at(i).right(6)=="button") {
      StateButton *w=NULL;
      w=new StateButton(routers.at(i),endpts.at(i),legends.at(i),masks.at(i),
			dirs.at(i),c_parser,this);
      c_widgets.push_back(w);
      QString colorstr=types.at(i).left(types.at(i).length()-6);

      if(colorstr.isEmpty()||(colorstr=="black")) {
	w->setTextColor("#000000");
	matched=true;
      }
      if(colorstr=="blue") {
	w->setTextColor("#0000FF");
	matched=true;
      }
      if(colorstr=="cyan") {
	w->setTextColor("#008888");
	matched=true;
      }
      if(colorstr=="green") {
	w->setTextColor("#008800");
	matched=true;
      }
      if(colorstr=="magenta") {
	w->setTextColor("#880088");
	matched=true;
      }
      if(colorstr=="red") {
	w->setTextColor("#CC0000");
	matched=true;
      }
      if(colorstr=="yellow") {
	w->setTextColor("#FFFF00");
	matched=true;
      }
    }

    if(matched) {
      c_hint_width+=5+c_widgets.back()->sizeHint().width();
      if(c_widgets.back()->sizeHint().height()>c_hint_height) {
	c_hint_height=c_widgets.back()->sizeHint().height();
      }
    }
    else {
      processError(tr("Invalid --gpio argument type")+" \""+types.at(i)+"\".");
    }
    c_widgets.back()->hide();
  }
  c_hint_width-=5;    // Remove unused space after last widget

  //
  // The SA Connection
  //
  connect(c_parser,SIGNAL(connected(bool,SaParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,SaParser::ConnectionState)));

  show();
}


GpioWidget::~GpioWidget()
{
  for(int i=0;i<c_widgets.size();i++) {
    delete c_widgets.at(i);
  }
  c_widgets.clear();
  delete c_title_label;
}


QSize GpioWidget::sizeHint() const
{
  int height=c_hint_height;
  if(!c_title_label->text().isEmpty()) {
    height+=22;  // Title Height
  }

  return QSize(c_hint_width,height);
}


QString GpioWidget::title() const
{
  return c_title_label->text();
}


void GpioWidget::setTitle(const QString &str)
{
  c_title_label->setText(str);
}


void GpioWidget::processError(const QString &err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);;
  exit(1);
}


void GpioWidget::resizeEvent(QResizeEvent *e)
{
  int label_height=22;

  if(c_title_label->text().isEmpty()) {
    label_height=0;
  }
  c_title_label->setGeometry(10,0,size().width(),20);

  int xpos=0;
  for(int i=0;i<c_widgets.size();i++) {
    QWidget *w=c_widgets.at(i);
    w->setGeometry(xpos,label_height,
		   w->sizeHint().width(),w->sizeHint().height());
    xpos+=w->sizeHint().width()+5;
  }
}


void GpioWidget::changeConnectionState(bool state,
					SaParser::ConnectionState cstate)
{
  c_title_label->setVisible(state);
  for(int i=0;i<c_widgets.size();i++) {
    c_widgets.at(i)->setVisible(state);
  }
}
