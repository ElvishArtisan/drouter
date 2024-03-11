// sendmailtest.h
//
// Test the email sending routines.
//
//   (C) Copyright 2020-2022 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef SENDMAILTEST_H
#define SENDMAILTEST_H

#include <QObject>

#define SENDMAILTEST_USAGE "[options]\n\nTest the Rivendell sending routines\n\nOptions are:\n--from-addr=<addr>\n     Originating email address\n\n--to-addrs=<addrs>\n     To addresses (comma seperated)\n\n--cc-addresses=<addrs>\n     CC addrs (comma seperated)\n\n--bcc-addrs=<addrs>\n     BCC addresses (comma seperated)\n\n--subject=<str>\n     Message subject\n\n--body=<str>\n     Message body\n\n--body-file=<filename>\n     Use contents of <filename> for body\n\n--dry-run\n     Print the raw message to STDOUT, then exit\n\n"

class MainObject : public QObject
{
  Q_OBJECT;
 public:
  MainObject(QObject *parent=0);
};


#endif  // SENDMAILTEST_H
