//
//  r2mem_slbf2.cpp
//  r2ad2
//
//  Created by hadoop on 10/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#include "legacy/r2mem_slbf2.h"

r2mem_slbf2::r2mem_slbf2(int iMicNum, float* pMicPosLst, float* pMicDelay, r2_mic_info* pMicInfo_Bf, r2mem_sl3* pMem_Sl3){
  
  m_iFrmSize = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS ;
  
  m_iFrmNum_Sl = 12 ;
  m_iMicNum = iMicNum ;
  
  m_iFrmNum_In = 0 ;
  m_iFrmNum_In_Total = 64 ;
  m_pData_In = R2_SAFE_NEW_AR2(m_pData_In, float, iMicNum , m_iFrmNum_In_Total * m_iFrmSize) ;
  
  m_iFrmNum_Block = 12 ;
  m_iFrmNum_Out = 0 ;
  m_iFrmNum_Out_Total = 64 ;
  m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, float, m_iFrmNum_Out_Total * m_iFrmSize);
  
  m_fAzimuth = 0.0f ;
  m_fElevation = 0.0f ;
  
  m_pMem_Sl3 = pMem_Sl3 ;
  m_pMem_Bf = R2_SAFE_NEW(m_pMem_Bf, r2mem_bf, iMicNum,  pMicPosLst, pMicDelay,  pMicInfo_Bf);
  
  m_iFrmNum_Vad = 0 ;
}

r2mem_slbf2::~r2mem_slbf2(void){
  
  R2_SAFE_DEL(m_pMem_Bf) ;
  R2_SAFE_DEL_AR2(m_pData_In) ;
  R2_SAFE_DEL_AR1(m_pData_Out) ;
  
}

int r2mem_slbf2::reset(){
  
  m_iFrmNum_In = 0 ;
  m_iFrmNum_Out = 0 ;
  
  m_iFrmNum_Vad = 0 ;
  
  m_pMem_Bf->reset() ;
  
  return  0 ;
}


int r2mem_slbf2::process(float** pData_In, int iLen_In, bool bVadEnd, float*& pData_Out, int& iLen_Out){
  
  assert(iLen_In % m_iFrmSize == 0) ;
  
  ProcessLastDataOut() ;
  
  AddDataIn(pData_In, iLen_In);
  
  //BF
  if (m_iFrmNum_In >= m_iFrmNum_Sl || bVadEnd) {
    
    m_pMem_Sl3->getsl(m_iFrmNum_Sl * 3 * m_iFrmSize, 0, m_fAzimuth, m_fElevation);
    m_pMem_Bf->steer(m_fAzimuth, m_fElevation);
    
    //ZLOG_INFO(m_pMem_Bf->getinfo_sl()) ;
    
    float* pData_Out_Tmp = NULL ;
    int iLen_Out_Tmp = 0 ;
    m_pMem_Bf->process(m_pData_In , m_iFrmNum_In * m_iFrmSize , pData_Out_Tmp, iLen_Out_Tmp) ;
    
    AddDataOut(pData_Out_Tmp, iLen_Out_Tmp) ;
    
    m_iFrmNum_In = 0 ;
    
  }
  
  //For Vad End
  if (bVadEnd) {
    int iFrmNum_Left = m_iFrmNum_Out % m_iFrmNum_Block ;
    if(iFrmNum_Left > 0 ){
      int iFrmNum_Need = m_iFrmNum_Block - iFrmNum_Left ;
      memset(m_pData_Out + m_iFrmNum_Out * m_iFrmSize, 0, sizeof(float) * m_iFrmSize * iFrmNum_Need);
      m_iFrmNum_Out += iFrmNum_Need ;
    }
  }
  
  //Out
  pData_Out = m_pData_Out ;
  iLen_Out = (m_iFrmNum_Out / m_iFrmNum_Block) * m_iFrmNum_Block * m_iFrmSize ;
  
  return  0 ;
}

int r2mem_slbf2::AddDataIn(float** pData_In, int iLen_In){
  
  int iFrmNum_In = iLen_In / m_iFrmSize ;
  if (m_iFrmNum_In + iFrmNum_In > m_iFrmNum_In_Total) {
    m_iFrmNum_In_Total = (m_iFrmNum_In + iFrmNum_In) * 2 ;
    float** pTmp = R2_SAFE_NEW_AR2(pTmp, float, m_iMicNum, m_iFrmNum_In_Total * m_iFrmSize);
    for (int i = 0 ; i < m_iMicNum ; i ++) {
      memcpy(pTmp[i], m_pData_In[i], sizeof(float) * m_iFrmNum_In * m_iFrmSize);
    }
    R2_SAFE_DEL_AR2(m_pData_In);
    m_pData_In = pTmp ;
  }
  
  for (int i = 0 ; i < m_iMicNum ; i ++) {
    memcpy(m_pData_In[i] + m_iFrmNum_In * m_iFrmSize, pData_In[i],  sizeof(float) * iFrmNum_In * m_iFrmSize);
  }
  m_iFrmNum_In += iFrmNum_In ;
  
  m_iFrmNum_Vad += iFrmNum_In ;
  
  return 0 ;
}

int r2mem_slbf2::ProcessLastDataOut(){
  
  //process out left
  if (m_iFrmNum_Out / m_iFrmNum_Block > 0 ) {
    int iFrmNum_Left = m_iFrmNum_Out % m_iFrmNum_Block ;
    if (iFrmNum_Left > 0 ) {
      memcpy(m_pData_Out , m_pData_Out + (m_iFrmNum_Out - iFrmNum_Left) * m_iFrmSize, sizeof(float) * iFrmNum_Left * m_iFrmSize);
    }
    m_iFrmNum_Out = iFrmNum_Left ;
  }
  
  return  0 ;
}

int r2mem_slbf2::AddDataOut(float* pData_Out, int iLen_Out){
  
  //Copy to Out
  int iFrmNum_Out = iLen_Out / m_iFrmSize ;
  
  if (m_iFrmNum_Out + iFrmNum_Out + m_iFrmNum_Block > m_iFrmNum_Out_Total) {
    m_iFrmNum_Out_Total = (m_iFrmNum_Out + iFrmNum_Out + m_iFrmNum_Block) * 2 ;
    float* pData_Tmp = R2_SAFE_NEW_AR1(pData_Tmp, float, m_iFrmNum_Out_Total * m_iFrmSize);
    memcpy(pData_Tmp, m_pData_Out, sizeof(float) * m_iFrmNum_Out * m_iFrmSize);
    R2_SAFE_DEL_AR1(m_pData_Out);
    m_pData_Out = pData_Tmp ;
  }
  
  memcpy(m_pData_Out + m_iFrmNum_Out * m_iFrmSize, pData_Out, sizeof(float) * iLen_Out);
  
  m_iFrmNum_Out += iFrmNum_Out ;
  
  return 0 ;
}

const char* r2mem_slbf2::getinfo_sl(){
  
  return  m_pMem_Bf->getinfo_sl() ;
}

int r2mem_slbf2::getleftfrmnum(){
  
  return m_iFrmNum_Out % m_iFrmNum_Block ;
}





