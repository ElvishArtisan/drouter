// buttonpanel.h
//
// Button applet for controlling an SA output.
//
//   (C) Copyright 2002-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <drouter/drendpointmap.h>
#include <drouter/drjparser.h>
#include <drouter/drlogindialog.h>

#include "autopushbutton.h"
#include "buttonwidget.h"
#include "gpioparser.h"

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
  void parserErrorData(DRJParser::ErrorType err,const QString &remarks);
  void changeConnectionState(bool state,DRJParser::ConnectionState cstate);
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
  bool panel_no_max_size;
  QPixmap *panel_saspanels_map;
  DRJParser *panel_parser;
  QSignalMapper *panel_button_mapper;
  QLabel *panel_connecting_label;
  DRLoginDialog *panel_login_dialog;
  QTimer *panel_resize_timer;
  QList<QWidget *> panel_widgets;
  QList<DREndPointMap::RouterType> panel_arg_types;
  QList<int> panel_arg_audio_routers;
  QList<int> panel_arg_audio_outputs;
  QList<GpioParser *> panel_gpio_parsers;
};


#endif  // BUTTONPANEL_H
