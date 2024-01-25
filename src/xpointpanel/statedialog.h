// statedialog.h
//
// Set state on a GPIO endpoint
//
//   (C) Copyright 2020-2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef STATEDIALOG_H
#define STATEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <drendpointmap.h>
#include <drsaparser.h>

class StateDialog : public QDialog
{
 Q_OBJECT
 public:
 StateDialog(int router,int endpoint,DREndPointMap::Type type,DRSaParser *parser,
	     QWidget *parent=0);
  QSize sizeHint() const;

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private slots:
  void stateTextChangedData(const QString &str);
  void stateReturnPressedData();
  void gpioStateChangedData(int router,int endpt,const QString &code);
  void setData();
  void resetData();

 private:
  QLabel *d_name_label;
  QLineEdit *d_state_edit;
  QPushButton *d_set_button;
  QPushButton *d_reset_button;
  int d_router;
  int d_endpoint;
  DREndPointMap::Type d_type;
  DRSaParser *d_parser;
  int d_width;
};


#endif  // STATEDIALOG_H
