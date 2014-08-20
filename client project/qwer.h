#include "highgui.h"
#include "cv.h"
#define CV_H
#include <iostream>
#include <conio.h>

using namespace std;
using namespace cv;

class  UDPClient
{
public:
   UDPClient(char *,int );
   int sendData(char*, int);
};