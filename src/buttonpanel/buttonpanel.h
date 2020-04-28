// buttonpanel.h
//
// Button applet for controlling an SA output.
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

#ifndef BUTTONPANEL_H
#define BUTTONPANEL_H

#include <QLabel>
#include <QList>
#include <QPixmap>
#include <QPushButton>
#include <QSignalMapper>
#include <QTimer>
#include <QWidget>

#include "autopushbutton.h"
#include "buttonwidget.h"
#include "logindialog.h"
#include "saparser.h"

#define BUTTONPANEL_USAGE "[options]\n"
#define LWPANELBUTTON_ACTIVE_STYLESHEET "color: #FFFFFF; background-color: #0000FF;"

class MainWidget : public QWidget
{
  Q_OBJECT
 public:
  MainWidget(QWidget *parent=0);
  ~MainWidget();
  QSize sizeHint() const;

 private slots:
  void changeConnectionState(bool state,SaParser::ConnectionState cstate);
  void resizeData();

 protected:
  void resizeEvent(QResizeEvent *e);
  void paintEvent(QPaintEvent *e);

 private:
  int panel_columns;
  QString panel_hostname;
  QString panel_username;
  QString panel_password;
  bool panel_arm_button;
  QList<int> panel_routers;
  QList<int> panel_outputs;
  QPixmap *panel_saspanels_map;
  SaParser *panel_parser;
  QSignalMapper *panel_button_mapper;
  QLabel *panel_connecting_label;
  QList<ButtonWidget *> panel_panels;
  LoginDialog *panel_login_dialog;
  QTimer *panel_resize_timer;
  int panel_width;
  int panel_height;
};


#endif  // BUTTONPANEL_H
