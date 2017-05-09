//
//  r2mem_ns.cpp
//  r2ad2
//
//  Created by hadoop on 10/26/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#include "legacy/r2mem_ns.h"

//r2ssp_handle r2ssp_ns_create();
//
//int r2ssp_ns_free(r2ssp_handle hNs);
//
//int r2ssp_ns_init(r2ssp_handle hNs, int nSampleRate);
//
//int r2ssp_ns_set_mode(r2ssp_handle hNs, int nMode);
//
//int r2ssp_ns_process(r2ssp_handle hNs, const float *pFrame, float *pOutFrame);

r2mem_ns::r2mem_ns(int iMicNum, r2_mic_info* pMicInfo_Ns, int iNsMode){
  
  m_iMicNum = iMicNum ;
  m_pMicInfo_Ns = pMicInfo_Ns ;
  m_iFrmSize = 160 ;
  
  m_pHandle = R2_SAFE_NEW_AR1(m_pHandle, r2ssp_handle, pMicInfo_Ns->iMicNum);
  
  for (int i = 0 ; i < m_pMicInfo_Ns->iMicNum ; i ++) {
    m_pHandle[i] = r2ssp_ns_create();
    r2ssp_ns_init(m_pHandle[i], 16000);
    r2ssp_ns_set_mode(m_pHandle[i], iNsMode);
  }
  
  m_iLen_Out_Total = m_iFrmSize * 10 ;
  m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total);
  
}

r2mem_ns::~r2mem_ns(void){
  
  for (int i = 0 ; i < m_pMicInfo_Ns->iMicNum ; i ++) {
    r2ssp_ns_free(m_pHandle[i]);
  }
  
  R2_SAFE_DEL_AR1(m_pHandle);
  R2_SAFE_DEL_AR2(m_pData_Out);
  
}

int r2mem_ns::Process(float** pData_In, int iLen_In, float** &pData_Out, int &iLen_Out){
  
  assert(iLen_In % m_iFrmSize == 0) ;
  
  if (iLen_In > m_iLen_Out_Total ) {
    R2_SAFE_DEL_AR2(m_pData_Out);
    m_iLen_Out_Total = iLen_In * 2 ;
    m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total);
  }
  
  int iFrmNum = iLen_In / m_iFrmSize ;
  for (int i = 0 ; i < m_pMicInfo_Ns->iMicNum ; i ++) {
    int iMicId = m_pMicInfo_Ns->pMicIdLst[i] ;
    for (int j = 0 ; j < iFrmNum ; j ++) {
      r2ssp_ns_process(m_pHandle[i], pData_In[iMicId] + j * m_iFrmSize , m_pData_Out[iMicId] + j * m_iFrmSize);
    }
  }
  
  pData_Out = m_pData_Out ;
  iLen_Out = iLen_In ;
  
  return  0 ;
}

