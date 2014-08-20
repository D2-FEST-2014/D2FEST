
#include "StdAfx.h"
#include "BlobLabeling.h"

#define _DEF_MAX_BLOBS		10000
#define _DEF_MAX_LABEL		  100

CBlobLabeling::CBlobLabeling(void)  // 생성자
{
	m_nThreshold	= 0;
	m_nBlobs		= _DEF_MAX_BLOBS;
	m_Image			= NULL;
	                                        // 레이블링에 필요한 변수 초기값 세팅
	m_recBlobs		= NULL;
	m_intBlobs		= NULL;	
}

CBlobLabeling::~CBlobLabeling(void)   // 소멸자
{
	if( m_Image != NULL )	cvReleaseImage( &m_Image );	

	if( m_recBlobs != NULL )
	{
		delete m_recBlobs;
		m_recBlobs = NULL;           // 이미지를 반환하고 동적할당한 변수 삭제 및 값을 NULL로 설정
	}

	if( m_intBlobs != NULL )
	{
		delete m_intBlobs;
		m_intBlobs = NULL;
	}
}

void CBlobLabeling::SetParam(IplImage* image, int nThreshold)   // 변수 값 설정함수
{
	if( m_recBlobs != NULL )   // 각 레이블의 정보가 이미 있을때
	{
		delete m_recBlobs;  // 동적할당된 BLOB 삭제

		m_recBlobs	= NULL;            // 초기값으로 설정
		m_nBlobs	= _DEF_MAX_BLOBS;
	}

	if( m_intBlobs != NULL )   // 각 레이블의 인덱스가 이미 있을때
	{
		delete m_intBlobs;   // 동적할당된 BLOB 삭제

		m_intBlobs	= NULL;             // 초기값으로 설정
		m_nBlobs	= _DEF_MAX_BLOBS;
	}

	if( m_Image != NULL )	cvReleaseImage( &m_Image );   // 레이블링에 필요한 이미지가 이미 세팅되있을 경우 이미지 반환

	m_Image			= cvCloneImage( image );   // 받아온 이미지를 그대로 복사

	m_nThreshold	= nThreshold;   // 받아온 Threshold 값을 레이블링 클래스의 변수로 복사
}

void CBlobLabeling::DoLabeling()  // 레이블링 시작 함수
{
	m_nBlobs = Labeling(m_Image, m_nThreshold);   // 이미지와 Threshold값을 넘겨주어 인식된 객체의 개수를 리턴 받음.
}

int CBlobLabeling::Labeling(IplImage* image, int nThreshold)  // 레이블링 함수
{
	if( image->nChannels != 1 ) 	
		return 0;   // 이미지가 흑백이 아닐때 함수 종료.

	int nNumber;  // 객체수에 관한 변수
	
	int nWidth	= image->width;   // 이미지의 너비와 높이를 복사
	int nHeight = image->height;
	
	unsigned char* tmpBuf = new unsigned char [nWidth * nHeight];  // 이미지의 값을 1차원으로 계산하기 위한 동적할당.

	int i,j;

	for(j=0;j<nHeight;j++)      // 이미지의 너비와 높이만큼 for문 반복
	for(i=0;i<nWidth ;i++)
		tmpBuf[j*nWidth+i] = (unsigned char)image->imageData[j*image->widthStep+i];   // 이미지 데이터 값을 temp 배열에 복사
	
	InitvPoint(nWidth, nHeight);   // 레이블링을 위한 포인트 초기화

	nNumber = _Labeling(tmpBuf, nWidth, nHeight, nThreshold);  // 레이블링

	DeletevPoint(); // 포인트 메모리 해제

	if( nNumber != _DEF_MAX_BLOBS )		m_recBlobs = new CvRect [nNumber]; // 객체의 개수가 초기값이 아닐때 객체의 개수만큼 동적할당.

	if( nNumber != _DEF_MAX_BLOBS )		m_intBlobs = new int [nNumber];

	if( nNumber != 0 )	DetectLabelingRegion(nNumber, tmpBuf, nWidth, nHeight);  // 객체의 개수가 0이상 일경우 사각형을 그리기위한 좌표 탐색.

	for(j=0;j<nHeight;j++)
	for(i=0;i<nWidth ;i++)
		image->imageData[j*image->widthStep+i] = tmpBuf[j*nWidth+i];  // 임시 버퍼의 값을 원래 이미지에 복사

	delete tmpBuf;   // temp 버퍼 삭제
	return nNumber;  // 객체 개수 리턴
}

