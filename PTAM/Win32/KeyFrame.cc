// Copyright 2008 Isis Innovation Limited
#include "KeyFrame.h"
#include "ShiTomasi.h"
#include "SmallBlurryImage.h"
#include <cvd/vision.h>
#include <cvd/fast_corner.h>
////用DBSCAN对fast角点进行密度聚类
#define Eps 3 // Eps为邻域半径值 
#define MinPts 3 // 邻域密度阈值 
#define N 500 // 数据集包含N个对象 



using namespace CVD;
using namespace std;
using namespace GVars3;

void KeyFrame::MakeKeyFrame_Lite(BasicImage<byte> &im)
{
  // Perpares a Keyframe from an image. Generates pyramid levels, does FAST detection, etc.
  // Does not fully populate the keyframe struct, but only does the bits needed for the tracker;
  // e.g. does not perform FAST nonmax suppression. Things like that which are needed by the 
  // mapmaker but not the tracker go in MakeKeyFrame_Rest();
  
  // First, copy out the image data to the pyramid's zero level.
  //将数据传输到金字塔的零级
	aLevels[0].im.resize(im.size());
  copy(im, aLevels[0].im);

  // Then, for each level...
  for(int i=0; i<LEVELS; i++)
    {
      Level &lev = aLevels[i];
      if(i!=0)
	{  // .. make a half-size image from the previous level..
	  lev.im.resize(aLevels[i-1].im.size() / 2);//上一层仅仅像素除以2？
	  halfSample(aLevels[i-1].im, lev.im);
	}
      
      // .. and detect and store FAST corner points.
      // I use a different threshold on each level; this is a bit of a hack
      // whose aim is to balance the different levels' relative feature densities.
	
	  
	  lev.vCorners.clear();
      lev.vCandidates.clear();
      lev.vMaxCorners.clear();
      if(i == 0)
	fast_corner_detect_10(lev.im, lev.vCorners, 10);
 	
      if(i == 1)
	fast_corner_detect_10(lev.im, lev.vCorners, 15);
      if(i == 2)
	fast_corner_detect_10(lev.im, lev.vCorners, 15);
      if(i == 3)
	fast_corner_detect_10(lev.im, lev.vCorners, 10);

	  vector<int> kernel_point; // 保存核心点在point[][]中的位置 
	  vector<int> border_point; // 保存边界点在point[][]中的位置 
	  vector<int> noise_point; // 保存噪声点在point[][]中的位置 
	  vector<vector<int> > mid; // 可能存在重叠的簇 
	  double point[N][2]; // 保存所有的数据点 
	  vector<vector<int> > cluster; // 保存最终形成的簇，每个簇中包含点在point[][]中的位置 
      
     /*  Generate row look-up-table for the FAST corner points: this speeds up 
       finding close-by corner points later on.*/
	//  DBSCAN算法聚类
	  for (int i = 0; i <N; i++)
	  {
		  point[i][0] = lev.vCorners[i].x;
		  point[i][1] = lev.vCorners[i].y;
	  }
	 /* for (int i = 0; i<lev.vCorners.size(); i++) {*/
	  for (int i = 0; i<N; i++) {
		  
		  int num = 0; // 判断是否超过MinPts，若一次循环后num>=MinPts，则加入核心点 
		  for (int j = 0; j<N; j++) {
			  if (pow(point[i][0] - point[j][0], 2) + pow(point[i][1] - point[j][1], 2) <= pow(Eps, 2)) { // 本身也算一个 
				  num++;
			  }
		  }
		  if (num >= MinPts) {
			  kernel_point.push_back(i);
		  }
	  }
	  for (int i = 0; i<N; i++) {
		  // 边界点或噪声点不能是核心点
		  int flag = 0; // 若flag=0，则该点不是核心点，若flag=1，则该点为核心点 
		  for (int j = 0; j<kernel_point.size(); j++) {
			  if (i == kernel_point[j]) {
				  flag = 1;
				  break;
			  }
		  }
		  if (flag == 0) {
			  // 判断是边界点还是噪声点 
			  int flag2 = 0; // 若flag=0，则该点为边界点，若flag=1，则该点位噪声点
			  for (int j = 0; j<kernel_point.size(); j++) {
				  int s = kernel_point[j]; // 标记第j个核心点在point[][]中的位置，方便调用 
				  if (pow(point[i][0] - point[s][0], 2) + pow(point[i][1] - point[s][1], 2)<pow(Eps, 2)) {
					  flag2 = 0;
					  border_point.push_back(i);
					  break;
				  }
				  else {
					  flag2 = 1;
					  continue;
				  }
			  }
			  if (flag2 == 1) {
				  // 加入噪声点
				  noise_point.push_back(i);
				  continue;
			  }
		  }
		  else {
			  continue;
		  }
	  }
	  for (int i = 0; i<kernel_point.size(); i++) {
		  int x = kernel_point[i];
		  vector<int> record; // 对于每一个点建立一个record，放入mid当中
		  record.push_back(x);
		  for (int j = i + 1; j<kernel_point.size(); j++) {
			  int y = kernel_point[j];
			  if (pow(point[x][0] - point[y][0], 2) - pow(point[x][1] - point[y][1], 2)<pow(Eps, 2)) {
				  record.push_back(y);
			  }
		  }
		  mid.push_back(record);
	  }

	  // 合并vector
	  for (int i = 0; i < mid.size(); i++) { // 对于mid中的每一行 
		  // 判断该行是否已经添加进前面的某一行中 
		  if (mid[i][0] == -1) {
			  continue;
		  }
		  // 如果没有被判断过 
		  for (int j = 0; j < mid[i].size(); j++) { // 判断其中的每一个值 
			  // 对每一个值判断其他行中是否存在
			  for (int x = i + 1; x < mid.size(); x++) { // 对于之后的每一行 
				  if (mid[x][0] == -1) {
					  continue;
				  }
				  for (int y = 0; y < mid[x].size(); y++) {
					  if (mid[i][j] == mid[x][y]) {
						  // 如果有一样的元素，应该放入一个vector中，并在循环后加入precluster，同时置该vector内所有元素值为-1
						  for (int a = 0; a < mid[x].size(); a++) {
							  mid[i].push_back(mid[x][a]);
							  mid[x][a] = -1;
						  }
						  break;
					  }
				  }
			  }
		  }
		  cluster.push_back(mid[i]);
	  }
	  // 删除cluster中的重复元素
	  for (int i = 0; i<cluster.size(); i++) { // 对于每一行 
		  for (int j = 0; j<cluster[i].size(); j++) {
			  for (int n = j + 1; n<cluster[i].size(); n++) {
				  if (cluster[i][j] == cluster[i][n]) {
					  cluster[i].erase(cluster[i].begin() + n);
					  n--;
				  }
			  }
		  }
	  }

	  // 至此，cluster中保存了各个簇，每个簇中有点对应在point[][]中的位置
	  // 将每个边界点指派到一个与之相关联的核心点的簇中
	  for (int i = 0; i<border_point.size(); i++) { // 对于每一个边界点 
		  int x = border_point[i];
		  for (int j = 0; j<cluster.size(); j++) { // 检查每一个簇，判断边界点与哪个簇中的核心点关联，将边界点加入到第一个核心点出现的簇中 
			  int flag = 0; // flag=0表示没有匹配的项，flag=1表示已经匹配，退出循环 
			  for (int k = 0; k<cluster[j].size(); k++) {
				  int y = cluster[j][k];
				  if (pow(point[x][0] - point[y][0], 2) + pow(point[x][1] - point[y][1], 2)<pow(Eps, 2)) {
					  cluster[j].push_back(x);
					// vector <CVD::ImageRef>().swap(lev.vCorners);
					  flag = 1;
					  break;
				  }
			  }
			  if (flag == 1) {
				  break;
			  }
		  }
		/*  vector <CVD::ImageRef>().swap(lev.vCorners);
		  for (int i = 0; i<kernel_point.size(); i++)
		  {
		
			  struct  ImageRef m;
			   
			  m.x = point[kernel_point[i]][0];
			  m.y = point[kernel_point[i]][1];

			  lev.vCorners.push_back(m);

		  }*/
	  }


	  /*******************************************************************************************/







      unsigned int v=0;
      lev.vCornerRowLUT.clear();
      for(int y=0; y<lev.im.size().y; y++)
	{
	  while(v < lev.vCorners.size() && y > lev.vCorners[v].y)
	    v++;
	  lev.vCornerRowLUT.push_back(v);
	}
    };
}

