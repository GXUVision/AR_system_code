// Copyright 2008 Isis Innovation Limited
#include "EyeGame.h"
#include "OpenGL.h"
#include <cvd/convolution.h>
#include <fstream>
#include <iostream>

using namespace CVD;

EyeGame::EyeGame()
{
  mdEyeRadius = 0.1;
  mdShadowHalfSize = 2.5 * mdEyeRadius;
  mbInitialised = false;
}

void EyeGame::DrawStuff(Vector<3> v3CameraPos)
{
  if(!mbInitialised)
    Init();

  mnFrameCounter ++;

  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CW);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);
  
  GLfloat af[4]; //创建光源
  af[0]=0.5; af[1]=0.5; af[2]=0.5; af[3]=1.0;
  glLightfv(GL_LIGHT0, GL_AMBIENT, af);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, af);
  af[0]=1.0; af[1]=0.0; af[2]=1.0; af[3]=0.0;
  glLightfv(GL_LIGHT0, GL_POSITION, af);
  af[0]=1.0; af[1]=1.0; af[2]=1.0; af[3]=1.0;
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, af);//指定用于光照计算的当前材质属性
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);
  
  glMatrixMode(GL_MODELVIEW);//接下来对模型视图进行操作
  
  for(int i=0; i<1; i++)
    {
      if(mnFrameCounter < 100)
	LookAt(i, 500.0 * (Vector<3>) makeVector( (i<2?-1:1)*(mnFrameCounter < 50 ? -1 : 1) * -0.4 , -0.1, 1) , 0.05 );
      else
	LookAt(i, v3CameraPos, 0.02 );
      
      glLoadIdentity();
      glMultMatrix(ase3WorldFromEye[i]);
      glScaled(mdEyeRadius, mdEyeRadius, mdEyeRadius);
      glCallList(mnEyeDisplayList);
    }
  
  glDisable(GL_LIGHTING);
  
  glLoadIdentity();//重置当前指定的矩阵为单位矩阵
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, mnShadowTex);
  glEnable(GL_BLEND);
  glColor4f(0,0,0,0.5);
  glBegin(GL_QUADS);
  glTexCoord2f(0,0);
  glVertex2d(-mdShadowHalfSize, -mdShadowHalfSize);
  glTexCoord2f(0,1);
  glVertex2d(-mdShadowHalfSize,  mdShadowHalfSize);
  glTexCoord2f(1,1);
  glVertex2d( mdShadowHalfSize,  mdShadowHalfSize);
  glTexCoord2f(1,0);
  glVertex2d( mdShadowHalfSize, -mdShadowHalfSize);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
};


void EyeGame::Reset()
{
  for(int i=0; i<4; i++)
    ase3WorldFromEye[i] = SE3<>();//李代数

  ase3WorldFromEye[0].get_translation()[0] = -mdEyeRadius;
  ase3WorldFromEye[1].get_translation()[0] = mdEyeRadius;
  ase3WorldFromEye[2].get_translation()[0] = -mdEyeRadius;
  ase3WorldFromEye[3].get_translation()[0] = mdEyeRadius;

  ase3WorldFromEye[0].get_translation()[1] = -mdEyeRadius;
  ase3WorldFromEye[1].get_translation()[1] = -mdEyeRadius;
  ase3WorldFromEye[2].get_translation()[1] = mdEyeRadius;
  ase3WorldFromEye[3].get_translation()[1] = mdEyeRadius;

  ase3WorldFromEye[0].get_translation()[2] = mdEyeRadius;
  ase3WorldFromEye[1].get_translation()[2] = mdEyeRadius;
  ase3WorldFromEye[2].get_translation()[2] = mdEyeRadius;
  ase3WorldFromEye[3].get_translation()[2] = mdEyeRadius;
  mnFrameCounter = 0;
};




