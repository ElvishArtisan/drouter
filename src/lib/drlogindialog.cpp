// drlogindialog.cpp
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

#include "drlogindialog.h"

DRLoginDialog::DRLoginDialog(const QString &title,QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle(title+" - "+tr("Login"));

  QFont label_font(font().family(),font().pointSize(),QFont::Bold);

  setMinimumSize(sizeHint());
  setMaximumHeight(sizeHint().height());

  d_username_label=new QLabel(tr("Username")+":",this);
  d_username_label->setFont(label_font);
  d_username_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_username_edit=new QLineEdit(this);

  d_password_label=new QLabel(tr("Password")+":",this);
  d_password_label->setFont(label_font);
  d_password_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_password_edit=new QLineEdit(this);
  d_password_edit->setEchoMode(QLineEdit::Password);

  d_ok_button=new QPushButton(tr("OK"),this);
  d_ok_button->setFont(label_font);
  d_ok_button->setDefault(true);
  connect(d_ok_button,SIGNAL(clicked()),this,SLOT(okData()));

  d_cancel_button=new QPushButton(tr("Cancel"),this);
  d_cancel_button->setFont(label_font);
  connect(d_cancel_button,SIGNAL(clicked()),this,SLOT(cancelData()));
}


QSize DRLoginDialog::sizeHint() const
{
  return QSize(400,120);
}


int DRLoginDialog::exec(QString *username,QString *password)
{
  d_username_edit->setText(*username);
  d_username_edit->selectAll();
  d_password_edit->setText(*password);
  d_username=username;
  d_password=password;
  return QDialog::exec();
}


void DRLoginDialog::closeEvent(QCloseEvent *e)
{
  cancelData();
}


void DRLoginDialog::resizeEvent(QResizeEvent *e)
{
  d_username_label->setGeometry(10,10,100,20);
  d_username_edit->setGeometry(115,10,size().width()-125,20);

  d_password_label->setGeometry(10,32,100,20);
  d_password_edit->setGeometry(115,32,size().width()-125,20);

  d_ok_button->setGeometry(size().width()-180,size().height()-60,80,50);

  d_cancel_button->setGeometry(size().width()-90,size().height()-60,80,50);
}


void DRLoginDialog::okData()
{
  *d_username=d_username_edit->text();
  *d_password=d_password_edit->text();
  done(true);
}


void DRLoginDialog::cancelData()
{
  done(false);
}
