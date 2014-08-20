
#include "StdAfx.h"
#include "BlobLabeling.h"

#define _DEF_MAX_BLOBS		10000
#define _DEF_MAX_LABEL		  100

CBlobLabeling::CBlobLabeling(void)  // ������
{
	m_nThreshold	= 0;
	m_nBlobs		= _DEF_MAX_BLOBS;
	m_Image			= NULL;
	                                        // ���̺��� �ʿ��� ���� �ʱⰪ ����
	m_recBlobs		= NULL;
	m_intBlobs		= NULL;	
}

CBlobLabeling::~CBlobLabeling(void)   // �Ҹ���
{
	if( m_Image != NULL )	cvReleaseImage( &m_Image );	

	if( m_recBlobs != NULL )
	{
		delete m_recBlobs;
		m_recBlobs = NULL;           // �̹����� ��ȯ�ϰ� �����Ҵ��� ���� ���� �� ���� NULL�� ����
	}

	if( m_intBlobs != NULL )
	{
		delete m_intBlobs;
		m_intBlobs = NULL;
	}
}

void CBlobLabeling::SetParam(IplImage* image, int nThreshold)   // ���� �� �����Լ�
{
	if( m_recBlobs != NULL )   // �� ���̺��� ������ �̹� ������
	{
		delete m_recBlobs;  // �����Ҵ�� BLOB ����

		m_recBlobs	= NULL;            // �ʱⰪ���� ����
		m_nBlobs	= _DEF_MAX_BLOBS;
	}

	if( m_intBlobs != NULL )   // �� ���̺��� �ε����� �̹� ������
	{
		delete m_intBlobs;   // �����Ҵ�� BLOB ����

		m_intBlobs	= NULL;             // �ʱⰪ���� ����
		m_nBlobs	= _DEF_MAX_BLOBS;
	}

	if( m_Image != NULL )	cvReleaseImage( &m_Image );   // ���̺��� �ʿ��� �̹����� �̹� ���õ����� ��� �̹��� ��ȯ

	m_Image			= cvCloneImage( image );   // �޾ƿ� �̹����� �״�� ����

	m_nThreshold	= nThreshold;   // �޾ƿ� Threshold ���� ���̺� Ŭ������ ������ ����
}

void CBlobLabeling::DoLabeling()  // ���̺� ���� �Լ�
{
	m_nBlobs = Labeling(m_Image, m_nThreshold);   // �̹����� Threshold���� �Ѱ��־� �νĵ� ��ü�� ������ ���� ����.
}

int CBlobLabeling::Labeling(IplImage* image, int nThreshold)  // ���̺� �Լ�
{
	if( image->nChannels != 1 ) 	
		return 0;   // �̹����� ����� �ƴҶ� �Լ� ����.

	int nNumber;  // ��ü���� ���� ����
	
	int nWidth	= image->width;   // �̹����� �ʺ�� ���̸� ����
	int nHeight = image->height;
	
	unsigned char* tmpBuf = new unsigned char [nWidth * nHeight];  // �̹����� ���� 1�������� ����ϱ� ���� �����Ҵ�.

	int i,j;

	for(j=0;j<nHeight;j++)      // �̹����� �ʺ�� ���̸�ŭ for�� �ݺ�
	for(i=0;i<nWidth ;i++)
		tmpBuf[j*nWidth+i] = (unsigned char)image->imageData[j*image->widthStep+i];   // �̹��� ������ ���� temp �迭�� ����
	
	InitvPoint(nWidth, nHeight);   // ���̺��� ���� ����Ʈ �ʱ�ȭ

	nNumber = _Labeling(tmpBuf, nWidth, nHeight, nThreshold);  // ���̺�

	DeletevPoint(); // ����Ʈ �޸� ����

	if( nNumber != _DEF_MAX_BLOBS )		m_recBlobs = new CvRect [nNumber]; // ��ü�� ������ �ʱⰪ�� �ƴҶ� ��ü�� ������ŭ �����Ҵ�.

	if( nNumber != _DEF_MAX_BLOBS )		m_intBlobs = new int [nNumber];

	if( nNumber != 0 )	DetectLabelingRegion(nNumber, tmpBuf, nWidth, nHeight);  // ��ü�� ������ 0�̻� �ϰ�� �簢���� �׸������� ��ǥ Ž��.

	for(j=0;j<nHeight;j++)
	for(i=0;i<nWidth ;i++)
		image->imageData[j*image->widthStep+i] = tmpBuf[j*nWidth+i];  // �ӽ� ������ ���� ���� �̹����� ����

	delete tmpBuf;   // temp ���� ����
	return nNumber;  // ��ü ���� ����
}

