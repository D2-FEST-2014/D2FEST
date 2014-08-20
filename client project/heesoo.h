#include "stdafx.h"


int PYRAMIDMAX = 3;

#define  FEATURE_SIZE      36
#define  CONT_THRESHOLD    1.2f
IplImage *m_image;

#define   MAX_PATTERN_SIZE  10

typedef  int FVECTOR[FEATURE_SIZE];

double **dir;
int patternSize=0;

int averageContrast;

FVECTOR trainList[MAX_PATTERN_SIZE];

IplImage *trainImage[MAX_PATTERN_SIZE];

int Key =-1;

int m_nindex=0;