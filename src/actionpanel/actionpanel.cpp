// actionpanel.cpp
//
// Action Viewer for Drouter
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

#include <stdlib.h>

#include <QApplication>
#include <QHeaderView>
#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QSettings>

#include <sy5/sycmdswitch.h>
#include <sy5/symcastsocket.h>

#include "actionpanel.h"

//
// Icons
//
#include "../../icons/drouter-16x16.xpm"

MainWidget::MainWidget(QWidget *parent)
  :QWidget(parent)
{
  bool ok=false;
  QList<int> router_filter;

  //
  // Initialize Variables
  //
  d_hostname="";
  d_initial_connected=false;
  d_initial_router=-1;

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=new SyCmdSwitch("actionpanel",VERSION,ACTIONPANEL_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--hostname") {
      d_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--initial-router") {
      d_initial_router=cmd->value(i).toInt(&ok);
      if(!ok) {
	QMessageBox::warning(this,"Actionpanel - "+tr("Error"),
			     tr("Invalid --initial-router value")+
			     " \""+cmd->value(i)+"\".");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--router") {
      int router=cmd->value(i).toInt(&ok);
      if(!ok) {
	QMessageBox::warning(this,"Actionpanel - "+tr("Error"),
			     tr("Invalid --router value")+
			     " \""+cmd->value(i)+"\".");
	exit(1);
      }
      router_filter.push_back(router);
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      QMessageBox::warning(this,"Actionpanel - "+tr("Error"),
			   tr("Unknown argument")+" \""+cmd->key(i)+"\".");
      exit(1);
    }
  }

  //
  // Get the hostname
  //
  if(d_hostname.isEmpty()) {
    if(getenv("DROUTER_HOSTNAME")!=NULL) {
      d_hostname=getenv("DROUTER_HOSTNAME");
    }
    else {
      d_hostname="localhost";
    }
  }

  //
  // Create And Set Icon
  //
  setWindowIcon(QPixmap(drouter_16x16_xpm));
  setWindowTitle(QString("ActionPanel [v")+VERSION+"]");
  //
  // Fonts
  //
  QFont label_font("helvetica",12,QFont::Bold);
  label_font.setPixelSize(12);
  QFont button_font("helvetica",14,QFont::Bold);
  button_font.setPixelSize(14);

  //
  // Router Control
  //
  d_router_label=new QLabel(tr("Router")+":",this);
  d_router_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_router_label->setFont(button_font);
  d_router_label->setDisabled(true);
  d_router_box=new QComboBox(this);
  d_router_box->setDisabled(true);
  connect(d_router_box,SIGNAL(activated(int)),
	  this,SLOT(routerBoxActivatedData(int)));

  //
  // Action View
  //
  d_action_view=new QTableView(this);
  d_action_view->setSelectionBehavior(QAbstractItemView::SelectRows);
  d_action_view->setSelectionMode(QAbstractItemView::SingleSelection);
  d_action_view->setShowGrid(false);
  d_action_view->setWordWrap(true);

  //
  // Add Button
  //
  d_add_button=new QPushButton(tr("Add"),this);
  d_add_button->setFont(button_font);
  connect(d_add_button,SIGNAL(clicked()),this,SLOT(addData()));

  //
  // Edit Button
  //
  d_edit_button=new QPushButton(tr("Edit"),this);
  d_edit_button->setFont(button_font);
  connect(d_edit_button,SIGNAL(clicked()),this,SLOT(editData()));

  //
  // Delete Button
  //
  d_delete_button=new QPushButton(tr("Delete"),this);
  d_delete_button->setFont(button_font);
  connect(d_delete_button,SIGNAL(clicked()),this,SLOT(deleteData()));

  //
  // Close Button
  //
  d_close_button=new QPushButton(tr("Close"),this);
  d_close_button->setFont(button_font);
  connect(d_close_button,SIGNAL(clicked()),this,SLOT(closeData()));

  //
  // Fix the Window Size
  //
  /*
  setMinimumWidth(sizeHint().width());
  setMaximumWidth(sizeHint().width());
  setMinimumHeight(sizeHint().height());
  setMaximumHeight(sizeHint().height());
  */
  //
  // The Protocol J Connection
  //
  d_parser=new DRJParser(this);
  d_parser->setModelFont(font());
  d_parser->setRouterFilter(router_filter);
  connect(d_parser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(connectedData(bool,DRJParser::ConnectionState)));
  connect(d_parser,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));

  //
  // Dialogs
  //
  d_edit_dialog=new EditActionDialog(d_parser,this);

  d_router_box->setModel(d_parser->routerModel());
  d_router_box->setModelColumn(0);

  d_parser->
    connectToHost(d_hostname,9600,"","");
}


