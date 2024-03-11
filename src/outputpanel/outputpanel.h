// outputpanel.h
//
// Applet for controling a ProtocolJ output
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

#ifndef OUTPUTPANEL_H
#define OUTPUTPANEL_H

#include <QList>
#include <QPixmap>
#include <QWidget>

#include <drouter/drjparser.h>
#include <drouter/drlogindialog.h>

#include "panelwidget.h"

#define OUTPUTPANEL_USAGE "[options]\n"

class MainWidget : public QWidget
{
  Q_OBJECT
 public:
  MainWidget(QWidget *parent=0);
  ~MainWidget();
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;

 signals:
  void clockTicked(bool state);

 private slots:
  void tickClock();

 protected:
  void paintEvent(QPaintEvent *e);

 private:
  QString panel_hostname;
  QString panel_username;
  QString panel_password;
  unsigned panel_quantity;
  unsigned panel_columns;
  unsigned panel_rows;
  bool clock_state;
  QPixmap *panel_saspanels_map;
  DRJParser *panel_parser;
  DRLoginDialog *panel_login_dialog;
  QList<PanelWidget *> panel_widgets;
};


#endif  // OUTPUTPANEL_H
