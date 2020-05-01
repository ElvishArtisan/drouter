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

GpioWidget::GpioWidget(const QStringList &types,const QList<QChar> &dirs,
			  const QList<int> &routers,const QList<int> &endpts,
			  const QStringList &masks,SaParser *parser,
			  QWidget *parent)
  : QWidget(parent)
{
  c_parser=parser;
  c_hint_width=0;
  c_hint_height=0;

  //
  // Fonts
  //
  QFont title_font("helvetica",14,QFont::Bold);

  //
  // Title
  //
  c_title_label=new QLabel(this);
  c_title_label->setFont(title_font);
  c_title_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

  //
  // Create Widgets
  //
  for(int i=0;i<types.size();i++) {
    bool matched=false;
    if(types.at(i)=="statelight") {
      StateLight *w=NULL;
      w=new StateLight(routers.at(i),endpts.at(i),masks.at(i),dirs.at(i),
		       c_parser,this);
      c_widgets.push_back(w);
      matched=true;
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
  }
  c_hint_width-=5;    // Remove unused space after last widget
  c_hint_height+=22;  // Title Height

  //
  // The SA Connection
  //
  connect(c_parser,SIGNAL(connected(bool,SaParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,SaParser::ConnectionState)));

  show();
}


GpioWidget::~GpioWidget()
{
  //  delete c_title_label;
}


QSize GpioWidget::sizeHint() const
{
  return QSize(c_hint_width,c_hint_height);
}


void GpioWidget::processError(const QString &err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);;
  exit(1);
}


void GpioWidget::resizeEvent(QResizeEvent *e)
{
  c_title_label->setGeometry(10,0,size().width(),20);

  int xpos=0;
  for(int i=0;i<c_widgets.size();i++) {
    QWidget *w=c_widgets.at(i);
    w->setGeometry(xpos,22,w->sizeHint().width(),w->sizeHint().height());
    xpos+=w->sizeHint().width()+5;
  }
}


void GpioWidget::changeConnectionState(bool state,
					SaParser::ConnectionState cstate)
{
  if(state) {
    c_title_label->setText("Test Strip");
  }
  else {
  }
}
