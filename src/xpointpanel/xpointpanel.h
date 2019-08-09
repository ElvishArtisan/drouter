// xpointpanel.h
//
// Full graphical crosspoint panel for SA devices.
//
//   (C) Copyright 2002-2016 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef XPOINTPANEL_H
#define XPOINTPANEL_H

#include <QGraphicsScene>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "combobox.h"
#include "endpointlist.h"
#include "logindialog.h"
#include "saparser.h"
#include "xpointview.h"

#define XPOINTPANEL_USAGE "[options]\n"

class MainWidget : public QWidget
{
  Q_OBJECT
 public:
  MainWidget(QWidget *parent=0);
  ~MainWidget();
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;

 private slots:
  void routerBoxActivatedData(int n);
  void connectedData(bool state,SaParser::ConnectionState cstate);
  void errorData(QAbstractSocket::SocketError err);
  void outputCrosspointChangedData(int router,int output,int input);
  void xpointDoubleClickedData(int output_num,int input_num);

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  LoginDialog *panel_login_dialog;
  QString panel_hostname;
  QString panel_username;
  QString panel_password;
  int panel_initial_router;
  QLabel *panel_router_label;
  ComboBox *panel_router_box;
  SaParser *panel_parser;
  bool panel_initial_connected;
  QGraphicsScene *panel_scene;
  XPointView *panel_view;
  EndpointList *panel_input_list;
  EndpointList *panel_output_list;
  QPixmap *panel_greenx_map;
};


#endif  // XPOINTPANEL_H
