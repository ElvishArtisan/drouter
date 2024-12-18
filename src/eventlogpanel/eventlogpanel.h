// eventlogpanel.h
//
// Applet for reading the event log
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

#ifndef EVENTLOGPANEL_H
#define EVENTLOGPANEL_H

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QTimer>
#include <QWidget>

#include "eventlogmodel.h"
#include "instanceindicator.h"

#define EVENTLOGPANEL_USAGE "[options]\n"

class MainWidget : public QWidget
{
  Q_OBJECT
 public:
  MainWidget(QWidget *parent=0);
  ~MainWidget();
  QSize sizeHint() const;

 private slots:
  void showAttributesData(int n);
  void toggleScrollingData();
  void refreshData();
  void dbKeepaliveData();

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  QLabel *d_show_attributes_label;
  QComboBox *d_show_attributes_box;
  QPushButton *d_scroll_button;
  InstanceIndicator *d_instance_indicator;
  QTableView *d_table_view;
  EventLogModel *d_log_model;
  bool d_scrolling;
  QTimer *d_refresh_timer;
  int d_db_keepalive_interval;
  QTimer *d_db_keepalive_timer;
};


#endif  // EVENTLOGPANEL_H
