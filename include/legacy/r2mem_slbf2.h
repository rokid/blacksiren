//
//  r2mem_slbf2.h
//  r2ad2
//
//  Created by hadoop on 10/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2mem_slbf2__
#define __r2ad2__r2mem_slbf2__

#include "r2mem_sl3.h"
#include "r2mem_bf.h"


class r2mem_slbf2
{
public:
  r2mem_slbf2(int iMicNum, float* pMicPosLst, float* pMicDelay, r2_mic_info* pMicInfo_Bf, r2mem_sl3* pMem_Sl3);
  
public:
  ~r2mem_slbf2(void);
  
  int reset();
  int process(float** pData_In, int iLen_In, bool bVadEnd, float*& pData_Out, int& iLen_Out);
  int getleftfrmnum();
  const char* getinfo_sl();
  
protected:
  
  int AddDataIn(float** pData_In, int iLen_In);
  int ProcessLastDataOut();
  int AddDataOut(float* pData_Out, int iLen_Out);
  
public:
  
  //data
  int     m_iFrmSize ;
  int     m_iMicNum ;
  
  //In
  int     m_iFrmNum_Sl ;
  
  int     m_iFrmNum_In ;
  int     m_iFrmNum_In_Total ;
  float** m_pData_In ;
  
  //Out
  int     m_iFrmNum_Out ;
  int     m_iFrmNum_Block ;
  int     m_iFrmNum_Out_Total ;
  float*  m_pData_Out ;
  
  float   m_fAzimuth ;
  float   m_fElevation ;
  
  r2mem_sl3*  m_pMem_Sl3 ;
  r2mem_bf*   m_pMem_Bf ;
  
  //VadLen
  int   m_iFrmNum_Vad ;
  
};

#endif /* __r2ad2__r2mem_slbf2__ */
