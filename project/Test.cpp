#include "stdafx.h"
#include "heesoo.h"
#include "BlobLabeling.h"
#include "UDPClient.h"
#include <stdio.h>                
#include <cv.h>                     
#include <highgui.h>
#include <time.h>
#include <vector>
#include <ctype.h>

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
using namespace std;
using namespace cv;


char *addr = "192.168.0.83";
int port = 9999;
WSADATA wsaData;
SOCKET clientSock;
sockaddr_in default_destination;
int default_destination_size;
bool default_destination_set;


void gaussian(IplImage* curr);
void MedianFilter(IplImage* Input, IplImage* Output);
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
	IplImage *sendgray1;
	sendgray1 = cvCreateImage(cvSize(240,120),IPL_DEPTH_8U,1);
	while(1)  
	{
		Image=cvQueryFrame(capture); 

		gray = cvCreateImage(cvGetSize(Image), IPL_DEPTH_8U, 1);  
		cvCvtColor(Image, gray, CV_RGB2GRAY);

		cvResize(gray, sendgray1, CV_INTER_LINEAR);

		gaussian(gray);  
		cvSmooth(gmmframe,gmmframe,CV_MEDIAN,5);
		cvMorphologyEx(gmmframe,gmmframe,0,0,CV_MOP_CLOSE,2);

		for(int i=0; i<sendgray1->height; i++){
			for(int j=0; j<sendgray1->width; j++){
				if(sendgray1->imageData[j+i*sendgray1->widthStep]==0)
					sendgray1->imageData[j+i*sendgray1->widthStep]+=1;
			}
		}
		char *buff = new char[strlen(sendgray1->imageData)];
		strcpy(buff, sendgray1->imageData);

		int result = client->sendData((char*)(&buff[0]), strlen(sendgray1->imageData));
		if(result < 0)
			cout<<"전송 실패"<<endl;
		else
			cout<<"전송 성공"<<endl;

		if(frameCnt>25)
		{

			CBlobLabeling blob;   
			blob.SetParam(gmmframe, 100 );   
			blob.DoLabeling();   
			if(blob.m_nBlobs>0)
			{
				printf("%d", blob.m_nBlobs);
				printf("명이 있습니다 \n");
			}


			for(int i=0; i< blob.m_nBlobs; i++)   
			{
				cvDrawRect(Image,cvPoint(blob.m_recBlobs[i].x,blob.m_recBlobs[i].y), 
					cvPoint(blob.m_recBlobs[i].x+blob.m_recBlobs[i].width, blob.m_recBlobs[i].y+blob.m_recBlobs[i].height),
					CV_RGB(255,0,0),2); 
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

			cvShowImage("Image",Image);  
			cvShowImage("back",gmmframe);
		}
		cvWaitKey(1);
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