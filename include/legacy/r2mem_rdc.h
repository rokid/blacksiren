//
//  r2mem_rdc.h
//  r2ad2
//
//  Created by hadoop on 11/4/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2mem_rdc__
#define __r2ad2__r2mem_rdc__

#include "r2math.h"

class r2mem_rdc
{
public:
  r2mem_rdc(r2_mic_info* pMicInfo_Rdc,r2_mic_info* pMicInfo_AecRef, int iMaxLen);
public:
  ~r2mem_rdc(void);
  
public:
  int reset();
  int process(float** pData_In, int& iLen_In);
  
public:
  
  r2_mic_info* m_pMicInfo_Rdc;
  r2_mic_info* m_pMicInfo_AecRef;
  
  int m_iCurLen ;
  int m_iMaxLen ;
  
  float* m_pRDc ;
  double* m_pRDc_Var ;
  
  int * m_bMicOk ;
  
};

#endif /* __r2ad2__r2mem_rdc__ */
