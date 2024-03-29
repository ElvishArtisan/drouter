// xpointview.h
//
// QGraphicsScene viewer for xpointpanel(1)
//
//   (C) Copyright 2017-2022 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef XPOINTVIEW_H
#define XPOINTVIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>

#define XPOINTVIEW_CROSSPOINT_COLOR "#CCFFCC"

class XPointView : public QGraphicsView
{
  Q_OBJECT
 public:
  XPointView(QGraphicsScene *scene,QWidget *parent=0);
  QSize sizeHint() const;
  int xSlotQuantity() const;
  void setXSlotQuantity(int quan);
  int ySlotQuantity() const;
  void setYSlotQuantity(int quan);

 signals:
  void doubleClicked(int x_slot,int y_slot);
  void crosspointSelected(int x_slot,int y_slot);
  void crosspointDoubleClicked(int x_slot,int y_slot);

 protected:
  void mouseMoveEvent(QMouseEvent *e);
  void leaveEvent(QEvent *e);
  void mouseDoubleClickEvent(QMouseEvent *e);

 private:
  int d_prev_hover_x;
  int d_prev_hover_y;
  QGraphicsRectItem *d_input_cursor;
  QGraphicsRectItem *d_output_cursor;
  QColor d_selection_color;
  int d_x_slot_quantity;
  int d_y_slot_quantity;
};


#endif  // XPOINTVIEW_H
