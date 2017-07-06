//
//  r2mem_i.cpp
//  r2ad
//
//  Created by hadoop on 10/23/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#include "legacy/r2mem_i.h"
#include <assert.h>

r2mem_i::r2mem_i(int iMicNum, r2_in_type iInType, r2_mic_info* pMicInfo_In){
  
  //In
  m_iMicNum = iMicNum ;
  m_iInType = iInType ;
  m_pMicInfo_In = pMicInfo_In ;
  
  
  m_iLen_Out_Total = R2_AUDIO_SAMPLE_RATE ;
  m_iLen_Out = 0 ;
  m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total);
  
}

r2mem_i::~r2mem_i(void){
  
  R2_SAFE_DEL_AR2(m_pData_Out);
}

int r2mem_i::reset(){
  
  m_iLen_Out = 0 ;
  
  return  0 ;
}

int r2mem_i::process(char* pData_In, int iLen_In, float**& pData_Out, int& iLen_Out){
  
  assert(iLen_In == 0 || (iLen_In > 0 && pData_In != NULL)) ;
  R2_MEM_ASSERT(this,0);
  
  if (m_iInType == r2_in_int_24) {
    
    assert(iLen_In % (sizeof(r2_int24) * m_pMicInfo_In->iMicNum) == 0 );
    iLen_Out = iLen_In / (sizeof(r2_int24) * m_pMicInfo_In->iMicNum) ;
    
    if (iLen_Out > m_iLen_Out_Total) {
      R2_SAFE_DEL_AR2(m_pData_Out);
      m_iLen_Out_Total = iLen_Out * 2 ;
      m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total);
    }
    
    r2_int24* pData = (r2_int24*) pData_In ;
    for (int j = 0 ; j < m_pMicInfo_In->iMicNum ; j ++) {
      int iMicId = m_pMicInfo_In->pMicIdLst[j] ;
      for (int i = 0 ; i < iLen_Out ; i ++) {
        m_pData_Out[iMicId][i] = pData[i * m_pMicInfo_In->iMicNum + j].toint() / 4.0f;
      }
    }
    
    pData_Out = m_pData_Out ;
    
  }else if (m_iInType == r2_in_int_32_10) {
    
    assert(iLen_In % (sizeof(int) * m_pMicInfo_In->iMicNum) == 0 );
    iLen_Out = iLen_In / (sizeof(int) * m_pMicInfo_In->iMicNum) ;
    
    if (iLen_Out > m_iLen_Out_Total) {
      R2_SAFE_DEL_AR2(m_pData_Out);
      m_iLen_Out_Total = iLen_Out * 2 ;
      m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total);
    }
    
    int* pData = (int*) pData_In ;
    for (int j = 0 ; j < m_pMicInfo_In->iMicNum ; j ++) {
      int iMicId = m_pMicInfo_In->pMicIdLst[j] ;
      for (int i = 0 ; i < iLen_Out ; i ++) {
        m_pData_Out[iMicId][i] = pData[i * m_pMicInfo_In->iMicNum + j] / 4.0f ;
      }
    }
    
    pData_Out = m_pData_Out ;
    
  }else if (m_iInType == r2_in_int_32) {
    
    assert(iLen_In % (sizeof(int) * m_pMicInfo_In->iMicNum) == 0 );
    iLen_Out = iLen_In / (sizeof(int) * m_pMicInfo_In->iMicNum) ;
    
    if (iLen_Out > m_iLen_Out_Total) {
      R2_SAFE_DEL_AR2(m_pData_Out);
      m_iLen_Out_Total = iLen_Out * 2 ;
      m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total);
    }
    
    int* pData = (int*) pData_In ;
    for (int j = 0 ; j < m_pMicInfo_In->iMicNum ; j ++) {
      int iMicId = m_pMicInfo_In->pMicIdLst[j] ;
      for (int i = 0 ; i < iLen_Out ; i ++) {
        m_pData_Out[iMicId][i] = pData[i * m_pMicInfo_In->iMicNum + j] / 1024.0f ;
      }
    }
    pData_Out = m_pData_Out ;
    
  }else if (m_iInType == r2_in_float_32){
    
    assert(iLen_In % (sizeof(float) * m_pMicInfo_In->iMicNum) == 0 );
    iLen_Out = iLen_In / (sizeof(float) * m_pMicInfo_In->iMicNum) ;
    
    if (iLen_Out > m_iLen_Out_Total) {
      R2_SAFE_DEL_AR2(m_pData_Out);
      m_iLen_Out_Total = iLen_Out * 2 ;
      m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total);
    }
    
    float* pData = (float*) pData_In ;
    for (int j = 0 ; j < m_pMicInfo_In->iMicNum ; j ++) {
      int iMicId = m_pMicInfo_In->pMicIdLst[j] ;
      for (int i = 0 ; i < iLen_Out ; i ++) {
        m_pData_Out[iMicId][i] = pData[i * m_pMicInfo_In->iMicNum + j];
      }
    }
    
    
    pData_Out = m_pData_Out ;
  }
  
  
  return 0 ;
}