ObjLoader::ObjLoader(string filename)
{
	string line;
	fstream f;
	f.open(filename, ios::in);
	if (!f.is_open()){
		cout << "Something Went Wrong When Opening Objfiles" << endl;
	}

	while (!f.eof()){
		getline(f, line);//拿到obj文件中一行，作为一个字符串
		vector<string>parameters;
		string tailMark = " ";
		string ans = "";

		line = line.append(tailMark);
		for (int i = 0; i < line.length(); i++) {
			char ch = line[i];
			if (ch != ' ') {
				ans += ch;
			}
			else {
				parameters.push_back(ans); //取出字符串中的元素，以空格切分
				ans = "";
			}
		}
		//cout << parameters.size() << endl;
		if (parameters.size() != 4) {
			cout << "the size is not correct" << endl;
		}
		else {
			if (parameters[0] == "v") {   //如果是顶点的话
				vector<GLfloat>Point;
				for (int i = 1; i < 4; i++) {   //从1开始，将顶点的xyz三个坐标放入顶点vector
					GLfloat xyz = atof(parameters[i].c_str());
					Point.push_back(xyz);
				}
				vSets.push_back(Point);
			}

			else if (parameters[0] == "f") {   //如果是面的话，存放三个顶点的索引
				vector<GLint>vIndexSets;
			
				for (int i = 1; i < 4; i++){
					string x = parameters[i];
					string ans = "";
					for (int j = 0; j < x.length(); j=j+1) {   //跳过‘/’
						char ch = x[j];
						if (ch != '/') {
							ans += ch;
						}
						else {
							break;
						}
					}
					GLint index = atof(ans.c_str());
					index = index--;//因为顶点索引在obj文件中是从1开始的，而我们存放的顶点vector是从0开始的，因此要减1
					vIndexSets.push_back(index);
				}
				fSets.push_back(vIndexSets);
			}
		}
	}
	f.close();
}

