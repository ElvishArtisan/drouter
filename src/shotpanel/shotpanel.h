// shotpanel.h
//
// An applet for activating a snapshot
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef SHOTPANEL_H
#define SHOTPANEL_H

#include <vector>

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "combobox.h"
#include "logindialog.h"
#include "saparser.h"

#define SHOTPANEL_USAGE "[options]\n"

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
  void activateData();
  void connectedData(bool state,SaParser::ConnectionState cstate);
  void errorData(QAbstractSocket::SocketError err);

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  LoginDialog *panel_login_dialog;
  QString panel_hostname;
  QString panel_username;
  QString panel_password;
  QLabel *panel_router_label;
  ComboBox *panel_router_box;
  QLabel *panel_snapshot_label;
  QPushButton *panel_activate_button;
  ComboBox *panel_snapshot_box;
  SaParser *panel_parser;
  bool panel_initial_connected;
};


#endif  // SHOTPANEL_H
