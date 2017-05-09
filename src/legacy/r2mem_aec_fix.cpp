//
//  r2mem_aec_fix.cpp
//  r2ad2
//
//  Created by hadoop on 1/20/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#include "legacy/r2mem_aec_fix.h"

r2mem_aec_fix::r2mem_aec_fix(int iMicNum, float fAecShield, r2_mic_info* pMicInfo_AecRef){
  
  m_iMicNum = iMicNum ;
  m_fAecShield = fAecShield * 5 ;
  
  m_pMicInfo_AecRef = pMicInfo_AecRef ;
  
  m_iNoAecDur_Total = R2_AUDIO_SAMPLE_RATE / 1000 * 30 ;
  m_iNoAecDur_Cur = 0 ;
  
}


r2mem_aec_fix::~r2mem_aec_fix(void){
  
  
}

int r2mem_aec_fix::process(float** pData_In, int& iLen_In){
  
  if (m_iMicNum == 8) {
    bool bAec = false ;
    for (int i = 0 ; i < m_pMicInfo_AecRef->iMicNum ; i ++) {
      int iMicId = m_pMicInfo_AecRef->pMicIdLst[i] ;
      for (int j = 0 ; j < iLen_In ; j ++) {
        if (fabs(pData_In[iMicId][j]) > m_fAecShield) {
          bAec = true ;
          break ;
        }
      }
      if (bAec) {
        break ;
      }
    }
    
    if (bAec) {
      m_iNoAecDur_Cur = 0 ;
    }else{
      if (m_iNoAecDur_Cur < m_iNoAecDur_Total) {
        bAec = true ;
        m_iNoAecDur_Cur += iLen_In ;
      }
    }
    
    if (bAec) {
      for (int i = 0 ; i < m_pMicInfo_AecRef->iMicNum ; i ++) {
        int iMicId = m_pMicInfo_AecRef->pMicIdLst[i] ;
        for (int j = 0 ; j < iLen_In ; j ++) {
          pData_In[iMicId][j] = pData_In[iMicId][j] / 5 ;
        }
      }
    }else{
      for (int i = 0 ; i < m_pMicInfo_AecRef->iMicNum ; i ++) {
        int iMicId = m_pMicInfo_AecRef->pMicIdLst[i] ;
        memset(pData_In[iMicId], 0, sizeof(float) * iLen_In) ;
      }
    }
  }
  return  0 ;
  
}