void KeyFrame::MakeKeyFrame_Rest()
{
  // Fills the rest of the keyframe structure needed by the mapmaker:
  // FAST nonmax suppression, generation of the list of candidates for further map points,
  // creation of the relocaliser's SmallBlurryImage.
  static gvar3<double> gvdCandidateMinSTScore("MapMaker.CandidateMinShiTomasiScore", 70, SILENT);
  
  // For each level...
  for(int l=0; l<LEVELS; l++)
    {
      Level &lev = aLevels[l];
      // .. find those FAST corners which are maximal..
      fast_nonmax(lev.im, lev.vCorners, 10, lev.vMaxCorners);
      // .. and then calculate the Shi-Tomasi scores of those, and keep the ones with
      // a suitably high score as Candidates, i.e. points which the mapmaker will attempt
      // to make new map points out of.

      for(vector<ImageRef>::iterator i=lev.vMaxCorners.begin(); i!=lev.vMaxCorners.end(); i++)
	{
	  if(!lev.im.in_image_with_border(*i, 10))
	    continue;
	  double dSTScore = FindShiTomasiScoreAtPoint(lev.im, 3, *i);
	  if(dSTScore > *gvdCandidateMinSTScore)
	    {
	      Candidate c;
	      c.irLevelPos = *i;
	      c.dSTScore = dSTScore;
	      lev.vCandidates.push_back(c);
	    }
	}
    };
  
  // Also, make a SmallBlurryImage of the keyframe: The relocaliser uses these.
  pSBI = new SmallBlurryImage(*this);  
  // Relocaliser also wants the jacobians..
  pSBI->MakeJacs();
}