void ObjLoader::Draw(){

	glBegin(GL_TRIANGLES);//开始绘制
	for (int i = 0; i < fSets.size(); i++) {
		GLfloat VN[3];
		//三个顶点
		GLfloat SV1[3];
		GLfloat SV2[3];
		GLfloat SV3[3];

		if ((fSets[i]).size() != 3) {
			cout << "the fSetsets_Size is not correct" << endl;
		}
		else {
			GLint firstVertexIndex = (fSets[i])[0];//取出顶点索引
			GLint secondVertexIndex = (fSets[i])[1];
			GLint thirdVertexIndex = (fSets[i])[2];

			SV1[0] = (vSets[firstVertexIndex])[0];//第一个顶点
			SV1[1] = (vSets[firstVertexIndex])[1];
			SV1[2] = (vSets[firstVertexIndex])[2];

			SV2[0] = (vSets[secondVertexIndex])[0]; //第二个顶点
			SV2[1] = (vSets[secondVertexIndex])[1];
			SV2[2] = (vSets[secondVertexIndex])[2];

			SV3[0] = (vSets[thirdVertexIndex])[0]; //第三个顶点
			SV3[1] = (vSets[thirdVertexIndex])[1];
			SV3[2] = (vSets[thirdVertexIndex])[2];


			GLfloat vec1[3], vec2[3], vec3[3];//计算法向量
			//(x2-x1,y2-y1,z2-z1)
			vec1[0] = SV1[0] - SV2[0];
			vec1[1] = SV1[1] - SV2[1];
			vec1[2] = SV1[2] - SV2[2];

			//(x3-x2,y3-y2,z3-z2)
			vec2[0] = SV1[0] - SV3[0];
			vec2[1] = SV1[1] - SV3[1];
			vec2[2] = SV1[2] - SV3[2];

			//(x3-x1,y3-y1,z3-z1)
			vec3[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
			vec3[1] = vec2[0] * vec1[2] - vec2[2] * vec1[0];
			vec3[2] = vec2[1] * vec1[0] - vec2[0] * vec1[1];

			GLfloat D = sqrt(pow(vec3[0], 2) + pow(vec3[1], 2) + pow(vec3[2], 2));

			VN[0] = vec3[0] / D;
			VN[1] = vec3[1] / D;
			VN[2] = vec3[2] / D;

			glNormal3f(VN[0], VN[1], VN[2]);//绘制法向量

			glVertex3f(SV1[0], SV1[1], SV1[2]);//绘制三角面片
			glVertex3f(SV2[0], SV2[1], SV2[2]);
			glVertex3f(SV3[0], SV3[1], SV3[2]);
		}
	}
	glEnd();
}


void EyeGame::DrawEye()
{
	string filePath = "C://Users//zyyyyc//Desktop//DICOMimage//monkey.obj";
	ObjLoader objModel = ObjLoader(filePath);
	objModel.Draw();//绘制obj模型
	
}

void EyeGame::Init()//初始化
{
  if(mbInitialised) return;
  mbInitialised = true;
  // Set up the display list for the eyeball.
  mnEyeDisplayList = glGenLists(1);
  glNewList(mnEyeDisplayList,GL_COMPILE);
  DrawEye();
  glEndList();
  MakeShadowTex();
};


void EyeGame::LookAt(int nEye, Vector<3> v3, double dRotLimit)
{
  Vector<3> v3E = ase3WorldFromEye[nEye].inverse() * v3;
  if(v3E * v3E == 0.0)
    return;
  
  normalize(v3E);
  Matrix<3> m3Rot = Identity;
  m3Rot[2] = v3E;
  m3Rot[0] -= m3Rot[2]*(m3Rot[0]*m3Rot[2]); 
  normalize(m3Rot[0]);
  m3Rot[1] = m3Rot[2] ^ m3Rot[0];
  
  SO3<> so3Rotator = m3Rot;
  Vector<3> v3Log = so3Rotator.ln();
  v3Log[2] = 0.0;
  double dMagn = sqrt(v3Log * v3Log);
  if(dMagn > dRotLimit)
    {
      v3Log = v3Log * ( dRotLimit / dMagn);
    }
  ase3WorldFromEye[nEye].get_rotation() = ase3WorldFromEye[nEye].get_rotation() * SO3<>::exp(-v3Log);
};

void EyeGame::MakeShadowTex()
{
  const int nTexSize = 256;
  Image<byte> imShadow(ImageRef(nTexSize, nTexSize));
  double dFrac = 1.0 - mdEyeRadius / mdShadowHalfSize;
  double dCenterPos = dFrac * nTexSize / 2 - 0.5;
  ImageRef irCenter; irCenter.x = irCenter.y = (int) dCenterPos;
  int nRadius = int ((nTexSize / 2 - irCenter.x) * 1.05);
  unsigned int nRadiusSquared = nRadius*nRadius;
  ImageRef ir;
  for(ir.y = 0; 2 * ir.y < nTexSize; ir.y++)
    for(ir.x = 0; 2 * ir.x < nTexSize; ir.x++)
      {
	byte val = 0;
	if((ir - irCenter).mag_squared() < nRadiusSquared)
	  val = 255;
	imShadow[ir] = val;
	imShadow[ImageRef(nTexSize - 1 - ir.x, ir.y)] = val;
	imShadow[ImageRef(nTexSize - 1 - ir.x, nTexSize - 1 - ir.y)] = val;
	imShadow[ImageRef(ir.x, nTexSize - 1 - ir.y)] = val;
      }
  
  convolveGaussian(imShadow,4.0 );
  glGenTextures(1, &mnShadowTex);
  glBindTexture(GL_TEXTURE_2D,mnShadowTex);
  glTexImage2D(GL_TEXTURE_2D, 0, 
	       GL_ALPHA, nTexSize, nTexSize, 0, 
	       GL_ALPHA, GL_UNSIGNED_BYTE, imShadow.data()); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
};





