//
//  r2mem_rs2.cpp
//  r2ad2
//
//  Created by hadoop on 4/19/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#include "legacy/r2mem_rs2.h"

  r2mem_rs2::r2mem_rs2(int iMicNum, int iSampleRate, r2_mic_info* pMicInfo_Rs, bool bDelay){
    
    m_iMicNum = iMicNum ;
    m_pMicInfo_Rs = pMicInfo_Rs ;
    
    m_bDelay = bDelay ;
    
    m_hRs_48_16 = r2_rs_create(m_pMicInfo_Rs->iMicNum, 3, 1, 160) ;
    
    if (m_bDelay) {
      m_hRs_48_96 = r2_rs_create(m_pMicInfo_Rs->iMicNum, 1, 2, 960) ;
    }else{
      m_hRs_48_96 = NULL ;
    }
    
    
    m_pData_Tmp = R2_SAFE_NEW_AR1(m_pData_Tmp, float*, m_pMicInfo_Rs->iMicNum) ;

    m_iLen_Out_Total = 16000 ;
    m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total) ;

    
    
  }
  
  r2mem_rs2::~r2mem_rs2(void){
    
    r2_rs_free(m_hRs_48_16) ;
    
    if (m_bDelay) {
      r2_rs_free(m_hRs_48_96) ;
    }
    
    
    R2_SAFE_DEL_AR1(m_pData_Tmp) ;
    R2_SAFE_DEL_AR2(m_pData_Out) ;
    
  }

  int r2mem_rs2::reset(){
    
    r2_rs_reset(m_hRs_48_16);
    if (m_bDelay) {
      r2_rs_reset(m_hRs_48_96) ;
    }
    
    return 0 ;
  }
  
  int r2mem_rs2::process(float** pData_In, int iLen_In, float**& pData_Out, int& iLen_Out){
    
    for (int i = 0 ; i < m_pMicInfo_Rs->iMicNum ; i ++) {
      m_pData_Tmp[i] = pData_In[m_pMicInfo_Rs->pMicIdLst[i]] ;
    }
    
    float** pData_Tmp = m_pData_Tmp ;
    int iLen_Tmp = iLen_In ;
    
    
    
    if (m_bDelay) {
      r2_rs_process_float(m_hRs_48_96, (const float**)pData_Tmp, iLen_Tmp, pData_Tmp, iLen_Tmp) ;
      iLen_Tmp = iLen_Tmp / 2 ;
      
      for (int i = 0 ; i < m_pMicInfo_Rs->iMicNum; i ++) {
        if (m_pMicInfo_Rs->pMicIdLst[i] % 2 == 0) {
          for (int j = 0 ; j < iLen_Tmp ; j ++) {
            pData_Tmp[i][j] = pData_Tmp[i][j * 2 + 1] ;
          }
        }else{
          for (int j = 0 ; j < iLen_Tmp ; j ++) {
            pData_Tmp[i][j] = pData_Tmp[i][j * 2] ;
          }
        }
      }
      
    }
    
    r2_rs_process_float(m_hRs_48_16, (const float**)pData_Tmp, iLen_Tmp, pData_Tmp, iLen_Tmp) ;
    if (iLen_Tmp > m_iLen_Out_Total) {
      m_iLen_Out_Total = iLen_Tmp * 2 ;
      R2_SAFE_DEL_AR2(m_pData_Out);
      m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total) ;
    }
    
    for (int i = 0 ; i < m_pMicInfo_Rs->iMicNum; i ++) {
      memcpy(m_pData_Out[m_pMicInfo_Rs->pMicIdLst[i]], pData_Tmp[i], sizeof(float) * iLen_Tmp) ;
    }
    
    pData_Out = m_pData_Out ;
    iLen_Out = iLen_Tmp ;
    
    return 0 ;
  }
  




