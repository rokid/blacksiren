//
//  r2mem_agc.cpp
//  r2ad2
//
//  Created by hadoop on 8/4/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#include "legacy/r2mem_agc.h"

r2mem_agc::r2mem_agc(void){
  
  m_iFrmSize = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS ;
  m_hEngine_Agc =  r2ssp_agc_create();
  r2ssp_agc_init(m_hEngine_Agc, R2_AUDIO_FRAME_MS, R2_AUDIO_SAMPLE_RATE);

  m_iDataLen_Total = m_iFrmSize * 100 ;
  m_pData_Out =  R2_SAFE_NEW_AR1(m_pData_Out, float, m_iDataLen_Total);
  m_pData_Tmp = R2_SAFE_NEW_AR1(m_pData_Tmp, short, m_iFrmSize);
  
}

r2mem_agc::~r2mem_agc(void){
  
  
  r2ssp_agc_free(m_hEngine_Agc);
  R2_SAFE_DEL_AR1(m_pData_Out);
  R2_SAFE_DEL_AR1(m_pData_Tmp);
}

int r2mem_agc::process(float* pData_In, int iLen_In, float*& pData_Out, int& iLen_Out){
  
  assert(iLen_In % m_iFrmSize == 0);
  
  if (iLen_In > m_iDataLen_Total) {
    R2_SAFE_DEL_AR1(m_pData_Out);
    m_iDataLen_Total = iLen_In * 2 ;
    m_pData_Out =  R2_SAFE_NEW_AR1(m_pData_Out, float, m_iDataLen_Total);
  }
  
  int iFrmNum = iLen_In / m_iFrmSize ;
  for (int i = 0 ; i < iFrmNum ; i ++) {
    for (int j = 0 ; j < m_iFrmSize ; j ++) {
      m_pData_Tmp[j] = pData_In[i * m_iFrmSize + j ] / 4.0f;
    }
    r2ssp_agc_process(m_hEngine_Agc, m_pData_Tmp);
    for (int j = 0 ; j < m_iFrmSize ; j ++) {
      m_pData_Out[i * m_iFrmSize + j ] = m_pData_Tmp[j] ;
    }
  }
  
  pData_Out = m_pData_Out ;
  iLen_Out = iLen_In ;
  
  return 0 ;
  
  
}

int r2mem_agc::reset(){
  
  r2ssp_agc_reset(m_hEngine_Agc);
  
  return 0 ;
}


