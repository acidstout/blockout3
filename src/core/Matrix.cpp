/*
  File:        Matrix.cpp
  Description: 4x4 matrix (row major, column vectors), port of GLMatrix
  Program:     Block Out III
  Based on:    BlockOut II 2.5 by Jean-Luc PONS

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "Matrix.h"
#include "CoreTypes.h"
#include <cmath>

namespace bo3 {

void Normalize(VERTEX *v) {

  float n = sqrtf( v->x * v->x + v->y * v->y + v->z * v->z );
  v->x = v->x / n;
  v->y = v->y / n;
  v->z = v->z / n;

}

// -------------------------------------

Matrix::Matrix() {
  Identity();
}

// -------------------------------------

void Matrix::Init33(float m11,float m12,float m13,float m21,float m22,float m23,float m31,float m32,float m33) {

  this->_11 = m11; this->_12 = m12; this->_13 = m13;
  this->_21 = m21; this->_22 = m22; this->_23 = m23;
  this->_31 = m31; this->_32 = m32; this->_33 = m33;

}

// -------------------------------------

void Matrix::Identity() {

  _11=1.0f;  _12=0.0f;  _13=0.0f; _14=0.0f;
  _21=0.0f;  _22=1.0f;  _23=0.0f; _24=0.0f;
  _31=0.0f;  _32=0.0f;  _33=1.0f; _34=0.0f;
  _41=0.0f;  _42=0.0f;  _43=0.0f; _44=1.0f;

}

// -------------------------------------

void Matrix::Translate(float x,float y,float z) {

  Identity();
  _14 = x;
  _24 = y;
  _34 = z;

}

// -------------------------------------

void Matrix::Transpose() {

  Matrix m = *this;

  _11=m._11;  _12=m._21;  _13=m._31; _14=m._41;
  _21=m._12;  _22=m._22;  _23=m._32; _24=m._42;
  _31=m._13;  _32=m._23;  _33=m._33; _34=m._43;
  _41=m._14;  _42=m._24;  _43=m._34; _44=m._44;

}

// -------------------------------------

void Matrix::Multiply(const Matrix *b) {

  Matrix a = *this;
  Multiply(b,&a);

}

void Matrix::Multiply(const Matrix *m2,const Matrix *m1) {

  // Copy first: 'this' may alias one of the operands
  Matrix r;

  r._11 = m1->_11*m2->_11 + m1->_12*m2->_21 + m1->_13*m2->_31 + m1->_14*m2->_41;
  r._12 = m1->_11*m2->_12 + m1->_12*m2->_22 + m1->_13*m2->_32 + m1->_14*m2->_42;
  r._13 = m1->_11*m2->_13 + m1->_12*m2->_23 + m1->_13*m2->_33 + m1->_14*m2->_43;
  r._14 = m1->_11*m2->_14 + m1->_12*m2->_24 + m1->_13*m2->_34 + m1->_14*m2->_44;

  r._21 = m1->_21*m2->_11 + m1->_22*m2->_21 + m1->_23*m2->_31 + m1->_24*m2->_41;
  r._22 = m1->_21*m2->_12 + m1->_22*m2->_22 + m1->_23*m2->_32 + m1->_24*m2->_42;
  r._23 = m1->_21*m2->_13 + m1->_22*m2->_23 + m1->_23*m2->_33 + m1->_24*m2->_43;
  r._24 = m1->_21*m2->_14 + m1->_22*m2->_24 + m1->_23*m2->_34 + m1->_24*m2->_44;

  r._31 = m1->_31*m2->_11 + m1->_32*m2->_21 + m1->_33*m2->_31 + m1->_34*m2->_41;
  r._32 = m1->_31*m2->_12 + m1->_32*m2->_22 + m1->_33*m2->_32 + m1->_34*m2->_42;
  r._33 = m1->_31*m2->_13 + m1->_32*m2->_23 + m1->_33*m2->_33 + m1->_34*m2->_43;
  r._34 = m1->_31*m2->_14 + m1->_32*m2->_24 + m1->_33*m2->_34 + m1->_34*m2->_44;

  r._41 = m1->_41*m2->_11 + m1->_42*m2->_21 + m1->_43*m2->_31 + m1->_44*m2->_41;
  r._42 = m1->_41*m2->_12 + m1->_42*m2->_22 + m1->_43*m2->_32 + m1->_44*m2->_42;
  r._43 = m1->_41*m2->_13 + m1->_42*m2->_23 + m1->_43*m2->_33 + m1->_44*m2->_43;
  r._44 = m1->_41*m2->_14 + m1->_42*m2->_24 + m1->_43*m2->_34 + m1->_44*m2->_44;

  *this = r;

}

// -------------------------------------

void Matrix::TransfomVec(float x,float y,float z,float w,float *rx,float *ry,float *rz,float *rw) const {

  *rx = x * _11 + y * _12 + z * _13 + w * _14;
  *ry = x * _21 + y * _22 + z * _23 + w * _24;
  *rz = x * _31 + y * _32 + z * _33 + w * _34;
  *rw = x * _41 + y * _42 + z * _43 + w * _44;

}

// -------------------------------------

void Matrix::RotateX(float angle) {

  float cs = cosf(angle);
  float sn = sinf(angle);

  Identity();
  _22=cs;   _23=-sn;
  _32=sn;   _33=cs;

}

// -------------------------------------

void Matrix::RotateY(float angle) {

  float cs = cosf(angle);
  float sn = sinf(angle);

  Identity();
  _11=cs;  _13=sn;
  _31=-sn; _33=cs;

}

// -------------------------------------

void Matrix::RotateZ(float angle) {

  float cs = cosf(angle);
  float sn = sinf(angle);

  Identity();
  _11=cs;   _12=-sn;
  _21=sn;   _22=cs;

}

} // namespace bo3
