//
//  r2mem_bf.h
//  r2audio
//
//  Created by hadoop on 7/22/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#ifndef __r2audio__r2mem_bf__
#define __r2audio__r2mem_bf__

#include <stdio.h>

#include "r2math.h"

#ifdef CONFIG_BF_MVDR
#include "r2mvdrapi.h"
#else
#include "r2ssp.h"
#endif

class r2mem_bf
{
public:
  r2mem_bf(int iMicNum, float* pMicPosLst, float* pMicDelay, r2_mic_info* pMicInfo_Bf);
  
public:
  ~r2mem_bf(void);
  
public:
  int reset();
  int process(float** pData_In, int iLen_In, float*& pData_Out, int& iLen_Out);
  
  int steer(float fAzimuth, float fElevation, int bSteer = 1);
  const char* getinfo_sl();
  
  bool check(float fAzimuth, float fElevation);
  
public:
  
  //in
  int m_iMicNum ;
  int m_iFrmSize ;
  int m_iFrmSize_Out ;
  
  float m_fSlInfo[3] ;
  
  //pos
  r2_mic_info* m_pMicInfo_Bf ;
  float* m_pMics_Bf ;
  float* m_pMicI2sDelay ;
  
  float * m_pData_Bf ;
  
  int m_iLen_In_Cur ;
  float** m_pData_In ;
  
  //out
  int m_iLen_Out_Total ;
  float* m_pData_Out ;
  
#ifdef CONFIG_BF_MVDR
  r2mvdr_handle m_hEngine_Bf ;
#else
  r2ssp_handle m_hEngine_Bf ;
#endif
  
  //output
  std::string m_strSlInfo ;
  
};

#endif /* defined(__r2audio__r2mem_bf__) */
