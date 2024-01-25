// drlogindialog.h
//
// Dialog for login information
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef DRLOGINDIALOG_H
#define DRLOGINDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class DRLoginDialog : public QDialog
{
 Q_OBJECT
 public:
  DRLoginDialog(const QString &title,QWidget *parent=0);
  QSize sizeHint() const;

 public slots:
  int exec(QString *username,QString *password);

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private slots:
  void okData();
  void cancelData();

 private:
  QLabel *d_username_label;
  QLineEdit *d_username_edit;
  QLabel *d_password_label;
  QLineEdit *d_password_edit;
  QPushButton *d_ok_button;
  QPushButton *d_cancel_button;
  QString *d_username;
  QString *d_password;
};


#endif  // DRLOGINDIALOG_H
