//
//  r2mem_o.cpp
//  r2ad
//
//  Created by hadoop on 10/23/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#include "legacy/r2mem_o.h"

r2mem_o::r2mem_o(int iMicNum, r2_out_type iOutType, r2_mic_info* pMicInfo_Out){
  
  m_iMicNum = iMicNum ;
  
  //Out
  m_pMicInfo_Out = pMicInfo_Out ;
  m_iOutType = iOutType ;
  
  if (m_iOutType == r2_out_int_32) {
    m_iLen_Out_Total = R2_AUDIO_SAMPLE_RATE * sizeof(int) * m_pMicInfo_Out->iMicNum ;
  }else if(m_iOutType == r2_out_float_32){
    m_iLen_Out_Total = R2_AUDIO_SAMPLE_RATE * sizeof(float) * m_pMicInfo_Out->iMicNum ;
  }else {
    m_iLen_Out_Total = R2_AUDIO_SAMPLE_RATE * sizeof(short) * m_pMicInfo_Out->iMicNum ;
  }
  m_iLen_Out = 0 ;
  m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, char, m_iLen_Out_Total);
  

  
}

r2mem_o::~r2mem_o(void){
  
  R2_SAFE_DEL_AR1(m_pData_Out);
  
}

int r2mem_o::reset(){
  
  m_iLen_Out = 0 ;
  
  return  0 ;
}

int r2mem_o::process(float** pData_In, int iLen_In, char* &pData_Out, int& iLen_Out){
  
  assert(iLen_In == 0 || (iLen_In > 0 && pData_In != NULL)) ;
  R2_MEM_ASSERT(this,0);
  
  if (m_iOutType == r2_out_int_32) {
    
    iLen_Out = iLen_In * sizeof(int) * m_pMicInfo_Out->iMicNum ;
    if (iLen_Out > m_iLen_Out_Total) {
      m_iLen_Out_Total = iLen_Out * 2 ;
      R2_SAFE_DEL_AR1(m_pData_Out);
      m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, char, m_iLen_Out_Total);
    }
    
    int* pData = (int*) m_pData_Out ;
    for (int j = 0 ; j < m_pMicInfo_Out->iMicNum ; j ++) {
      int iMicId = m_pMicInfo_Out->pMicIdLst[j] ;
      for (int i = 0 ; i < iLen_In ; i ++) {
        pData[i * m_pMicInfo_Out->iMicNum + j] = pData_In[iMicId][i];
      }
    }
    pData_Out = (char*) m_pData_Out ;
    
    return 0 ;
  }else if(m_iOutType == r2_out_float_32){
    iLen_Out = iLen_In * sizeof(float) * m_pMicInfo_Out->iMicNum ;
    if (iLen_Out > m_iLen_Out_Total) {
      m_iLen_Out_Total = iLen_Out * 2 ;
      R2_SAFE_DEL_AR1(m_pData_Out);
      m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, char, m_iLen_Out_Total);
    }
    
    float* pData = (float*) m_pData_Out ;
    for (int j = 0 ; j < m_pMicInfo_Out->iMicNum ; j ++) {
      int iMicId = m_pMicInfo_Out->pMicIdLst[j] ;
      for (int i = 0 ; i < iLen_In ; i ++) {
        pData[i * m_pMicInfo_Out->iMicNum + j] = pData_In[iMicId][i];
      }
    }
    
    pData_Out = (char*) m_pData_Out ;
    
    return 0 ;
  }else{
    
    iLen_Out = iLen_In * sizeof(short) * m_pMicInfo_Out->iMicNum ;
    if (iLen_Out > m_iLen_Out_Total) {
      m_iLen_Out_Total = iLen_Out * 2 ;
      R2_SAFE_DEL_AR1(m_pData_Out);
      m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, char, m_iLen_Out_Total);
    }
    
    short* pData = (short*) m_pData_Out ;
    for (int j = 0 ; j < m_pMicInfo_Out->iMicNum ; j ++) {
      int iMicId = m_pMicInfo_Out->pMicIdLst[j] ;
      for (int i = 0 ; i < iLen_In ; i ++) {
        pData[i * m_pMicInfo_Out->iMicNum + j] = pData_In[iMicId][i];
      }
    }
    
    pData_Out = (char*) m_pData_Out ;
    
    return 0 ;
  }
  
  
}
