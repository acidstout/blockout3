/*
  File:        Matrix.h
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
#pragma once

namespace bo3 {

class Matrix {

public:

  Matrix();

  void Init33(float _11,float _12,float _13,
              float _21,float _22,float _23,
              float _31,float _32,float _33);

  void Identity();
  void RotateX(float angle);
  void RotateY(float angle);
  void RotateZ(float angle);
  void Translate(float x,float y,float z);
  void Transpose();

  // this = b * a  (note the order: kept from BlockOut II, callers rely on it)
  void Multiply(const Matrix *a,const Matrix *b);
  // this = this * b
  void Multiply(const Matrix *b);

  void TransfomVec(float x,float y,float z,float w,float *rx,float *ry,float *rz,float *rw) const;

  float _11;float _12;float _13;float _14;
  float _21;float _22;float _23;float _24;
  float _31;float _32;float _33;float _34;
  float _41;float _42;float _43;float _44;

};

} // namespace bo3