// m_vPoint �ʱ�ȭ �Լ�
void CBlobLabeling::InitvPoint(int nWidth, int nHeight)
{
	int nX, nY;   // for���� ����ϱ� ���� ����

	m_vPoint = new Visited [nWidth * nHeight];  // 1�������� ����ϱ� ���Ͽ� �̹����� �� �ȼ��� �ش��ϴ� ������ �����Ҵ�.

	for(nY = 0; nY < nHeight; nY++)       // �̹����� ũ�⸸ŭ for�� �ݺ�
	{
		for(nX = 0; nX < nWidth; nX++)
		{
			m_vPoint[nY * nWidth + nX].bVisitedFlag		= FALSE;   // 2�������� ������ �� �̹��� �ȼ��� �ش��ϴ� m_vPoint���� ���� ����.
			m_vPoint[nY * nWidth + nX].ptReturnPoint.x	= nX;
			m_vPoint[nY * nWidth + nX].ptReturnPoint.y	= nY;
		}
	}
}

void CBlobLabeling::DeletevPoint()
{
	delete m_vPoint;
}

// Size�� nWidth�̰� nHeight�� DataBuf���� 
// nThreshold���� ���� ������ ������ �������� blob���� ȹ��
int CBlobLabeling::_Labeling(unsigned char *DataBuf, int nWidth, int nHeight, int nThreshold)
{
	int Index = 0, num = 0;
	int nX, nY, k, l;                   // ���̺��� �ʿ��� ��ǥ ���� �� ����.
	int StartX , StartY, EndX , EndY;
	
	for(nY = 0; nY < nHeight; nY++)         // �̹����� �ȼ��� ��ŭ for�� �ݺ�
	{
		for(nX = 0; nX < nWidth; nX++)
		{
			if(DataBuf[nY * nWidth + nX] == 255)  // 2���� �̹����� �ȼ����� ����Ǿ��ִ� 1�����迭���� �� �ȼ��� ���� 255�϶� �� ��ü�� �Ǵܵɋ�
			{
				num++;   // ��ü�� ����

				DataBuf[nY * nWidth + nX] = num;  // �ش� �ȼ��� ���� num���� �ٲ���.
				
				StartX = nX, StartY = nY, EndX = nX, EndY= nY;  // ���� Ž������ X,Y��ǥ�� Ž�� ���� ��ǥ ����. 

				__NRFIndNeighbor(DataBuf, nWidth, nHeight, nX, nY, &StartX, &StartY, &EndX, &EndY);  // ��ü�� �νĵ� �ȼ��� ���� �ȼ����� �����ϴ� �Լ�

				if(__Area(DataBuf, StartX, StartY, EndX, EndY, nWidth, num) < nThreshold)  //  ��ü�� �ȼ����� Threshold������ ���� ���
				{
		 			for(k = StartY; k <= EndY; k++)
					{
						for(l = StartX; l <= EndX; l++)         // Threshold ������ ���� ��� ��ü�� ����
						{
							if(DataBuf[k * nWidth + l] == num)
								DataBuf[k * nWidth + l] = 0;     // ��ü�� ��ȣ�� �ٽ� 0���� �ٲ���.
						}
					}
					--num;  // ��ü ���� ����

					if(num > 250)   // ��ü ������ 250�̻��ΰ��
						return  0;  // �Լ� ����
				}
			}
		}
	}

	return num;	// ��ü ���� ��ȯ.
}


