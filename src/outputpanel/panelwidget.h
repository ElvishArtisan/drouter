// panelwidget.h
//
// Output panel widget for OutputPanel
//
//   (C) Copyright 2016-2024 Fred Gleason <fredg@paravelsystems.com>
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


#ifndef PANELWIDGET_H
#define PANELWIDGET_H

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QWidget>

#include <drjparser.h>

#define PANELWIDGET_FLASH_INTERVAL 300
#define PANELWIDGET_WIDTH 140
#define PANELWIDGET_HEIGHT 190

class PanelInput
{
 public: 
  PanelInput(unsigned num,const QString &name);
  unsigned number() const;
  QString name() const;
  bool operator<(const PanelInput &other) const;

 private:
  unsigned panel_number;
  QString panel_name;
};


class PanelWidget : public QWidget
{
  Q_OBJECT
 public:
  PanelWidget(DRJParser *parser,int router,int output,QWidget *parent=0);
  ~PanelWidget();
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;

 public slots:
  void changeConnectionState(bool state,DRJParser::ConnectionState cstate);
  void changeOutputCrosspoint(int router,int output,int input);
  void tickClock(bool);

 private slots:
  void inputBoxActivatedData(int index);
  void takeButtonClickedData();
  void cancelButtonClickedData();

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  int SelectedInput() const;
  void SetArmedState(bool state);
  QLabel *widget_output_label;
  QComboBox *widget_input_box;
  QPushButton *widget_take_button;
  QPushButton *widget_cancel_button;
  DRJParser *widget_parser;
  int widget_router;
  int widget_output;
  int widget_input;
  bool widget_xpoint_synced;
  QPalette widget_blue_palette;
  QString widget_blue_stylesheet;
  QPalette widget_red_palette;
  QString widget_red_stylesheet;
};


#endif  // PANEL_WIDGET_H
