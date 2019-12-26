// Copyright 2008 Isis Innovation Limited
#include "KeyFrame.h"
#include "ShiTomasi.h"
#include "SmallBlurryImage.h"
#include <cvd/vision.h>
#include <cvd/fast_corner.h>
////��DBSCAN��fast�ǵ�����ܶȾ���
#define Eps 3 // EpsΪ����뾶ֵ 
#define MinPts 3 // �����ܶ���ֵ 
#define N 500 // ���ݼ�����N������ 



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
  //�����ݴ��䵽���������㼶
	aLevels[0].im.resize(im.size());
  copy(im, aLevels[0].im);

  // Then, for each level...
  for(int i=0; i<LEVELS; i++)
    {
      Level &lev = aLevels[i];
      if(i!=0)
	{  // .. make a half-size image from the previous level..
	  lev.im.resize(aLevels[i-1].im.size() / 2);//��һ��������س���2��
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

	  vector<int> kernel_point; // ������ĵ���point[][]�е�λ�� 
	  vector<int> border_point; // ����߽����point[][]�е�λ�� 
	  vector<int> noise_point; // ������������point[][]�е�λ�� 
	  vector<vector<int> > mid; // ���ܴ����ص��Ĵ� 
	  double point[N][2]; // �������е����ݵ� 
	  vector<vector<int> > cluster; // ���������γɵĴأ�ÿ�����а�������point[][]�е�λ�� 
      
     /*  Generate row look-up-table for the FAST corner points: this speeds up 
       finding close-by corner points later on.*/
	//  DBSCAN�㷨����
	  for (int i = 0; i <N; i++)
	  {
		  point[i][0] = lev.vCorners[i].x;
		  point[i][1] = lev.vCorners[i].y;
	  }
	 /* for (int i = 0; i<lev.vCorners.size(); i++) {*/
	  for (int i = 0; i<N; i++) {
		  
		  int num = 0; // �ж��Ƿ񳬹�MinPts����һ��ѭ����num>=MinPts���������ĵ� 
		  for (int j = 0; j<N; j++) {
			  if (pow(point[i][0] - point[j][0], 2) + pow(point[i][1] - point[j][1], 2) <= pow(Eps, 2)) { // ����Ҳ��һ�� 
				  num++;
			  }
		  }
		  if (num >= MinPts) {
			  kernel_point.push_back(i);
		  }
	  }
	  for (int i = 0; i<N; i++) {
		  // �߽��������㲻���Ǻ��ĵ�
		  int flag = 0; // ��flag=0����õ㲻�Ǻ��ĵ㣬��flag=1����õ�Ϊ���ĵ� 
		  for (int j = 0; j<kernel_point.size(); j++) {
			  if (i == kernel_point[j]) {
				  flag = 1;
				  break;
			  }
		  }
		  if (flag == 0) {
			  // �ж��Ǳ߽�㻹�������� 
			  int flag2 = 0; // ��flag=0����õ�Ϊ�߽�㣬��flag=1����õ�λ������
			  for (int j = 0; j<kernel_point.size(); j++) {
				  int s = kernel_point[j]; // ��ǵ�j�����ĵ���point[][]�е�λ�ã�������� 
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
				  // ����������
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
		  vector<int> record; // ����ÿһ���㽨��һ��record������mid����
		  record.push_back(x);
		  for (int j = i + 1; j<kernel_point.size(); j++) {
			  int y = kernel_point[j];
			  if (pow(point[x][0] - point[y][0], 2) - pow(point[x][1] - point[y][1], 2)<pow(Eps, 2)) {
				  record.push_back(y);
			  }
		  }
		  mid.push_back(record);
	  }

	  // �ϲ�vector
	  for (int i = 0; i < mid.size(); i++) { // ����mid�е�ÿһ�� 
		  // �жϸ����Ƿ��Ѿ���ӽ�ǰ���ĳһ���� 
		  if (mid[i][0] == -1) {
			  continue;
		  }
		  // ���û�б��жϹ� 
		  for (int j = 0; j < mid[i].size(); j++) { // �ж����е�ÿһ��ֵ 
			  // ��ÿһ��ֵ�ж����������Ƿ����
			  for (int x = i + 1; x < mid.size(); x++) { // ����֮���ÿһ�� 
				  if (mid[x][0] == -1) {
					  continue;
				  }
				  for (int y = 0; y < mid[x].size(); y++) {
					  if (mid[i][j] == mid[x][y]) {
						  // �����һ����Ԫ�أ�Ӧ�÷���һ��vector�У�����ѭ�������precluster��ͬʱ�ø�vector������Ԫ��ֵΪ-1
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
	  // ɾ��cluster�е��ظ�Ԫ��
	  for (int i = 0; i<cluster.size(); i++) { // ����ÿһ�� 
		  for (int j = 0; j<cluster[i].size(); j++) {
			  for (int n = j + 1; n<cluster[i].size(); n++) {
				  if (cluster[i][j] == cluster[i][n]) {
					  cluster[i].erase(cluster[i].begin() + n);
					  n--;
				  }
			  }
		  }
	  }

	  // ���ˣ�cluster�б����˸����أ�ÿ�������е��Ӧ��point[][]�е�λ��
	  // ��ÿ���߽��ָ�ɵ�һ����֮������ĺ��ĵ�Ĵ���
	  for (int i = 0; i<border_point.size(); i++) { // ����ÿһ���߽�� 
		  int x = border_point[i];
		  for (int j = 0; j<cluster.size(); j++) { // ���ÿһ���أ��жϱ߽�����ĸ����еĺ��ĵ���������߽����뵽��һ�����ĵ���ֵĴ��� 
			  int flag = 0; // flag=0��ʾû��ƥ����flag=1��ʾ�Ѿ�ƥ�䣬�˳�ѭ�� 
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







