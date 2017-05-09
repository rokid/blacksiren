#include "legacy/r2mem_aec.h"
#include <assert.h>

r2mem_aec::r2mem_aec(int iMicNum, r2_mic_info* pMicInfo_Aec, r2_mic_info* pMicInfo_AecRef, r2_mic_info*  m_pCpuInfo_Aec)
{
  m_iMicNum = iMicNum ;
  
  m_pMicInfo_Aec = pMicInfo_Aec ;
  m_pMicInfo_AecRef = pMicInfo_AecRef ;
  
  //Aec
  m_hEngine_Aec = r2ssp_aec_create(0);
#ifdef __ARM_ARCH_ARM__
  r2ssp_aec_set_thread_affinities(m_hEngine_Aec, m_pCpuInfo_Aec->pMicIdLst, m_pCpuInfo_Aec->iMicNum);
#endif
  r2ssp_aec_init(m_hEngine_Aec,R2_AUDIO_SAMPLE_RATE,m_pMicInfo_Aec->iMicNum,m_pMicInfo_AecRef->iMicNum);
  
  m_iFrmLen_Aec = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_AEC_FRAME_MS ;
  m_pData_Aec_In = R2_SAFE_NEW_AR1(m_pData_Aec_In,float,m_iFrmLen_Aec * m_pMicInfo_Aec->iMicNum);
  m_pData_Aec_Ref = R2_SAFE_NEW_AR1(m_pData_Aec_Ref,float,m_iFrmLen_Aec * m_pMicInfo_AecRef->iMicNum);
  m_pData_Aec_Out = R2_SAFE_NEW_AR1(m_pData_Aec_Out,float,m_iFrmLen_Aec * m_pMicInfo_Aec->iMicNum);
  
  //In
  m_iLen_In = 0 ;
  m_pData_In = R2_SAFE_NEW_AR2(m_pData_In,float,m_iMicNum,m_iFrmLen_Aec);
  
  //Out
  m_iFrmLen_Out = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS ;
  m_iLen_Out = 0 ;
  m_iLen_Out_Total = R2_AUDIO_SAMPLE_RATE ;
  m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out,float,m_iMicNum,m_iLen_Out_Total);
  
  m_iRt = 0 ;
  
}

r2mem_aec::~r2mem_aec(void)
{
  
  R2_SAFE_DEL_AR2(m_pData_Out);
  R2_SAFE_DEL_AR2(m_pData_In);
  
  R2_SAFE_DEL_AR1(m_pData_Aec_In);
  R2_SAFE_DEL_AR1(m_pData_Aec_Ref);
  R2_SAFE_DEL_AR1(m_pData_Aec_Out);
  
  r2ssp_aec_free(m_hEngine_Aec);
  
}

int r2mem_aec::reset(){
  
  m_iLen_In = 0 ;
  m_iLen_Out = 0 ;
  
  return 0 ;
}

