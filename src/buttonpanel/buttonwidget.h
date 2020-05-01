// buttonwidget.h
//
// Button container for a single output.
//
//   (C) Copyright 2002-2020 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef BUTTONWIDGET_H
#define BUTTONWIDGET_H

#include <QLabel>
#include <QMap>
#include <QPixmap>
#include <QPushButton>
#include <QSignalMapper>
#include <QWidget>

#include "autopushbutton.h"
#include "logindialog.h"
#include "saparser.h"

#define BUTTONWIDGET_ACTIVE_STYLESHEET "color: #FFFFFF; background-color: #0000FF;"
#define BUTTONWIDGET_CELL_WIDTH 90
#define BUTTONWIDGET_CELL_HEIGHT 60

class ButtonWidget : public QWidget
{
  Q_OBJECT
 public:
  ButtonWidget(int router,int output,int columns,SaParser *parser,
	       bool arm_button,QWidget *parent=0);
  ~ButtonWidget();
  QSize sizeHint() const;

 private slots:
  void buttonClickedData(int n);
  void armButtonClickedData();
  void changeConnectionState(bool state,SaParser::ConnectionState cstate);
  void changeOutputCrosspoint(int router,int output,int input);

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  int panel_columns;
  int panel_rows;
  int panel_router;
  int panel_output;
  SaParser *panel_parser;
  QSignalMapper *panel_button_mapper;
  QMap<int,AutoPushButton *> panel_buttons;
  AutoPushButton *panel_arm_button;
  bool panel_armed;
  QLabel *panel_title_label;
};


#endif  // BUTTONWIDGET_H
