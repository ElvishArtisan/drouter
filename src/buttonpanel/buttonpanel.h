// buttonpanel.h
//
// Button applet for controlling an SA output.
//
//   (C) Copyright 2002-2017 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef BUTTONPANEL_H
#define BUTTONPANEL_H


#include <QMap>
#include <QPixmap>
#include <QPushButton>
#include <QSignalMapper>
#include <QWidget>

#include "autopushbutton.h"
#include "logindialog.h"
#include "saparser.h"

#define BUTTONPANEL_USAGE "[options] --panel=<panel1>\n"
#define LWPANELBUTTON_ACTIVE_STYLESHEET "color: #FFFFFF; background-color: #0000FF;"

class MainWidget : public QWidget
{
  Q_OBJECT
 public:
  MainWidget(QWidget *parent=0);
  ~MainWidget();
  QSize sizeHint() const;

 private slots:
  void buttonClickedData(int n);
  void changeConnectionState(bool state,SaParser::ConnectionState cstate);
  void changeOutputCrosspoint(int router,int output,int input);

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  int panel_columns;
  int panel_rows;
  QString panel_hostname;
  QString panel_username;
  QString panel_password;
  int panel_router;
  int panel_output;
  QPixmap *panel_saspanels_map;
  SaParser *panel_parser;
  QSignalMapper *panel_button_mapper;
  QMap<int,AutoPushButton *> panel_buttons;
  LoginDialog *panel_login_dialog;
};


#endif  // BUTTONPANEL_H
