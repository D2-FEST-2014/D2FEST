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
IplImage *img1 = cvCreateImage(cvSize(240,120), IPL_DEPTH_8U, 1);
IplImage *img2 = cvCreateImage(cvSize(500,400), IPL_DEPTH_8U, 1);
int UDPMAX = 30000; // max buffer size
int x=30000;
int port = 9999; // listen port number
 
int main(){
    cout<<"=== VIDEO RECEIVER ==="<<endl;
	

    UDPClient *client = new UDPClient(port);

    vector<uchar> videoBuffer;
    CvVideoWriter* VideoOut = NULL;
    while(1){
 		char *buff = new char[UDPMAX];

        //read data
        int result = client->receiveData(buff, x);
        if(result < 0){
            cout<<"실패"<<endl;
            continue;
        }

        cout<<"recive 성공"<<endl;

		strcpy(img1->imageData,buff);


		if(VideoOut == NULL)
            VideoOut = cvCreateVideoWriter( "save.avi",CV_FOURCC('D', 'I', 'V', 'X'), 20, cvGetSize(img1), 0);
		cvWriteFrame(VideoOut, img1);

		 cvShowImage("UDP Video Receiver1", img1);//여기서 뻣는다

        cvWaitKey(100);
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
