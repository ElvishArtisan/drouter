// xpointpanel.h
//
// Full graphical crosspoint panel for SA devices.
//
//   (C) Copyright 2002-2022 Fred Gleason <fredg@paravelsystems.com>
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
#include "dparser.h"
#include "endpointlist.h"
#include "logindialog.h"
#include "saparser.h"
#include "sidelabel.h"
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
  void protocolDConnected(bool state);
  void errorData(QAbstractSocket::SocketError err);
  void outputCrosspointChangedData(int router,int output,int input);
  void xpointDoubleClickedData(int output_num,int input_num);
  void inputHoveredEndpointChangedData(int router,int input);
  void outputHoveredEndpointChangedData(int router,int output);
  void crosspointSelectedData(int slot_x,int slot_y);

 protected:
  void resizeEvent(QResizeEvent *e);
  void paintEvent(QPaintEvent *e);

 private:
  QString InputDescriptionTitle(int router,int input) const;
  QString OutputDescriptionTitle(int router,int output) const;
  LoginDialog *panel_login_dialog;
  QString panel_hostname;
  QString panel_username;
  QString panel_password;
  int panel_initial_router;
  QLabel *panel_router_label;
  QLabel *panel_inputs_label;
  SideLabel *panel_outputs_label;
  ComboBox *panel_router_box;
  QLabel *panel_description_name_label;
  QLabel *panel_description_text_label;
  DParser *panel_dparser;
  SaParser *panel_parser;
  bool panel_initial_connected;
  QGraphicsScene *panel_scene;
  XPointView *panel_view;
  EndpointList *panel_input_list;
  EndpointList *panel_output_list;
  QPixmap *panel_greenx_map;
};


#endif  // XPOINTPANEL_H
