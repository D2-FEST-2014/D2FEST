#include <cmath>
#include "stdafx.h"
#include "heesoo.h"
#include "BlobLabeling.h"
#include "qwer.h"
#include <stdio.h>                
#include <cv.h>
#include <highgui.h>
#include <time.h>
#include <vector>
#include <ctype.h>
//opencv_haartraining_engined.lib
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
using namespace std;
using namespace cv;


//char *addr = "192.168.43.189";
char *addr = "127.0.0.1";
int port = 9999;
WSADATA wsaData;
SOCKET clientSock;
sockaddr_in default_destination;
int default_destination_size;
bool default_destination_set;


void gaussian(IplImage* curr);
void MedianFilter(IplImage* Input, IplImage* Output);
void detectskin(IplImage* img);
CvBGStatModel* gmmModel;
IplImage* gmmframe;

int learnCnt = 20;
int frameCnt = 0 ;

void main()
{
	CvCapture *capture;
	IplImage * Image;
	UDPClient *client = new UDPClient(addr, port);
	int Dang = 0;
	capture = cvCaptureFromFile("[mix]test.mp4");  
	IplImage * median;
	IplImage * gray;
	IplImage * sendimage;
	IplImage * faceimage;
	IplImage * fa_gr_image;
	IplImage * mosaimage;
	IplImage * mosaimage1;
	IplImage *canny;
	IplImage * out1;

	IplImage *send3image;

	double a, b, x0, y0,busx=0.0,bus=0.0,k1,k2,angle1,temp;
	float *line, rho, theta;
	CvMemStorage *storage;
	CvSeq *lines = 0;
	CvPoint *point, pt1, pt2;

	while(1)  
	{
		bool movecheck = false;

		char endkey;
		Image=cvQueryFrame(capture); 
		if(!Image)
			break;
		send3image = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 3);
		mosaimage = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 3);
		mosaimage1 = cvCreateImage(cvSize(1,1), IPL_DEPTH_8U, 3);
		faceimage = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 3);
		fa_gr_image = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 1);
		gray = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 1);  
		sendimage = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 1);
		canny = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 1);
		IplImage *realhough = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 3);
		IplImage *probhough = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 3);


		cvCvtColor(Image, sendimage, CV_RGB2GRAY);
		cvCvtColor(Image, gray, CV_RGB2GRAY);

		cvCopy(Image,faceimage);
		cvCopy(Image,mosaimage);
		cvCopy(Image,send3image);

		
		gaussian(gray);  //open SW



		cvSmooth(gmmframe,gmmframe,CV_MEDIAN,5);
		cvMorphologyEx(gmmframe,gmmframe,0,0,CV_MOP_CLOSE,2);

		detectskin(faceimage);	//open SW






		cvCanny(gray, canny,60,70,3);


		storage = cvCreateMemStorage (0); 

		lines = cvHoughLines2 (canny, storage, CV_HOUGH_STANDARD, 1, CV_PI/180, 80, 0,0);

		for(int i = 0; i< MIN(lines->total, 100); i++)
		{
			line =(float*) cvGetSeqElem(lines, i);
			rho = line[0];
			theta = line[1];
			a = cos (theta);
			b = sin (theta);
			x0 = a*rho;
			y0 = b*rho;
			pt1.x =cvRound(x0 + 500 * (-b));
			pt1.y =cvRound(y0 + 500 * (a));
			pt2.x =cvRound(x0 - 500 * (-b));
			pt2.y =cvRound(y0 - 500 * (a));
			k1=(pt1.x-pt2.x);
			k2=pt1.y-pt2.y;

			angle1=k1/k2;
			if(0.08>angle1)
			{
				if(pt1.x>170 &&pt1.x<gmmframe->width)
				{
					cvLine (realhough, pt1,pt2,CV_RGB(255, 0,0), 1, 8,0);
					temp=pt1.x;
				}
			}
		}

		for(int i=0; i<gmmframe->height; i++)
		{
			for(int j = 0; j<gmmframe->width; j++)
			{
				if(j>temp)
					gmmframe->imageData[j+gmmframe->widthStep*i]=0;
			}
		}




		if(frameCnt>25)
		{

			CBlobLabeling blob;   //open SW(labeling)
			blob.SetParam(gmmframe, 100 );   
			blob.DoLabeling();   
			if(blob.m_nBlobs>0)
			{
				printf("%d", blob.m_nBlobs);
				printf("명이 있습니다 \n");
			}
			int face_size = blob.m_nBlobs;
			int face_x1[100],face_x2[100];
			int face_y1[100],face_y2[100];

			for(int i=0; i< blob.m_nBlobs; i++)   
			{
				cvDrawRect(Image,cvPoint(blob.m_recBlobs[i].x,blob.m_recBlobs[i].y), 
					cvPoint(blob.m_recBlobs[i].x+blob.m_recBlobs[i].width, blob.m_recBlobs[i].y+blob.m_recBlobs[i].height),
					CV_RGB(255,0,0),2); 
				if((blob.m_recBlobs[i].width>=(double)Image->width/3)&&(blob.m_recBlobs[i].height>=(double)Image->height*0.75))
					movecheck = true;
				face_x1[i] = blob.m_recBlobs[i].x;
				face_x2[i] = blob.m_recBlobs[i].x+blob.m_recBlobs[i].width;
				face_y1[i] = blob.m_recBlobs[i].y;
				face_y2[i] = blob.m_recBlobs[i].y+blob.m_recBlobs[i].height/3;
			}
			if(movecheck == true)
				continue;
			else{

			int mosa_cnt = 0;
			double mosa_var_r = 0;
			double mosa_var_g = 0;
			double mosa_var_b = 0;

			for(int i=0; i<faceimage->height; i++){
				for(int j=0; j<faceimage->width; j++){
					bool face_t = false;
					if(faceimage->imageData[(i*faceimage->widthStep)+j*3+0]!=0 
						|| faceimage->imageData[(i*faceimage->widthStep)+j*3+1]!=0 
						|| faceimage->imageData[(i*faceimage->widthStep)+j*3+2]!=0){
							for(int t=0; t<blob.m_nBlobs; t++){
								if(j>=face_x1[t] && j<=face_x2[t] && i>=face_y1[t] && i<=face_y2[t]){
									face_t = true;
								}
							}
					}


					if(face_t == false){
						faceimage->imageData[(i*faceimage->widthStep)+j*3+2] = 0;
						faceimage->imageData[(i*faceimage->widthStep)+j*3+1] = 0;
						faceimage->imageData[(i*faceimage->widthStep)+j*3+0] = 0;
					}

				}
			}

			cvCvtColor(faceimage, fa_gr_image, CV_RGB2GRAY);

			CBlobLabeling blob1;   
			blob1.SetParam(fa_gr_image, 3);   
			blob1.DoLabeling();

			for(int i=0; i<mosaimage->height; i++){
				for(int j=0; j<mosaimage->width; j++){
					for(int t=0; t<blob1.m_nBlobs; t++){
						int mosa_wid = blob1.m_recBlobs[t].width*0.5;
						int mosa_hei = blob1.m_recBlobs[t].height*0.5;
						if(j>blob1.m_recBlobs[t].x-mosa_wid && j<(blob1.m_recBlobs[t].width+blob1.m_recBlobs[t].x)+mosa_wid
							&& i>blob1.m_recBlobs[t].y -mosa_hei && i<(blob1.m_recBlobs[t].height+blob1.m_recBlobs[t].y)+mosa_hei){
								mosa_var_b += (unsigned char)mosaimage->imageData[(i*mosaimage->widthStep)+j*3+0];
								mosa_var_g += (unsigned char)mosaimage->imageData[(i*mosaimage->widthStep)+j*3+1];
								mosa_var_r += (unsigned char)mosaimage->imageData[(i*mosaimage->widthStep)+j*3+2];
								mosa_cnt++;
						}
					}
				}
			}

			mosa_var_b /= mosa_cnt;
			mosa_var_g /= mosa_cnt;
			mosa_var_r /= mosa_cnt;

			for(int i=0; i<faceimage->height; i++){
				for(int j=0; j<faceimage->width; j++){
					for(int t=0; t<blob1.m_nBlobs; t++){
						int mosa_wid = blob1.m_recBlobs[t].width*0.5;
						int mosa_hei = blob1.m_recBlobs[t].height*0.5;
						if(j>blob1.m_recBlobs[t].x-mosa_wid && j<(blob1.m_recBlobs[t].width+blob1.m_recBlobs[t].x)+mosa_wid
							&& i>blob1.m_recBlobs[t].y -mosa_hei && i<(blob1.m_recBlobs[t].height+blob1.m_recBlobs[t].y)+mosa_hei){
								mosaimage->imageData[(i*mosaimage->widthStep)+j*3+2] = (unsigned char)(int)mosa_var_r;
								mosaimage->imageData[(i*mosaimage->widthStep)+j*3+1] = (unsigned char)(int)mosa_var_g;
								mosaimage->imageData[(i*mosaimage->widthStep)+j*3+0] = (unsigned char)(int)mosa_var_b;
						}
					}
				}
			}

			for(int i=0; i<send3image->height; i++){
				for(int j=0; j<send3image->width; j++){
					if(send3image->imageData[j*3+0+i*send3image->widthStep]==0)
						send3image->imageData[j*3+0+i*send3image->widthStep]+=1;
					if(send3image->imageData[j*3+1+i*send3image->widthStep]==0)
						send3image->imageData[j*3+1+i*send3image->widthStep]+=1;
					if(send3image->imageData[j*3+2+i*send3image->widthStep]==0)
						send3image->imageData[j*3+2+i*send3image->widthStep]+=1;
				}
			}

			char *buff = new char[30000];

			int snsz = 30000;

			int x = strlen(send3image->imageData)/snsz+2;
			int y = strlen(send3image->imageData)%snsz;
			char end[10] = "end";
			int cnt1 = 0;

			for(int i = 0; i<x; i++)
			{
				int snsz1;
				if(i < x-2)
					snsz1 = snsz;
				else
					snsz1 = y;

				if(i == x-1)
					strcpy(buff,end);
				else
					strncpy(buff,send3image->imageData+(i*snsz),snsz1);

				int result = client->sendData((char*)(&buff[0]), strlen(buff));
				cnt1++;
				if(result < 0)
					cout<<"전송 실패"<<endl;
			}

			for(int i=0; i<mosaimage->height; i++){
				for(int j=0; j<mosaimage->width; j++){
					if(mosaimage->imageData[j*3+0+i*mosaimage->widthStep]==0)
						mosaimage->imageData[j*3+0+i*mosaimage->widthStep]+=1;
					if(mosaimage->imageData[j*3+1+i*mosaimage->widthStep]==0)
						mosaimage->imageData[j*3+1+i*mosaimage->widthStep]+=1;
					if(mosaimage->imageData[j*3+2+i*mosaimage->widthStep]==0)
						mosaimage->imageData[j*3+2+i*mosaimage->widthStep]+=1;
				}
			}

			x = strlen(mosaimage->imageData)/snsz+2;
			y = strlen(mosaimage->imageData)%snsz;

			cnt1 = 0;

			for(int i = 0; i<x; i++)
			{
				int snsz1;
				if(i < x-2)
					snsz1 = snsz;
				else
					snsz1 = y;

				if(i == x-1)
					strcpy(buff,end);
				else
					strncpy(buff,mosaimage->imageData+(i*snsz),snsz1);

				int result = client->sendData((char*)(&buff[0]), strlen(buff));
				cnt1++;
				if(result < 0)
					cout<<"전송 실패"<<endl;

			}

			if(blob.m_nBlobs == 0 )
			{
				PlaySound(0,0,0);
				Dang = 0;
			}

			else if(blob.m_nBlobs!=0 && Dang == 0)
			{
				PlaySound("wav.wav",NULL,SND_ASYNC | SND_LOOP);
				Dang = 1;
			}
			}

			cvShowImage("Image",Image);  
			cvShowImage("back",gmmframe);
			cvShowImage("face",faceimage);
			cvShowImage("mosaic",mosaimage);
			cvShowImage("Canny", canny );
			cvShowImage("Real Hough Transform", realhough);

		}

		cvReleaseImage(&gmmframe);
		cvReleaseImage(&faceimage);
		cvReleaseImage(&mosaimage);
		cvReleaseImage(&canny);
		cvReleaseImage(&realhough);
		cvReleaseImage(&send3image);
		cvReleaseImage(&fa_gr_image);

		endkey = cvWaitKey(1);
		if(endkey ==27)
			break;

	}
		
}
void detectskin(IplImage* img)
{
	unsigned char r=0, g=0, b=0;  // 0~255 까지의 값을 받는 각각의 변수
	unsigned char y=0, cb=0, cr=0;

	for(int i=0; i<img->height; i++)  //이미지 세로
	{
		for(int j=0; j<img->width; j++)  //이미지 가로
		{
			r=(unsigned char)img->imageData[(i*img->widthStep)+j*3+2]; // Red값
			g=(unsigned char)img->imageData[(i*img->widthStep)+j*3+1]; // Green값
			b=(unsigned char)img->imageData[(i*img->widthStep)+j*3+0]; // Blue값

			y=(unsigned char)(0.2999*r+0.587*g+0.114*b);
			cb=(unsigned char)(-0.1687*r-0.3313*g+0.5*b+128.0);
			cr=(unsigned char)(0.5*r-0.4187*g-0.0813*b+128.0);

			if((r > 120) && (g > 80) && (b > 50) && (cb>=77) && (cb<=127) && (cr>=133) && (cr<=173))  //살색검출 (황인 기준)
			{
				img->imageData[(i*img->widthStep)+j*3+2] = 255;
				img->imageData[(i*img->widthStep)+j*3+1] = 255;
				img->imageData[(i*img->widthStep)+j*3+0] = 255;
			}
			else //살색이 아닐경우 검은색으로 변환
			{
				img->imageData[(i*img->widthStep)+j*3+2] = 0;
				img->imageData[(i*img->widthStep)+j*3+1] = 0;
				img->imageData[(i*img->widthStep)+j*3+0] = 0;
			}
		}
	}
}


