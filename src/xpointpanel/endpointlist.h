// endpointlist.h
//
// Input/Output labels for xpointpanel(1)
//
//   (C) Copyright 2017-2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef ENDPOINTLIST_H
#define ENDPOINTLIST_H

#include <QList>
#include <QMap>
#include <QMenu>
#include <QWidget>

#include <drouter/drmultistatewidget.h>
#include <drouter/drjparser.h>

#include "statedialog.h"

#define ENDPOINTLIST_ITEM_HEIGHT 26
#define ENDPOINTLIST_MIN_INPUT_WIDTH 250
#define ENDPOINTLIST_GPIO_WIDTH 70

class EndpointList : public QWidget
{
  Q_OBJECT
 public:
  EndpointList(Qt::Orientation orient,QWidget *parent=0);
  ~EndpointList();
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;
  int router() const;
  void setRouter(int router);
  void setParser(DRJParser *psr);
  bool showGpio() const;
  void setShowGpio(bool state);
  int slot(int endpt) const;
  void addEndpoint(int router,int endpt,const QString &name);
  void clearEndpoints();
  int endpointQuantity() const;

 public slots:
  void selectCrosspoint(int slot_x,int slot_y);
  void setPosition(int pixels);
  void setGpioState(int router,int linenum,const QString &code);

 signals:
  void hoveredEndpointChanged(int router,int rownum);

 private slots:
  void showStateDialogData();
  void connectViaHttpData();
  void connectViaLwrpData();
  void copySourceNumberData();
  void copySourceStreamAddressData();
  void copyNodeAddressData();
  void copySlotNumberData();
  void aboutToShowMenuData();

 protected:
  void mousePressEvent(QMouseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);
  void leaveEvent(QEvent *event);
  void paintEvent(QPaintEvent *e);
  void resizeEvent(QResizeEvent *e);

 private:
  int LocalEndpoint(QMouseEvent *e) const;
  QMap<int,QString> list_labels;
  QMap<int,DRMultiStateWidget *> list_gpio_widgets;
  DREndPointMap::Type list_gpio_type;
  int list_router;
  int list_position;
  DRJParser *list_parser;
  Qt::Orientation list_orientation;
  bool list_show_gpio;
  int list_width;
  QMenu *list_mouse_menu;
  int list_mouse_endpoint;
  int list_move_endpoint;
  QPoint list_mouse_position;
  QAction *list_state_dialog_action;
  QAction *list_connect_via_http_action;
  QAction *list_connect_via_lwrp_action;
  QAction *list_copy_source_number_action;
  QAction *list_copy_source_address_action;
  QAction *list_copy_host_address_action;
  QAction *list_copy_slot_number_action;
  QMap<int,StateDialog *> list_state_dialogs;
  DREndPointListModel *list_model;
  Qt::MouseButtons list_mouse_buttons;
  QString list_description_text;
  int list_selected_rownum;
  QFont list_selected_font;
};


#endif  // ENDPOINTLIST_H
