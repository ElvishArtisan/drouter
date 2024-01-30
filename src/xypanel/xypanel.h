// xypanel.h
//
// X-Y controller applet for DRouter
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

#ifndef XYPANEL_H
#define XYPANEL_H

#include <vector>

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include <drcombobox.h>
#include <drlogindialog.h>
#include <drjparser.h>

#define XYPANEL_USAGE "[options]\n"

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
  void outputBoxActivatedData(int n);
  void inputBoxActivatedData(int n);
  void takeData();
  void cancelData();
  void connectedData(bool state,DRJParser::ConnectionState cstate);
  void errorData(QAbstractSocket::SocketError err);
  void outputCrosspointChangedData(int router,int output,int input);
  void clockData();

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  int SelectedRouter() const;
  int SelectedOutput() const;
  int SelectedInput() const;
  DRLoginDialog *panel_login_dialog;
  QString panel_hostname;
  QString panel_username;
  QString panel_password;
  void SetArmedState(bool state);
  QLabel *panel_router_label;
  DRComboBox *panel_router_box;
  int panel_current_input_index;
  QLabel *panel_output_label;
  DRComboBox *panel_input_box;
  QLabel *panel_input_label;
  QPushButton *panel_take_button;
  QPushButton *panel_cancel_button;
  DRComboBox *panel_output_box;
  DRJParser *panel_parser;
  QTimer *panel_clock_timer;
  bool panel_clock_state;
  bool panel_initial_connected;
  int panel_initial_router;
};


#endif  // LWPANEL_H
