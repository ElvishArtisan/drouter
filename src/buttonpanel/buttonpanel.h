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
#include "endpointmap.h"
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
  void processError(const QString err_msg);
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
  QPixmap *panel_saspanels_map;
  SaParser *panel_parser;
  QSignalMapper *panel_button_mapper;
  QLabel *panel_connecting_label;
  LoginDialog *panel_login_dialog;
  QTimer *panel_resize_timer;
  QList<QWidget *> panel_widgets;

  QList<EndPointMap::RouterType> panel_arg_types;
  QList<int> panel_arg_audio_routers;
  QList<int> panel_arg_audio_outputs;

  QStringList panel_arg_titles;
  QList<QStringList> panel_arg_gpio_types;
  QList<QStringList> panel_arg_gpio_colors;
  QList<QList <QChar> > panel_arg_gpio_dirs;
  QList<QList<int> > panel_arg_gpio_routers;
  QList<QList<int> > panel_arg_gpio_endpts;
  QList<QStringList> panel_arg_gpio_legends;
  QList<QStringList> panel_arg_gpio_masks;
};


#endif  // BUTTONPANEL_H
