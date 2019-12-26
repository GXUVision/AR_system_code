// -*- c++ -*-
// Copyright 2008 Isis Innovation Limited
//
// EyeGame.h
// Declares the EyeGame class
// EyeGame is a trivial AR app which draws some 3D graphics
// Draws a bunch of 3d eyeballs remniscient of the 
// AVL logo
//
#ifndef __EYEGAME_H
#define __EYEGAME_H
#include <TooN/TooN.h>
using namespace TooN;
#include "OpenGL.h"
using namespace std;
class EyeGame
{
 public:
  EyeGame();
  void DrawStuff(Vector<3> v3CameraPos);
  void Reset();
  void Init();

  
 protected:
  bool mbInitialised;
  void DrawEye();
  void LookAt(int nEye, Vector<3> v3, double dRotLimit);
  void MakeShadowTex();
 
  GLuint mnEyeDisplayList;
  GLuint mnShadowTex;
  double mdEyeRadius;
  double mdShadowHalfSize;
  SE3<> ase3WorldFromEye[4];
  int mnFrameCounter;

};
class ObjLoader{
public:
	ObjLoader(string filename);//构造函数
	void Draw();//绘制函数
private:
	vector<vector<GLfloat>>vSets;//存放顶点(x,y,z)坐标
	vector<vector<GLint>>fSets;//存放面的三个顶点索引
};


#endif
