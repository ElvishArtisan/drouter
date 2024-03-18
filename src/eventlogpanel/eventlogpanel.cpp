// eventlogpanel.cpp
//
// Applet for reading the event log
//
//   (C) Copyright 2002-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QApplication>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>

#include <sy5/sycmdswitch.h>

#include "eventlogpanel.h"
#include "richtextdelegate.h"

//
// Icons
//
#include "../../icons/drouter-16x16.xpm"

MainWidget::MainWidget(QWidget *parent)
  : QWidget(parent)
{
  d_scrolling=false;

  QString db_hostname="localhost";
  QString db_username="drouter";
  QString db_password="drouter";
  QString db_dbname="drouter";

  if(getenv("DROUTER_HOSTNAME")!=NULL) {
    db_hostname=getenv("DROUTER_HOSTNAME");
  }

  SyCmdSwitch *cmd=new SyCmdSwitch("eventlogpanel",VERSION,EVENTLOGPANEL_USAGE);
  for(int i=0;i<(cmd->keys());i++) {
    if(cmd->key(i)=="--hostname") {
      db_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--username") {
      db_username=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--password") {
      db_password=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--dbname") {
      db_dbname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      QMessageBox::warning(this,"EventLogPanel - "+tr("Error"),
			   tr("Unknown argument")+" \""+cmd->key(i)+"\".");
      exit(1);
    }
  }

  //
  // Create And Set Icon
  //
  setWindowIcon(QPixmap(drouter_16x16_xpm));
  setWindowTitle(tr("Drouter - EventLogPanel")+" ["+VERSION+"] - "+
		 tr("Server")+": "+db_hostname);

  //
  // Fonts
  //
  QFont label_font("helvetica",12,QFont::Bold);
  label_font.setPixelSize(12);
  QFont button_font("helvetica",14,QFont::Bold);
  button_font.setPixelSize(14);

  //
  // Connect to Database
  //
  QSqlDatabase db=QSqlDatabase::addDatabase("QMYSQL3");
  db.setHostName(db_hostname);
  db.setDatabaseName(db_dbname);
  db.setUserName(db_username);
  db.setPassword(db_password);
  if(!db.open()) {
    QMessageBox::warning(this,"EventLogPanel - "+tr("Database Error"),
			 tr("unable to open database")+" ["+
			 db.lastError().driverText()+"]");
    exit(1);
  }

  //
  // Attributes Filter
  //
  d_show_attributes_label=new QLabel(tr("Line Format")+":",this);
  d_show_attributes_label->setFont(label_font);
  d_show_attributes_label->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  d_show_attributes_box=new QComboBox(this);
  connect(d_show_attributes_box,SIGNAL(activated(int)),
	  this,SLOT(showAttributesData(int)));
  d_show_attributes_box->insertItem(0,tr("Show Names and Numbers"),
				    DREventLogModel::NumberAttribute|
				    DREventLogModel::NameAttribute);
  d_show_attributes_box->insertItem(1,tr("Show Numbers"),
				    DREventLogModel::NumberAttribute);
  d_show_attributes_box->insertItem(1,tr("Show Names"),
				    DREventLogModel::NameAttribute);

  //
  // Scroll Button
  //
  d_scroll_button=new QPushButton(tr("Scroll"),this);
  connect(d_scroll_button,SIGNAL(clicked()),this,SLOT(toggleScrollingData()));

  //
  // Instance Indicator
  //
  d_instance_indicator=new InstanceIndicator(this);

  //
  // Log View
  //
  d_table_view=new QTableView(this);
  d_table_view->setItemDelegate(new RichTextDelegate());
  d_log_model=new DREventLogModel(this);
  d_table_view->setModel(d_log_model);
  d_table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
  d_table_view->setSelectionMode(QAbstractItemView::SingleSelection);
  d_table_view->setShowGrid(false);
  d_table_view->setSortingEnabled(false);
  d_table_view->setWordWrap(false);
  d_table_view->verticalHeader()->setVisible(false);
  d_table_view->horizontalHeader()->setStretchLastSection(true);

  d_table_view->resizeColumnsToContents();

  d_refresh_timer=new QTimer(this);
  d_refresh_timer->setSingleShot(true);
  connect(d_refresh_timer,SIGNAL(timeout()),this,SLOT(refreshData()));

  d_refresh_timer->start(0);
  toggleScrollingData();
}


MainWidget::~MainWidget()
{
}


QSize MainWidget::sizeHint() const
{
  return QSize(640,480);
}


void MainWidget::showAttributesData(int n)
{
  d_log_model->setShowAttributes(d_show_attributes_box->
		     itemData(d_show_attributes_box->currentIndex()).toInt());
}


void MainWidget::toggleScrollingData()
{
  if(d_scrolling) {
    d_scrolling=false;
    d_scroll_button->setStyleSheet("");
  }
  else {
    d_scrolling=true;
    d_scroll_button->setStyleSheet("color: #FFFFFF; background-color: #0000FF");
  }
}


void MainWidget::refreshData()
{
  QModelIndex index=d_log_model->refresh();

  if(index.isValid()&&d_scrolling) {
    d_table_view->scrollTo(index);
  }
  d_refresh_timer->start(1000);
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  d_show_attributes_label->setGeometry(10,6,90,20);
  d_show_attributes_box->setGeometry(105,4,220,20);

  d_scroll_button->setGeometry(340,5,45,20);

  d_instance_indicator->setGeometry(size().width()-100,2,90,26);

  d_table_view->setGeometry(10,20+10,size().width()-20,size().height()-20-20);
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);

  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