void gaussian(IplImage* frame)
{
	if(gmmModel == NULL)  
	{   
		gmmModel = cvCreateGaussianBGModel(frame, NULL);  
	}
	else
	{

		if(frameCnt++ <= learnCnt)
		{
			cvUpdateBGStatModel(frame, gmmModel, -1);  
		}
		else 
		{
			cvUpdateBGStatModel(frame, gmmModel, 0);
		}

	}

	if(gmmframe)
	{
		cvCopy(gmmModel->foreground, gmmframe, NULL);

	}

	else
	{
		gmmframe = cvCloneImage(gmmModel->foreground);


	}
}



void MedianFilter(IplImage* Input, IplImage* Output)
{
	IplImage* temp;
	temp = cvCreateImage(cvGetSize(Input), IPL_DEPTH_8U, 1);
	int n = Input->width;
	int m = Input->height;
	int withstep = Input->widthStep;
	int min = 0;

	for(int j=0; j<m;j++)
	{
		for(int i=0; i<n; i++)
		{
			int w = 0;
			for(int u = j-2; u<j+3; u++){
				for(int v = i-2; v<i+3; v++){
					if(v<0 || u<0){
						continue;
					}
					else{
						temp->imageData[w] = Input->imageData[v+u*withstep];
						w++;
					}
				}
			}
			for(int k = 0; k<w; k++){
				min = k;
				for(int t = k ; t<w; t++){
					if(temp->imageData[t]<temp->imageData[min]){
						min = t;
					}
				}
				temp->imageData[w] = temp->imageData[min];
				temp->imageData[min] = temp->imageData[k];
				temp->imageData[k] = temp->imageData[w];
			}
			Output->imageData[i+j*withstep] = temp->imageData[(w/2)];
		}
	}
}

UDPClient::UDPClient(char* host_addr, int host_port){
	int result = WSAStartup(MAKEWORD(2, 2), (LPWSADATA) &wsaData);
	if(result < 0){
		throw "WSAStartup ERROR.";
		return;
	}

	clientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(clientSock == INVALID_SOCKET){
		throw "Invalid Socket ERROR.";
		return;
	}

	sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = inet_addr("localhost");
	local.sin_port = 0;
	bind(clientSock, (sockaddr*)&local, sizeof(local));


	default_destination.sin_family = AF_INET;
	default_destination.sin_addr.s_addr = inet_addr(host_addr);
	default_destination.sin_port = htons(host_port);
	default_destination_size = sizeof(default_destination);

	default_destination_set = true;
}

int UDPClient::sendData(char* buffer, int len){
	if(!default_destination_set){
		printf("No default set.\n");
		return -1;
	}

	return sendto(clientSock, buffer, len, 0,
		(sockaddr*)&default_destination, default_destination_size);
}