// m_vPoint 초기화 함수
void CBlobLabeling::InitvPoint(int nWidth, int nHeight)
{
	int nX, nY;   // for문을 사용하기 위한 변수

	m_vPoint = new Visited [nWidth * nHeight];  // 1차원으로 계산하기 위하여 이미지의 총 픽셀에 해당하는 값으로 동적할당.

	for(nY = 0; nY < nHeight; nY++)       // 이미지의 크기만큼 for문 반복
	{
		for(nX = 0; nX < nWidth; nX++)
		{
			m_vPoint[nY * nWidth + nX].bVisitedFlag		= FALSE;   // 2차원으로 설정된 각 이미지 픽셀에 해당하는 m_vPoint값에 변수 설정.
			m_vPoint[nY * nWidth + nX].ptReturnPoint.x	= nX;
			m_vPoint[nY * nWidth + nX].ptReturnPoint.y	= nY;
		}
	}
}

void CBlobLabeling::DeletevPoint()
{
	delete m_vPoint;
}

// Size가 nWidth이고 nHeight인 DataBuf에서 
// nThreshold보다 작은 영역을 제외한 나머지를 blob으로 획득
int CBlobLabeling::_Labeling(unsigned char *DataBuf, int nWidth, int nHeight, int nThreshold)
{
	int Index = 0, num = 0;
	int nX, nY, k, l;                   // 레이블링에 필요한 좌표 변수 값 선언.
	int StartX , StartY, EndX , EndY;
	
	for(nY = 0; nY < nHeight; nY++)         // 이미지의 픽셀수 만큼 for문 반복
	{
		for(nX = 0; nX < nWidth; nX++)
		{
			if(DataBuf[nY * nWidth + nX] == 255)  // 2차원 이미지의 픽셀값이 복사되어있는 1차원배열에서 한 픽셀의 값이 255일때 즉 객체로 판단될떄
			{
				num++;   // 객체값 설정

				DataBuf[nY * nWidth + nX] = num;  // 해당 픽셀의 값을 num으로 바꿔줌.
				
				StartX = nX, StartY = nY, EndX = nX, EndY= nY;  // 현재 탐색중인 X,Y좌표와 탐색 종료 좌표 지정. 

				__NRFIndNeighbor(DataBuf, nWidth, nHeight, nX, nY, &StartX, &StartY, &EndX, &EndY);  // 객체로 인식된 픽셀과 인접 픽셀값을 설정하는 함수

				if(__Area(DataBuf, StartX, StartY, EndX, EndY, nWidth, num) < nThreshold)  //  객체의 픽셀수가 Threshold값보다 작은 경우
				{
		 			for(k = StartY; k <= EndY; k++)
					{
						for(l = StartX; l <= EndX; l++)         // Threshold 값보다 작은 경우 객체를 삭제
						{
							if(DataBuf[k * nWidth + l] == num)
								DataBuf[k * nWidth + l] = 0;     // 객체의 번호를 다시 0으로 바꿔줌.
						}
					}
					--num;  // 객체 개수 감소

					if(num > 250)   // 객체 개수가 250이상인경우
						return  0;  // 함수 종료
				}
			}
		}
	}

	return num;	// 객체 개수 반환.
}


void CBlobLabeling::DetectLabelingRegion(int nLabelNumber, unsigned char *DataBuf, int nWidth, int nHeight)  // Blob labeling해서 얻어진 결과의 rec을 얻어냄 
{
	int nX, nY;
	int nLabelIndex ;

	bool bFirstFlag[255] = {FALSE,};
	
	for(nY = 1; nY < nHeight - 1; nY++)    // 이미지 탐색
	{
		for(nX = 1; nX < nWidth - 1; nX++)
		{
			nLabelIndex = DataBuf[nY * nWidth + nX];  // 픽셀값을 Labelindex에 저장

			if(nLabelIndex != 0) // 인덱스의 값이 존재하여 객체일때
			{
				if(bFirstFlag[nLabelIndex] == FALSE) // 초기상태
				{
					m_recBlobs[nLabelIndex-1].x			= nX;   // 좌표 설정
					m_recBlobs[nLabelIndex-1].y			= nY;
					m_recBlobs[nLabelIndex-1].width		= 0;
					m_recBlobs[nLabelIndex-1].height	= 0;
				
					bFirstFlag[nLabelIndex] = TRUE;  // flag값을 true로 바꿔줌.
				}
				else
				{
					int left	= m_recBlobs[nLabelIndex-1].x;
					int right	= left + m_recBlobs[nLabelIndex-1].width;
					int top		= m_recBlobs[nLabelIndex-1].y;
					int bottom	= top + m_recBlobs[nLabelIndex-1].height;

					if( left   >= nX )	left	= nX;                      // 객체를 사각형으로 표시하기위해 꼭지점에 해당하는 부분의 좌표를 구함.
					if( right  <= nX )	right	= nX;
					if( top    >= nY )	top		= nY;
					if( bottom <= nY )	bottom	= nY;

					m_recBlobs[nLabelIndex-1].x			= left;
					m_recBlobs[nLabelIndex-1].y			= top;
					m_recBlobs[nLabelIndex-1].width		= right - left;
					m_recBlobs[nLabelIndex-1].height	= bottom - top;

					m_intBlobs[nLabelIndex-1]			= nLabelIndex;
				}
			}
				
		}
	}
	
}

