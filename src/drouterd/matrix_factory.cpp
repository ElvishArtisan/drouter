// matrix_factory.cpp
//
// Instantiate a matrix instance
//
// (C) 2023-2024 Fred Gleason <fredg@paravelsystems.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of version 2.1 of the GNU Lesser General Public
//    License as published by the Free Software Foundation;
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, 
//    Boston, MA  02111-1307  USA
//

#include "matrix_bt-41mlr.h"
#include "matrix_gvg7000.h"
#include "matrix_lwrp.h"

#include "matrix_factory.h"

Matrix *MatrixFactory(Config::MatrixType type,unsigned id,Config *conf,
		      QObject *parent)
{
  Matrix *matrix=NULL;

  switch(type) {
  case Config::LwrpMatrix:
    matrix=new MatrixLwrp(id,conf,parent);
    break;

  case Config::Bt41MlrMatrix:
    matrix=new MatrixBt41Mlr(id,conf,parent);
    break;

  case Config::Gvg7000Matrix:
    matrix=new MatrixGvg7000(id,conf,parent);
    break;

  case Config::LastMatrix:
    break;
  }
  return matrix;
}
