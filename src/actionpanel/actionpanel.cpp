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
  QString time_format="hh:mm:ss";
  QString date_format="dddd, MMMM d yyyy";

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
    if(cmd->key(i)=="--date-format") {
      date_format=cmd->value(i);
      cmd->setProcessed(i,true);
    }
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
    if(cmd->key(i)=="--time-format") {
      time_format=cmd->value(i);
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
  d_action_view->sortByColumn(1,Qt::AscendingOrder);  // Sort by time
  connect(d_action_view,SIGNAL(doubleClicked(const QModelIndex &)),
	  this,SLOT(doubleClickedData(const QModelIndex &)));

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
  // Wall Clock
  //
  d_wall_clock=new WallClock(this);
  d_wall_clock->setTimeFormat(time_format);
  d_wall_clock->setDateFormat(date_format);

  //
  // Close Button
  //
  d_close_button=new QPushButton(tr("Close"),this);
  d_close_button->setFont(button_font);
  connect(d_close_button,SIGNAL(clicked()),this,SLOT(closeData()));

  //
  // Fix the Window Size
  //
  setMinimumSize(sizeHint());

  //
  // The Protocol J Connection
  //
  d_parser=new DRJParser(false,this);
  d_parser->setModelFont(font());
  d_parser->setModelPalette(palette());
  d_parser->setDateFormat(date_format);
  d_parser->setTimeFormat(time_format);
  d_parser->setRouterFilter(router_filter);
  connect(d_parser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(connectedData(bool,DRJParser::ConnectionState)));
  connect(d_parser,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
  connect(d_parser,SIGNAL(parserError(DRJParser::ErrorType,const QString &)),
	  this,SLOT(parserErrorData(DRJParser::ErrorType,const QString &)));

  //
  // Dialogs
  //
  d_edit_dialog=new EditActionDialog(d_parser,this);

  d_router_box->setModel(d_parser->routerModel());
  d_router_box->setModelColumn(0);

  d_parser->connectToHost(d_hostname,9600,"","");
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
  int router=
    d_parser->routerModel()->routerNumber(d_router_box->currentIndex());
  QVariantMap fields;
  if(d_edit_dialog->exec(router,&fields)) {
    d_parser->saveAction(router,fields);
  }
}


void MainWidget::editData()
{
  int rownum=SelectedRow();

  if(rownum>=0) {
    int router=
      d_parser->routerModel()->routerNumber(d_router_box->currentIndex());
    DRActionListModel *amodel=d_parser->actionModel(router);
    QVariantMap fields=amodel->rowMetadata(rownum);
    if(d_edit_dialog->exec(router,&fields)) {
      d_parser->saveAction(router,fields);
    }
  }
}


void MainWidget::doubleClickedData(const QModelIndex &index)
{
  editData();
}


void MainWidget::deleteData()
{
  int rownum=SelectedRow();

  if(rownum>=0) {
    int router=
      d_parser->routerModel()->routerNumber(d_router_box->currentIndex());
    DRActionListModel *amodel=d_parser->actionModel(router);
    QVariantMap fields=amodel->rowMetadata(rownum);

    if(QMessageBox::question(this,"ActionPanel - "+tr("Delete Action"),
			     tr("Are you sure you want to delete the action")+
			     " \""+fields.value("comment").toString()+"\"?",
			     QMessageBox::Yes,QMessageBox::No)==
       QMessageBox::Yes) {
      d_parser->removeAction(fields.value("id").toInt());
    }
  }
}


void MainWidget::closeData()
{
  exit(0);
}


void MainWidget::routerBoxActivatedData(int n)
{
  if(n>=0) {
    int router=d_parser->routerModel()->routerNumber(n);

    d_action_view->setModel(d_parser->actionModel(router));
    d_action_view->resizeColumnsToContents();
    d_action_view->resizeRowsToContents();
    d_action_view->horizontalHeader()->setStretchLastSection(true);
    d_action_view->verticalHeader()->setVisible(false);

    d_parser->actionModel(router)->sort(1,Qt::AscendingOrder);
  }
}


void MainWidget::connectedData(bool state,DRJParser::ConnectionState cstate)
{
  if(state) {
    d_action_view->setSortingEnabled(true);
    d_initial_connected=true;
    if(d_initial_router>0) {
      DRActionListModel *amodel=d_parser->actionModel(d_initial_router);
      if(amodel==NULL) {
	QMessageBox::warning(this,"ActionPanel - "+tr("Error"),
			     tr("Router number")+
			     QString::asprintf(" %d ",d_initial_router)+
			     tr("does not exist."));
	exit(1);
      }
      d_router_box->setCurrentIndex(d_parser->routerModel()->
				    rowNumber(d_initial_router));
      routerBoxActivatedData(d_router_box->currentIndex());
    }
    else {
      if(d_router_box->count()>0) {
	d_router_box->setCurrentIndex(0);
	routerBoxActivatedData(d_router_box->currentIndex());
      }
    }
    d_router_label->setEnabled(true);
    d_router_box->setEnabled(true);
  }
  else {
    d_action_view->setSortingEnabled(false);
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


void MainWidget::parserErrorData(DRJParser::ErrorType type,
				 const QString &remarks)
{
  QMessageBox::warning(this,"ActionPanel - "+tr("Error"),
		       tr("Server error")+": "+remarks);
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

  d_wall_clock->setGeometry((w-360)/2,h-70,360,70);

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