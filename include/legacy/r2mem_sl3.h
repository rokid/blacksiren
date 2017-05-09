//
//  r2mem_sl3.h
//  r2ad2
//
//  Created by hadoop on 10/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2mem_sl3__
#define __r2ad2__r2mem_sl3__


#include "r2math.h"
#include "r2mem_ns.h"

#define USE_LZHU_SL

#ifdef USE_LZHU_SL
#include "zslapi.h"
#else
#include "zsourcelocationapi.h"
#endif


class r2mem_sl3
{
public:
  r2mem_sl3(int iMicNum, float* pMicPosLst, float* pMicDelay, r2_mic_info* pMicInfo_Sl);
  
public:
  ~r2mem_sl3(void);
  
public:
  int putdata(float** pfDataBuff, int iDataLen);
  int getsl(int iStartPos, int iEndPos, float& fAzimuth, float& fElevation );
  int reset();
  
  int fixmic(r2_mic_info* pMicInfo_Err) ;
  
public:
  
  //Ns
  r2mem_ns* m_pMem_Ns ;
  
  //mic
  int m_iMicNum ;
  float* m_pMics_Sl ;
  float* m_pMicI2sDelay ;
  r2_mic_info* m_pMicInfo_Sl ;
  
  
  //engine
#ifdef USE_LZHU_SL
  r2_sl_htask m_hEngine_Sl ;
#else
  r2_sourcelocation_htask m_hEngine_Sl ;
#endif
  
  float** m_pData_Sl ;
  float* m_pCandidate ;
  
};


#endif /* __r2ad2__r2mem_sl3__ */