int r2mem_aec::process(float** pData_In, int iLen_In, float**& pData_Out, int& iLen_Out){
  
  assert(iLen_In == 0 || (iLen_In > 0 && pData_In != NULL)) ;
  R2_MEM_ASSERT(this,0);
  
  m_iRt = 0 ;
  
  int left = m_iLen_Out % m_iFrmLen_Out ;
  for (int i = 0; i < m_pMicInfo_Aec->iMicNum ; i ++) {
    int iMicId = m_pMicInfo_Aec->pMicIdLst[i];
    memcpy(m_pData_Out[iMicId], m_pData_Out[iMicId] + m_iLen_Out - left, sizeof(float) * left);
  }
  for (int i = 0; i < m_pMicInfo_AecRef->iMicNum ; i ++) {
    int iMicId = m_pMicInfo_AecRef->pMicIdLst[i];
    memcpy(m_pData_Out[iMicId], m_pData_Out[iMicId] + m_iLen_Out - left, sizeof(float) * left);
    
  }
  m_iLen_Out = left ;
  
  if (iLen_In + m_iLen_In + m_iLen_Out > m_iLen_Out_Total) {
    m_iLen_Out_Total = (iLen_In + m_iLen_In + m_iLen_Out) * 2 ;
    float** pTmp = R2_SAFE_NEW_AR2(pTmp,float,m_iMicNum,m_iLen_Out_Total);
    for (int i = 0; i < m_iMicNum ; i ++) {
      memcpy(pTmp[i], m_pData_Out[i] , sizeof(float) * m_iLen_Out);
    }
    R2_SAFE_DEL_AR2(m_pData_Out);
    m_pData_Out = pTmp ;
  }
  
  int cur = 0 , ll = 0 ;
  while (cur < iLen_In) {
    ll = r2_min(m_iFrmLen_Aec - m_iLen_In, iLen_In - cur);
    for (int i = 0; i < m_pMicInfo_Aec->iMicNum ; i ++) {
      int iMicId = m_pMicInfo_Aec->pMicIdLst[i];
      memcpy(m_pData_In[iMicId] + m_iLen_In, pData_In[iMicId] + cur, sizeof(float) * ll);
    }
    for (int i = 0; i < m_pMicInfo_AecRef->iMicNum ; i ++) {
      int iMicId = m_pMicInfo_AecRef->pMicIdLst[i];
      memcpy(m_pData_In[iMicId] + m_iLen_In, pData_In[iMicId] + cur, sizeof(float) * ll);
      
    }
    cur += ll ;
    m_iLen_In += ll ;
    
    if (m_iLen_In == m_iFrmLen_Aec) {
      processfrm() ;
      m_iLen_In = 0 ;
    }
  }
  
  pData_Out = m_pData_Out ;
  iLen_Out = m_iLen_Out / m_iFrmLen_Out * m_iFrmLen_Out ;
  
  return m_iRt ;
}

int r2mem_aec::processfrm(){
  
  const float aaa = 64.0f ;
  
  int iLen1 = m_pMicInfo_AecRef->iMicNum * m_iFrmLen_Aec ;
  int iLen2 = m_pMicInfo_Aec->iMicNum * m_iFrmLen_Aec ;
  
  for (int j = 0 ; j < m_pMicInfo_AecRef->iMicNum ; j ++) {
    int iMicId = m_pMicInfo_AecRef->pMicIdLst[j] ;
    memcpy(m_pData_Aec_Ref + j * m_iFrmLen_Aec, m_pData_In[iMicId], sizeof(float) * m_iFrmLen_Aec);
  }
  
  for (int j = 0 ; j < iLen1 ; j ++) {
    m_pData_Aec_Ref[j] = m_pData_Aec_Ref[j] / aaa ;
  }
  
  r2ssp_aec_buffer_farend(m_hEngine_Aec,m_pData_Aec_Ref,m_iFrmLen_Aec * m_pMicInfo_AecRef->iMicNum );
  
  for (int j = 0 ; j < m_pMicInfo_Aec->iMicNum ; j ++) {
    int iMicId = m_pMicInfo_Aec->pMicIdLst[j] ;
    memcpy(m_pData_Aec_In + j * m_iFrmLen_Aec, m_pData_In[iMicId] , sizeof(float) * m_iFrmLen_Aec);
  }
  
  for (int j = 0 ; j < iLen2 ; j ++) {
    m_pData_Aec_In[j] = m_pData_Aec_In[j] / aaa ;
  }
  
  int rt = r2ssp_aec_process(m_hEngine_Aec,m_pData_Aec_In,m_iFrmLen_Aec * m_pMicInfo_Aec->iMicNum,m_pData_Aec_Out,0);
  if (rt == 0) {
    m_iRt = 1 ;
  }
  
  for (int j = 0 ; j < iLen2 ; j ++) {
    m_pData_Aec_Out[j]  = m_pData_Aec_Out[j] * aaa ;
  }
  
  for (int j = 0 ; j < m_pMicInfo_Aec->iMicNum ; j ++) {
    int iMicId = m_pMicInfo_Aec->pMicIdLst[j] ;
    memcpy(m_pData_Out[iMicId] + m_iLen_Out, m_pData_Aec_Out + j * m_iFrmLen_Aec,  sizeof(float) * m_iFrmLen_Aec);
  }
  
  m_iLen_Out += m_iFrmLen_Aec ;
  
  return 0 ;
  
}
