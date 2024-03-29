// instanceindicator.h
//
// InstanceIndicator widget
//
//   (C) Copyright 2022 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
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

#ifndef INSTANCEINDICATOR_H
#define INSTANCEINDICATOR_H

#include <QLabel>
#include <QTimer>

class InstanceIndicator : public QLabel
{
 Q_OBJECT;
 public:
  InstanceIndicator(QWidget *parent=0);
  QSize sizeHint() const;

 private slots:
  void timeoutData();

 private:
  QTimer *d_timer;
};


#endif  // INSTANCEINDICATOR_H
