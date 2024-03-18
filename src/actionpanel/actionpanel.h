// actionpanel.h
//
// Action Viewer for Drouter
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

#ifndef ACTIONPANEL_H
#define ACTIONPANEL_H

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QTimer>
#include <QWidget>

#include <drouter/drjparser.h>

#include "editactiondialog.h"
#include "wallclock.h"

#define ACTIONPANEL_USAGE "[options]\n"

class MainWidget : public QWidget
{
  Q_OBJECT
 public:
  MainWidget(QWidget *parent=0);
  ~MainWidget();
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;

 private slots:
  void addData();
  void editData();
  void doubleClickedData(const QModelIndex &index);
  void deleteData();
  void closeData();
  void routerBoxActivatedData(int n);
  void connectedData(bool state,DRJParser::ConnectionState cstate);
  void errorData(QAbstractSocket::SocketError err);
  void parserErrorData(DRJParser::ErrorType type,const QString &remarks);

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private:
  int SelectedRow() const;
  QLabel *d_router_label;
  QComboBox *d_router_box;
  QPushButton *d_add_button;
  QPushButton *d_edit_button;
  QPushButton *d_delete_button;
  QPushButton *d_close_button;
  WallClock *d_wall_clock;
  EditActionDialog *d_edit_dialog;
  DRJParser *d_parser;
  QTableView *d_action_view;
  QString d_hostname;
  bool d_initial_connected;
  int d_initial_router;
};


#endif  // ACTIONPANEL_H
