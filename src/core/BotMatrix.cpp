/*
  File:        BotMatrix.cpp
  Description: AI player (port)
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

// -------------------------------------
#include "BotMatrix.h"
#include <math.h>

namespace bo3 {

// -------------------------------------

BotMatrix::BotMatrix() {
  Identity();
}

// -------------------------------------

void BotMatrix::Init33(float m11,float m12,float m13,float m21,float m22,float m23,float m31,float m32,float m33) {

  this->_11 = m11; this->_12 = m12; this->_13 = m13;
  this->_21 = m21; this->_22 = m22; this->_23 = m23;
  this->_31 = m31; this->_32 = m32; this->_33 = m33;

}

// -------------------------------------
  
void BotMatrix::Identity() {

  _11=1.0f;  _12=0.0f;  _13=0.0f;
  _21=0.0f;  _22=1.0f;  _23=0.0f;
  _31=0.0f;  _32=0.0f;  _33=1.0f;

}
    
// -------------------------------------

void BotMatrix::RotateOX() {

  //matRotOx= 1.0f , 0.0f , 0.0f
  //          0.0f , 0.0f , 1.0f
  //          0.0f ,-1.0f , 0.0f
  
  BotMatrix m1 = *this;
  
  //this->_11 =  m1._11;
  this->_12 = -m1._13;
  this->_13 =  m1._12;

  //this->_21 =  m1._21;
  this->_22 = -m1._23;
  this->_23 =  m1._22;

  //this->_31 =  m1._31;
  this->_32 = -m1._33;
  this->_33 =  m1._32;

}

// -------------------------------------

void BotMatrix::RotateOY() {
  
  //  matRotOy=  0.0f , 0.0f , 1.0f
  //             0.0f , 1.0f , 0.0f
  //            -1.0f , 0.0f , 0.0f
  
  BotMatrix m1 = *this;
  
  this->_11 = -m1._13;
  //this->_12 =  m1._12;
  this->_13 =  m1._11;

  this->_21 = -m1._23;
  //this->_22 =  m1._22;
  this->_23 =  m1._21;

  this->_31 = -m1._33;
  //this->_32 =  m1._32;
  this->_33 =  m1._31;
  
}

// -------------------------------------

void BotMatrix::Transpose() {

  BotMatrix m1 = *this;
  this->_11 = m1._11; this->_12 = m1._21; this->_13 = m1._31;
  this->_21 = m1._12; this->_22 = m1._22; this->_23 = m1._32;
  this->_31 = m1._13; this->_32 = m1._23; this->_33 = m1._33;

}

// -------------------------------------

void BotMatrix::RotateOZ() {

  //  matRotOz= 0.0f , 1.0f , 0.0f
  //           -1.0f , 0.0f , 0.0f
  //            0.0f , 0.0f , 1.0f

  BotMatrix m1 = *this;
  
  this->_11 = -m1._12;
  this->_12 =  m1._11;
  //this->_13 =  m1._13;

  this->_21 = -m1._22;
  this->_22 =  m1._21;
  //this->_23 =  m1._23;

  this->_31 = -m1._32;
  this->_32 =  m1._31;
  //this->_33 =  m1._33;

}

// -------------------------------------

void BotMatrix::RotateNOX() {

  //   matRotNOx= 1.0f , 0.0f , 0.0f
  //              0.0f , 0.0f ,-1.0f
  //              0.0f , 1.0f , 0.0f

  BotMatrix m1 = *this;
  
  //this->_11 =  m1._11;
  this->_12 =  m1._13;
  this->_13 = -m1._12;

  //this->_21 =  m1._21;
  this->_22 =  m1._23;
  this->_23 = -m1._22;

  //this->_31 =  m1._31;
  this->_32 =  m1._33;
  this->_33 = -m1._32;

}

// -------------------------------------

void BotMatrix::RotateNOY() {

  //matRotNOy=  0.0f , 0.0f ,-1.0f
  //            0.0f , 1.0f , 0.0f
  //            1.0f , 0.0f , 0.0f

  BotMatrix m1 = *this;
  
  this->_11 =  m1._13;
  //this->_12 =  m1._12;
  this->_13 = -m1._11;

  this->_21 =  m1._23;
  //this->_22 =  m1._22;
  this->_23 = -m1._21;

  this->_31 =  m1._33;
  //this->_32 =  m1._32;
  this->_33 = -m1._31;

}

// -------------------------------------

void BotMatrix::RotateNOZ() {

  //matRotNOz= 0.0f ,-1.0f , 0.0f
  //           1.0f , 0.0f , 0.0f
  //           0.0f , 0.0f , 1.0f
  
  BotMatrix m1 = *this;
  
  this->_11 =  m1._12;
  this->_12 = -m1._11;
  //this->_13 =  m1._13;

  this->_21 =  m1._22;
  this->_22 = -m1._21;
  //this->_23 =  m1._23;

  this->_31 =  m1._32;
  this->_32 = -m1._31;
  //this->_33 =  m1._33;

}

} // namespace bo3