int CBlobLabeling::__NRFIndNeighbor(unsigned char *DataBuf, int nWidth, int nHeight, int nPosX, int nPosY, int *StartX, int *StartY, int *EndX, int *EndY )
{
	CvPoint CurrentPoint;    // 현재 픽셀의 좌표값을 표시하기 위한 변수
	
	CurrentPoint.x = nPosX;     // 객체로 인식된 픽셀의 x,y값을 복사
	CurrentPoint.y = nPosY;

	m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x].bVisitedFlag    = TRUE;    // 객체로 인식된 픽셀의 Flag값을 TRUE로 바꾸어 객체로 인식할 수 있도록 설정
	m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x].ptReturnPoint.x = nPosX;
	m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x].ptReturnPoint.y = nPosY;
			
	while(1)  // while문을 통해 객체로 인식된 좌표의 인접좌표들의 객체 유무를 계속해서 파악함.
	{
		if( (CurrentPoint.x != 0) && (DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x - 1] == 255) )  // 현재 픽셀이 객체이고, -X 방향의 픽셀또한 객체인 경우
		{
			if( m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x - 1].bVisitedFlag == FALSE ) // -X 방향의 픽셀이 객체이지만, FLAG 값이 FALSE인 경우
			{
				DataBuf[CurrentPoint.y  * nWidth + CurrentPoint.x  - 1]	= DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x]; // 현재 픽셀의 NUM값을 -X방향의 픽셀에 저장. 즉 BLOB번호
				m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x - 1].bVisitedFlag	= TRUE;   // FLAG 값을 TRUE로 바꿔줌.
				m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x - 1].ptReturnPoint	= CurrentPoint;  // -X 방향의 픽셀의 리턴 포인트 설정.
				CurrentPoint.x--; // 픽셀의 값을 감소시켜 현재 픽셀의 값을 -X 방향의 픽셀로 교체
				
				if(CurrentPoint.x <= 0)  // 현재 픽셀의 X좌표값이 0이하인경우
					CurrentPoint.x = 0;  // 현재 픽셀의 X좌표값을 0으로 설정

				if(*StartX >= CurrentPoint.x)  // 탐색중인 픽셀이 현재 픽셀값보다 크거나 같은 경우
					*StartX = CurrentPoint.x;  // 탐색중인 픽셀값을 현재 픽셀값으로 설정.

				continue;
			}
		}

		if( (CurrentPoint.x != nWidth - 1) && (DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x + 1] == 255) ) // 현재 픽셀이 객체이고, +X 방향의 픽셀또한 객체인 경우
		{
			if( m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x + 1].bVisitedFlag == FALSE )  // +X 방향의 픽셀이 객체이지만, FLAG 값이 FALSE인 경우
			{
				DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x + 1] = DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x]; // 현재 픽셀의 NUM값을 +X방향의 픽셀에 저장. 즉 BLOB번호
				m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x + 1].bVisitedFlag	= TRUE;  // FLAG 값을 TRUE로 바꿔줌.
				m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x + 1].ptReturnPoint	= CurrentPoint;   // +X 방향의 픽셀의 리턴 포인트 설정.
				CurrentPoint.x++; // 픽셀의 값을 증가시켜 현재 픽셀의 값을 +X 방향의 픽셀로 교체

				if(CurrentPoint.x >= nWidth - 1)   // 현재 픽셀의 X좌표값이 이미지의 너비보다 크거나 같은 경우
					CurrentPoint.x = nWidth - 1;   // 현재 픽셀의 X좌표값을 이미지의 너비로 설정.
				
				if(*EndX <= CurrentPoint.x)  // 탐색종료 픽셀값 이 현재 픽셀값보다 작거나 같은경우
					*EndX = CurrentPoint.x;  // 탐색종료 픽셀값을 현재 픽셀값으로 설정.

				continue;
			}
		}

		if( (CurrentPoint.y != 0) && (DataBuf[(CurrentPoint.y - 1) * nWidth + CurrentPoint.x] == 255) ) // 현재 픽셀이 객체이고, -Y 방향의 픽셀또한 객체인 경우
		{
			if( m_vPoint[(CurrentPoint.y - 1) * nWidth +  CurrentPoint.x].bVisitedFlag == FALSE ) // -Y 방향의 픽셀이 객체이지만, FLAG 값이 FALSE인 경우
			{
				DataBuf[(CurrentPoint.y - 1) * nWidth + CurrentPoint.x]	= DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x]; // 현재 픽셀의 NUM값을 +Y방향의 픽셀에 저장. 즉 BLOB번호
				m_vPoint[(CurrentPoint.y - 1) * nWidth +  CurrentPoint.x].bVisitedFlag	= TRUE; // FLAG 값을 TRUE로 바꿔줌.
				m_vPoint[(CurrentPoint.y - 1) * nWidth +  CurrentPoint.x].ptReturnPoint = CurrentPoint;  // +Y 방향의 픽셀의 리턴 포인트 설정.
				CurrentPoint.y--;  // 픽셀의 값을 감소시켜 현재 픽셀의 값을 -Y 방향의 픽셀로 교체

				if(CurrentPoint.y <= 0)  // 현재 픽셀의 Y좌표값이 0이하인경우
					CurrentPoint.y = 0;   // 현재 픽셀의 Y좌표값을 0으로 설정

				if(*StartY >= CurrentPoint.y) // 탐색중인 픽셀이 현재 픽셀값보다 크거나 같은 경우
					*StartY = CurrentPoint.y; // 탐색중인 픽셀값을 현재 픽셀값으로 설정.

				continue;
			}
		}
	
		if( (CurrentPoint.y != nHeight - 1) && (DataBuf[(CurrentPoint.y + 1) * nWidth + CurrentPoint.x] == 255) ) // 현재 픽셀이 객체이고, +Y 방향의 픽셀또한 객체인 경우
		{
			if( m_vPoint[(CurrentPoint.y + 1) * nWidth +  CurrentPoint.x].bVisitedFlag == FALSE ) // +Y 방향의 픽셀이 객체이지만, FLAG 값이 FALSE인 경우
			{
				DataBuf[(CurrentPoint.y + 1) * nWidth + CurrentPoint.x]= DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x];// 현재 픽셀의 NUM값을 +Y방향의 픽셀에 저장. 즉 BLOB번호
				m_vPoint[(CurrentPoint.y + 1) * nWidth +  CurrentPoint.x].bVisitedFlag	= TRUE;  // FLAG 값을 TRUE로 바꿔줌.
				m_vPoint[(CurrentPoint.y + 1) * nWidth +  CurrentPoint.x].ptReturnPoint = CurrentPoint;  // +Y 방향의 픽셀의 리턴 포인트 설정.
				CurrentPoint.y++;  // 픽셀의 값을 증가시켜 현재 픽셀의 값을 +Y 방향의 픽셀로 교체

				if(CurrentPoint.y >= nHeight - 1)  // 현재 픽셀의 X좌표값이 이미지의 높이 보다 크거나 같은 경우
					CurrentPoint.y = nHeight - 1;  // 현재 픽셀의 X좌표값을 이미지의 높이로 설정.

				if(*EndY <= CurrentPoint.y)  // 탐색중인 픽셀이 현재 픽셀값보다 크거나 같은 경우
					*EndY = CurrentPoint.y;   // 탐색종료 픽셀값을 현재 픽셀값으로 설정.

				continue;
			}
		}
		
		if((CurrentPoint.x == m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint.x) // 현재 픽셀의 X,Y좌표값이 탐색 종료 X Y값과 같은경우
			&& (CurrentPoint.y == m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint.y))
		{
			break;  // while문 종료
		}
		else
		{
			CurrentPoint = m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint;  // 다른 경우 현재 픽셀의 정보 저장.
		}
	}

	return 0;
}

// 영역중 실제 blob의 칼라를 가진 영역의 크기를 획득
int CBlobLabeling::__Area(unsigned char *DataBuf, int StartX, int StartY, int EndX, int EndY, int nWidth, int nLevel)
{
	int nArea = 0;
	int nX, nY;

	for (nY = StartY; nY < EndY; nY++)      // 초기 탐색 지점에서 탐색 종료지점까지 for문 반복.
		for (nX = StartX; nX < EndX; nX++)
			if (DataBuf[nY * nWidth + nX] == nLevel)  // 픽셀의 NUM값이 같은 경우. 같은 객체로 인식될 때.
				++nArea;     // 객체로 인식된 픽셀수를 구함.

	return nArea;  // 픽셀수 리턴.
}