//
//  r2mem_agc.h
//  r2ad2
//
//  Created by hadoop on 8/4/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2mem_agc__
#define __r2ad2__r2mem_agc__

#include "r2ssp.h"
#include "r2math.h"


class r2mem_agc
{
public:
  r2mem_agc(void);
public:
  ~r2mem_agc(void);
  
public:
  int process(float* pData_In, int iLen_In, float*& pData_Out, int& iLen_Out);
  int reset();
  
public:
  
  int m_iFrmSize ;
  r2ssp_handle m_hEngine_Agc ;
  
  int m_iDataLen_Total ;
  float * m_pData_Out ;
  
  short* m_pData_Tmp ;
  
};


#endif /* __r2ad2__r2mem_agc__ */