// The keyframe struct is quite happy with default operator=, but Level needs its own
// to override CVD's reference-counting behaviour.
Level& Level::operator=(const Level &rhs)
{
  // Operator= should physically copy pixels, not use CVD's reference-counting image copy.
  im.resize(rhs.im.size());
  copy(rhs.im, im);
  
  vCorners = rhs.vCorners;
  vMaxCorners = rhs.vMaxCorners;
  vCornerRowLUT = rhs.vCornerRowLUT;
  return *this;
}

// -------------------------------------------------------------
// Some useful globals defined in LevelHelpers.h live here:
Vector<3> gavLevelColors[LEVELS];

// These globals are filled in here. A single static instance of this struct is run before main()
struct LevelHelpersFiller // Code which should be initialised on init goes here; this runs before main()
{
  LevelHelpersFiller()
  {
    for(int i=0; i<LEVELS; i++)
      {
	if(i==0)  gavLevelColors[i] = makeVector( 1.0, 0.0, 0.0);
	else if(i==1)  gavLevelColors[i] = makeVector( 1.0, 1.0, 0.0);
	else if(i==2)  gavLevelColors[i] = makeVector( 0.0, 1.0, 0.0);
	else if(i==3)  gavLevelColors[i] = makeVector( 0.0, 0.0, 0.7);
	else gavLevelColors[i] =  makeVector( 1.0, 1.0, 0.7); // In case I ever run with LEVELS > 4
      }
  }
};
static LevelHelpersFiller foo;