void CBlobLabeling::DetectLabelingRegion(int nLabelNumber, unsigned char *DataBuf, int nWidth, int nHeight)  // Blob labeling�ؼ� ����� ����� rec�� �� 
{
	int nX, nY;
	int nLabelIndex ;

	bool bFirstFlag[255] = {FALSE,};
	
	for(nY = 1; nY < nHeight - 1; nY++)    // �̹��� Ž��
	{
		for(nX = 1; nX < nWidth - 1; nX++)
		{
			nLabelIndex = DataBuf[nY * nWidth + nX];  // �ȼ����� Labelindex�� ����

			if(nLabelIndex != 0) // �ε����� ���� �����Ͽ� ��ü�϶�
			{
				if(bFirstFlag[nLabelIndex] == FALSE) // �ʱ����
				{
					m_recBlobs[nLabelIndex-1].x			= nX;   // ��ǥ ����
					m_recBlobs[nLabelIndex-1].y			= nY;
					m_recBlobs[nLabelIndex-1].width		= 0;
					m_recBlobs[nLabelIndex-1].height	= 0;
				
					bFirstFlag[nLabelIndex] = TRUE;  // flag���� true�� �ٲ���.
				}
				else
				{
					int left	= m_recBlobs[nLabelIndex-1].x;
					int right	= left + m_recBlobs[nLabelIndex-1].width;
					int top		= m_recBlobs[nLabelIndex-1].y;
					int bottom	= top + m_recBlobs[nLabelIndex-1].height;

					if( left   >= nX )	left	= nX;                      // ��ü�� �簢������ ǥ���ϱ����� �������� �ش��ϴ� �κ��� ��ǥ�� ����.
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
	CvPoint CurrentPoint;    // ���� �ȼ��� ��ǥ���� ǥ���ϱ� ���� ����
	
	CurrentPoint.x = nPosX;     // ��ü�� �νĵ� �ȼ��� x,y���� ����
	CurrentPoint.y = nPosY;

	m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x].bVisitedFlag    = TRUE;    // ��ü�� �νĵ� �ȼ��� Flag���� TRUE�� �ٲپ� ��ü�� �ν��� �� �ֵ��� ����
	m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x].ptReturnPoint.x = nPosX;
	m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x].ptReturnPoint.y = nPosY;
			
	while(1)  // while���� ���� ��ü�� �νĵ� ��ǥ�� ������ǥ���� ��ü ������ ����ؼ� �ľ���.
	{
		if( (CurrentPoint.x != 0) && (DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x - 1] == 255) )  // ���� �ȼ��� ��ü�̰�, -X ������ �ȼ����� ��ü�� ���
		{
			if( m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x - 1].bVisitedFlag == FALSE ) // -X ������ �ȼ��� ��ü������, FLAG ���� FALSE�� ���
			{
				DataBuf[CurrentPoint.y  * nWidth + CurrentPoint.x  - 1]	= DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x]; // ���� �ȼ��� NUM���� -X������ �ȼ��� ����. �� BLOB��ȣ
				m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x - 1].bVisitedFlag	= TRUE;   // FLAG ���� TRUE�� �ٲ���.
				m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x - 1].ptReturnPoint	= CurrentPoint;  // -X ������ �ȼ��� ���� ����Ʈ ����.
				CurrentPoint.x--; // �ȼ��� ���� ���ҽ��� ���� �ȼ��� ���� -X ������ �ȼ��� ��ü
				
				if(CurrentPoint.x <= 0)  // ���� �ȼ��� X��ǥ���� 0�����ΰ��
					CurrentPoint.x = 0;  // ���� �ȼ��� X��ǥ���� 0���� ����

				if(*StartX >= CurrentPoint.x)  // Ž������ �ȼ��� ���� �ȼ������� ũ�ų� ���� ���
					*StartX = CurrentPoint.x;  // Ž������ �ȼ����� ���� �ȼ������� ����.

				continue;
			}
		}

		if( (CurrentPoint.x != nWidth - 1) && (DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x + 1] == 255) ) // ���� �ȼ��� ��ü�̰�, +X ������ �ȼ����� ��ü�� ���
		{
			if( m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x + 1].bVisitedFlag == FALSE )  // +X ������ �ȼ��� ��ü������, FLAG ���� FALSE�� ���
			{
				DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x + 1] = DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x]; // ���� �ȼ��� NUM���� +X������ �ȼ��� ����. �� BLOB��ȣ
				m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x + 1].bVisitedFlag	= TRUE;  // FLAG ���� TRUE�� �ٲ���.
				m_vPoint[CurrentPoint.y * nWidth +  CurrentPoint.x + 1].ptReturnPoint	= CurrentPoint;   // +X ������ �ȼ��� ���� ����Ʈ ����.
				CurrentPoint.x++; // �ȼ��� ���� �������� ���� �ȼ��� ���� +X ������ �ȼ��� ��ü

				if(CurrentPoint.x >= nWidth - 1)   // ���� �ȼ��� X��ǥ���� �̹����� �ʺ񺸴� ũ�ų� ���� ���
					CurrentPoint.x = nWidth - 1;   // ���� �ȼ��� X��ǥ���� �̹����� �ʺ�� ����.
				
				if(*EndX <= CurrentPoint.x)  // Ž������ �ȼ��� �� ���� �ȼ������� �۰ų� �������
					*EndX = CurrentPoint.x;  // Ž������ �ȼ����� ���� �ȼ������� ����.

				continue;
			}
		}

		if( (CurrentPoint.y != 0) && (DataBuf[(CurrentPoint.y - 1) * nWidth + CurrentPoint.x] == 255) ) // ���� �ȼ��� ��ü�̰�, -Y ������ �ȼ����� ��ü�� ���
		{
			if( m_vPoint[(CurrentPoint.y - 1) * nWidth +  CurrentPoint.x].bVisitedFlag == FALSE ) // -Y ������ �ȼ��� ��ü������, FLAG ���� FALSE�� ���
			{
				DataBuf[(CurrentPoint.y - 1) * nWidth + CurrentPoint.x]	= DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x]; // ���� �ȼ��� NUM���� +Y������ �ȼ��� ����. �� BLOB��ȣ
				m_vPoint[(CurrentPoint.y - 1) * nWidth +  CurrentPoint.x].bVisitedFlag	= TRUE; // FLAG ���� TRUE�� �ٲ���.
				m_vPoint[(CurrentPoint.y - 1) * nWidth +  CurrentPoint.x].ptReturnPoint = CurrentPoint;  // +Y ������ �ȼ��� ���� ����Ʈ ����.
				CurrentPoint.y--;  // �ȼ��� ���� ���ҽ��� ���� �ȼ��� ���� -Y ������ �ȼ��� ��ü

				if(CurrentPoint.y <= 0)  // ���� �ȼ��� Y��ǥ���� 0�����ΰ��
					CurrentPoint.y = 0;   // ���� �ȼ��� Y��ǥ���� 0���� ����

				if(*StartY >= CurrentPoint.y) // Ž������ �ȼ��� ���� �ȼ������� ũ�ų� ���� ���
					*StartY = CurrentPoint.y; // Ž������ �ȼ����� ���� �ȼ������� ����.

				continue;
			}
		}
	
		if( (CurrentPoint.y != nHeight - 1) && (DataBuf[(CurrentPoint.y + 1) * nWidth + CurrentPoint.x] == 255) ) // ���� �ȼ��� ��ü�̰�, +Y ������ �ȼ����� ��ü�� ���
		{
			if( m_vPoint[(CurrentPoint.y + 1) * nWidth +  CurrentPoint.x].bVisitedFlag == FALSE ) // +Y ������ �ȼ��� ��ü������, FLAG ���� FALSE�� ���
			{
				DataBuf[(CurrentPoint.y + 1) * nWidth + CurrentPoint.x]= DataBuf[CurrentPoint.y * nWidth + CurrentPoint.x];// ���� �ȼ��� NUM���� +Y������ �ȼ��� ����. �� BLOB��ȣ
				m_vPoint[(CurrentPoint.y + 1) * nWidth +  CurrentPoint.x].bVisitedFlag	= TRUE;  // FLAG ���� TRUE�� �ٲ���.
				m_vPoint[(CurrentPoint.y + 1) * nWidth +  CurrentPoint.x].ptReturnPoint = CurrentPoint;  // +Y ������ �ȼ��� ���� ����Ʈ ����.
				CurrentPoint.y++;  // �ȼ��� ���� �������� ���� �ȼ��� ���� +Y ������ �ȼ��� ��ü

				if(CurrentPoint.y >= nHeight - 1)  // ���� �ȼ��� X��ǥ���� �̹����� ���� ���� ũ�ų� ���� ���
					CurrentPoint.y = nHeight - 1;  // ���� �ȼ��� X��ǥ���� �̹����� ���̷� ����.

				if(*EndY <= CurrentPoint.y)  // Ž������ �ȼ��� ���� �ȼ������� ũ�ų� ���� ���
					*EndY = CurrentPoint.y;   // Ž������ �ȼ����� ���� �ȼ������� ����.

				continue;
			}
		}
		
		if((CurrentPoint.x == m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint.x) // ���� �ȼ��� X,Y��ǥ���� Ž�� ���� X Y���� �������
			&& (CurrentPoint.y == m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint.y))
		{
			break;  // while�� ����
		}
		else
		{
			CurrentPoint = m_vPoint[CurrentPoint.y * nWidth + CurrentPoint.x].ptReturnPoint;  // �ٸ� ��� ���� �ȼ��� ���� ����.
		}
	}

	return 0;
}

// ������ ���� blob�� Į�� ���� ������ ũ�⸦ ȹ��
int CBlobLabeling::__Area(unsigned char *DataBuf, int StartX, int StartY, int EndX, int EndY, int nWidth, int nLevel)
{
	int nArea = 0;
	int nX, nY;

	for (nY = StartY; nY < EndY; nY++)      // �ʱ� Ž�� �������� Ž�� ������������ for�� �ݺ�.
		for (nX = StartX; nX < EndX; nX++)
			if (DataBuf[nY * nWidth + nX] == nLevel)  // �ȼ��� NUM���� ���� ���. ���� ��ü�� �νĵ� ��.
				++nArea;     // ��ü�� �νĵ� �ȼ����� ����.

	return nArea;  // �ȼ��� ����.
}