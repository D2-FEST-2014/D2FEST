#include "highgui.h"
#include "cv.h"
#define CV_H
#include <iostream>
#include <conio.h>
#include "UDPClient.h"
#include <Windows.h>

using namespace std;
using namespace cv;
typedef struct CvVideoWriter CvVideoWriter;
WSADATA wsaData;
SOCKET clientSock;
sockaddr_in default_destination, last_address;
int default_destination_size;
bool default_destination_set;
IplImage *img = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);
IplImage *img1[100];
IplImage *img4[100];


int UDPMAX = 31000; // max buffer size
int x=31000;
int port = 9999; // listen port number

int main(){
	for(int i=0; i<100; i++){
		img1[i]= cvCreateImage(cvSize(368,208), IPL_DEPTH_8U, 3);
		for(int h=0; h<img1[i]->height; h++){
			for(int j=0; j<img1[i]->width; j++){
				img1[i]->imageData[j*3+0+h*img1[i]->widthStep]=0;
				img1[i]->imageData[j*3+1+h*img1[i]->widthStep]=0;
				img1[i]->imageData[j*3+2+h*img1[i]->widthStep]=0;
			}
		}
	}

	for(int i=0; i<100; i++){
		img4[i]= cvCreateImage(cvSize(368,208), IPL_DEPTH_8U, 3);
		for(int h=0; h<img4[i]->height; h++){
			for(int j=0; j<img4[i]->width; j++){
				img4[i]->imageData[j*3+0+h*img4[i]->widthStep]=0;
				img4[i]->imageData[j*3+1+h*img4[i]->widthStep]=0;
				img4[i]->imageData[j*3+2+h*img4[i]->widthStep]=0;
			}
		}
	}


	IplImage *img2 = cvCreateImage(cvSize(368,208), IPL_DEPTH_8U, 3);
	IplImage *img3 = cvCreateImage(cvSize(368,208), IPL_DEPTH_8U, 3);
	UDPClient *client = new UDPClient(port);

	vector<uchar> videoBuffer;
	CvVideoWriter* VideoOut = NULL;
	CvVideoWriter* VideoOut1 = NULL;
	int n = 30000;

	while(1){
		int count = 0;
		char *buff = new char[UDPMAX];
		char *buff1 = new char[UDPMAX];

		int cnt = 0;
		while(1){

			int result = client->receiveData(buff, x);
			if(result < 0){
				cout<<"실패"<<endl;
				continue;
			}

			if(buff[0] == 'e'&& buff[1] == 'n' && buff[2] == 'd')
				break;
			else
				strncpy(img1[count++]->imageData,buff,30000);

		}
		count=0;
		while(1){

			int result = client->receiveData(buff1, x);
			if(result < 0){
				cout<<"실패"<<endl;
				continue;
			}

			if(buff1[0] == 'e'&& buff1[1] == 'n' && buff1[2] == 'd')
				break;
			else
				strncpy(img4[count++]->imageData,buff1,30000);

		}




		int i1=0,j1 = 0;
		int k=0;
		for(int i=0;i<img2->height;i++){
			for(int j=0; j<img2->width; j++){
				if(img1[k]->imageData[(j1*3+0)+(i1)*img2->widthStep] == 0){
					k++;
					i1 = 0;
					j1 = 0;
				}
				img2->imageData[j*3+0+i*img2->widthStep] = img1[k]->imageData[(j1*3+0)+(i1)*img2->widthStep];
				img2->imageData[j*3+1+i*img2->widthStep] = img1[k]->imageData[(j1*3+1)+(i1)*img2->widthStep];
				img2->imageData[j*3+2+i*img2->widthStep] = img1[k]->imageData[(j1*3+2)+(i1)*img2->widthStep];
				j1++;
				if(j1==img1[k]->width)
				{
					j1=0;
					i1++;
				}
			}

		}
		

		

		
		i1=0,j1 = 0;
		k=0;
		for(int i=0;i<img3->height;i++){
			for(int j=0; j<img3->width; j++){
				if(img4[k]->imageData[(j1*3+0)+(i1)*img3->widthStep] == 0){
					k++;
					i1 = 0;
					j1 = 0;
				}
				img3->imageData[j*3+0+i*img3->widthStep] = img4[k]->imageData[(j1*3+0)+(i1)*img3->widthStep];
				img3->imageData[j*3+1+i*img3->widthStep] = img4[k]->imageData[(j1*3+1)+(i1)*img3->widthStep];
				img3->imageData[j*3+2+i*img3->widthStep] = img4[k]->imageData[(j1*3+2)+(i1)*img3->widthStep];
				j1++;
				if(j1==img1[k]->width)
				{
					j1=0;
					i1++;
				}
			}

		}
		
		
		if(VideoOut == NULL){
			VideoOut = cvCreateVideoWriter( "save.avi",CV_FOURCC('D', 'I', 'V', 'X'), 20, cvGetSize(img2), 1);
			VideoOut1 = cvCreateVideoWriter( "save1.avi",CV_FOURCC('D', 'I', 'V', 'X'), 20, cvGetSize(img3), 1);
		}
		cvWriteFrame(VideoOut, img2);
		cvWriteFrame(VideoOut1, img3);
		cvSaveImage("Receive.jpg",img2);

		cvShowImage("UDP Video Receiver1", img2);
		cvShowImage("UDP Video Receiver2", img3);

		cvWaitKey(1);
	}
	cvReleaseVideoWriter(&VideoOut);
}

UDPClient::UDPClient(int local_port){
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
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(local_port);
	result = bind(clientSock, (sockaddr*)&local, sizeof(local));
	if(result < 0){
		throw "Socket Bind ERROR";
		return;
	}

	default_destination_set = false;
}


int UDPClient::receiveData(char* buffer, int len){
	int received = 0;

	sockaddr_in source;
	int source_size = sizeof(source);

	if((received = recvfrom(clientSock, buffer, len, 0,
		(sockaddr*) &source, &source_size)) == SOCKET_ERROR){
			printf("ERROR #%d\n", WSAGetLastError());
			return SOCKET_ERROR;
	}

	last_address = source;
	return received;
}
