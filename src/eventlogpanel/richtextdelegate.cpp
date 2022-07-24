// richtextdelegate.cpp
//
// Allow "rich text" styling to be used in a Qt view widget.
//
//   (C) Copyright 2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QTextDocument>

#include "richtextdelegate.h"

void RichTextDelegate::paint(QPainter *p,const QStyleOptionViewItem &option,
			     const QModelIndex &index) const
{
  QStyleOptionViewItem opt=option;
  initStyleOption(&opt,index);

  QStyle *style=opt.widget? opt.widget->style() : QApplication::style();

  QTextDocument doc;
  doc.setHtml(opt.text);

  // No text?
  opt.text=QString();
  style->drawControl(QStyle::CE_ItemViewItem,&opt,p);

  QAbstractTextDocumentLayout::PaintContext ctx;

  // Item Selected?
  if(opt.state&QStyle::State_Selected) {
    ctx.palette.setColor(QPalette::Text,opt.palette.
			 color(QPalette::Active,QPalette::HighlightedText));
  }

  QRect rect=style->subElementRect(QStyle::SE_ItemViewItemText,&opt);
  p->save();
  p->translate(rect.topLeft());
  p->setClipRect(rect.translated(-rect.topLeft()));
  doc.documentLayout()->draw(p,ctx);
  p->restore();
}


QSize RichTextDelegate::sizeHint(const QStyleOptionViewItem &option,
				 QModelIndex &index) const
{
  QStyleOptionViewItem opt=option;
  initStyleOption(&opt,index);

  QTextDocument doc;
  doc.setHtml(opt.text);
  doc.setTextWidth(opt.rect.width());

  return QSize(doc.idealWidth(),doc.size().height());
}
