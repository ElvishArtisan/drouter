// editactiondialog.h
//
// Dialog for editing a Drouter action.
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef EDITACTIONDIALOG_H
#define EDITACTIONDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimeEdit>

#include <drouter/dractionlistmodel.h>
#include <drouter/drjparser.h>

#include "dowselector.h"

class EditActionDialog : public QDialog
{
 Q_OBJECT
 public:
  EditActionDialog(DRJParser *parser,QWidget *parent=0);
  QSize sizeHint() const;

 public slots:
  int exec(int router,QVariantMap *fields);

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private slots:
  void okData();
  void cancelData();

 private:
  QCheckBox *d_active_check;
  QLabel *d_active_label;
  QLabel *d_time_label;
  QTimeEdit *d_time_edit;
  DowSelector *d_dow_selector;
  QLabel *d_comment_label;
  QLineEdit *d_comment_edit;
  QLabel *d_destination_label;
  QComboBox *d_destination_box;
  QLabel *d_source_label;
  QComboBox *d_source_box;
  QPushButton *d_ok_button;
  QPushButton *d_cancel_button;
  DRJParser *d_parser;
  int d_id;
  int d_router;
  QVariantMap *d_fields;
};


#endif  // EDITACTIONDIALOG_H