MainWidget::~MainWidget()
{
}


QSize MainWidget::sizeHint() const
{
  return QSize(1024,600);
}


QSizePolicy MainWidget::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


void MainWidget::addData()
{
}


void MainWidget::editData()
{
  int row=SelectedRow();

  if(row>=0) {
    int router=
      d_parser->routerModel()->routerNumber(d_router_box->currentIndex());
    if(d_edit_dialog->exec(router,row)) {
      d_parser->saveAction(router,row);
    }
  }
}


void MainWidget::deleteData()
{
  int row=SelectedRow();

  if(row>=0) {
  }
}


void MainWidget::closeData()
{
  exit(0);
}


void MainWidget::routerBoxActivatedData(int n)
{
  int router=d_parser->routerModel()->routerNumber(n);

  d_action_view->setModel(d_parser->actionModel(router));
  d_action_view->resizeColumnsToContents();
  d_action_view->resizeRowsToContents();
  d_action_view->horizontalHeader()->setStretchLastSection(true);
  d_action_view->verticalHeader()->setVisible(false);
}


void MainWidget::connectedData(bool state,DRJParser::ConnectionState cstate)
{
  if(state) {
    if(d_initial_router>0) {
      d_router_box->setCurrentIndex(d_parser->routerModel()->
					rowNumber(d_initial_router));
    }
    d_initial_connected=true;
    routerBoxActivatedData(d_router_box->currentIndex());
    d_router_label->setEnabled(true);
    d_router_box->setEnabled(true);
  }
  else {
    if(cstate!=DRJParser::WatchdogActive) {
      QMessageBox::warning(this,"Actionpanel - "+tr("Error"),
			   tr("Login error")+": "+
			   DRJParser::connectionStateString(cstate));
      exit(1);
    }
    d_router_label->setDisabled(true);
    d_router_box->setDisabled(true);
  }
}


void MainWidget::errorData(QAbstractSocket::SocketError err)
{
  if(!d_initial_connected) {
    QMessageBox::warning(this,"Actionpanel - "+tr("Error"),
			 tr("Network Error")+": "+
			 SyMcastSocket::socketErrorText(err));
    exit(1);
  }
}


void MainWidget::closeEvent(QCloseEvent *e)
{
  closeData();
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  int w=size().width();
  int h=size().height();

  d_router_label->setGeometry(15,8,60,20);
  d_router_box->setGeometry(80,8,200,20);

  d_action_view->setGeometry(10,40,w-20,h-110);

  d_add_button->setGeometry(10,h-60,80,50);
  d_edit_button->setGeometry(100,h-60,80,50);
  d_delete_button->setGeometry(190,h-60,80,50);

  d_close_button->setGeometry(w-90,h-60,80,50);
}


int MainWidget::SelectedRow() const
{
  QModelIndexList rows=d_action_view->selectionModel()->selectedRows();

  if(rows.size()!=1) {
    return -1;
  }
  return rows.at(0).row();
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);

  //
  // Start Event Loop
  //
  MainWidget *w=new MainWidget(NULL);
  w->setGeometry(w->geometry().x(),w->geometry().y(),w->sizeHint().width(),w->sizeHint().height());
  w->show();
  return a.exec();
}
