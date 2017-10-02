//
//  r2mem_vbv3.cpp
//  r2ad3
//
//  Created by hadoop on 5/10/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#include "legacy/r2mem_vbv3.h"


r2mem_vbv3::r2mem_vbv3(int iMicNum, float* pMicPos, float* pMicDelay, r2_mic_info* pMicInfo_Bf, const char* pVtNnetPath, const char* pVtPhoneTablePath){
  
  m_iMicNum = iMicNum ;
  m_pMicPos = R2_SAFE_NEW_AR1(m_pMicPos, float, m_iMicNum * 3);
  memcpy(m_pMicPos, pMicPos, sizeof(float) * m_iMicNum * 3 ) ;
  
  m_pMicDelay = R2_SAFE_NEW_AR1(m_pMicDelay, float, m_iMicNum);
  memcpy(m_pMicDelay, pMicDelay, sizeof(float) * m_iMicNum ) ;
  
  m_iFrmSize = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS ;
  
  m_pMicInfo_Bf = r2_copymicinfo(pMicInfo_Bf);
  
  m_pVtNnetPath = pVtNnetPath ;
  m_pVtPhoneTablePath = pVtPhoneTablePath ;
  
  m_hEngine_Vbv = NULL ;
  m_pData_In = R2_SAFE_NEW_AR1(m_pData_In, float*, iMicNum) ;
  
  InitVbvEngine() ;
}

r2mem_vbv3::~r2mem_vbv3(void){
  
  R2_SAFE_DEL_AR1(m_pMicPos);
  R2_SAFE_DEL_AR1(m_pMicDelay);
  
  R2_SAFE_DEL_AR1(m_pData_In);
  
  r2_vbv_free(m_hEngine_Vbv);
  
  r2_free_micinfo(m_pMicInfo_Bf) ;
  
}

int r2mem_vbv3::SetWords(const WordInfo* pWordLst, int iWordNum){
  
  return r2_vbv_setwords(m_hEngine_Vbv, pWordLst, iWordNum);
  
}

int r2mem_vbv3::GetWords(const WordInfo** pWordLst, int* iWordNum){
  
  return r2_vbv_getwords(m_hEngine_Vbv, pWordLst, iWordNum);
}

int r2mem_vbv3::Process(float** pWavBuff, int iWavLen, bool bDirtyReset, bool bAec, bool bAwake, bool bSleep, bool bHotWord){
  
  if (!m_bPre) {
    m_pWordInfo = NULL ;
    m_pWordDetInfo = NULL ;
  }
  
  for (int i = 0 ; i < m_pMicInfo_Bf->iMicNum ; i ++) {
    m_pData_In[i] = pWavBuff[m_pMicInfo_Bf->pMicIdLst[i]] ;
  }
  
  int iVtFlag = 0 ;
  if (bAec) {
    iVtFlag |= R2_VT_FLAG_AEC ;
  }
  
  int rt_vbv = r2_vbv_process(m_hEngine_Vbv, (const float**)m_pData_In, iWavLen, iVtFlag, bDirtyReset) ;
  
  if (rt_vbv & R2_VT_WORD_PRE) {
    assert(!m_bPre) ;
    m_bPre = true ;
    //ZLOG_INFO("xxxxxxR2_VT_WORD_PRE");
    r2_vbv_getdetwordinfo(m_hEngine_Vbv, &m_pWordInfo, &m_pWordDetInfo) ;
  }
  
  if (rt_vbv & R2_VT_WORD_DET) {
    assert(m_bPre);
    //ZLOG_INFO("xxxxxxR2_VT_WORD_DET");
  }
  
  if (rt_vbv & R2_VT_WORD_CANCEL) {
    assert(m_bPre);
    //ZLOG_INFO("xxxxxxR2_VT_WORD_CANCEL");
    m_bPre = false ;
  }
  
  if (rt_vbv & R2_VT_WORD_DET_CMD) {
    assert(m_bPre);
    //ZLOG_INFO("xxxxxxR2_VT_WORD_DET_CMD");
    m_bPre = false ;
    
  }
  
  if (rt_vbv & R2_VT_WORD_DET_NOCMD) {
    assert(m_bPre);
    //ZLOG_INFO("xxxxxxR2_VT_WORD_DET_NOCMD");
    m_bPre = false ;
    
  }
  
  return rt_vbv ;
}

int r2mem_vbv3::GetDetWordInfo(const WordInfo** pWordInfo, const WordDetInfo** pWordDetInfo){
  
  return r2_vbv_getdetwordinfo(m_hEngine_Vbv, pWordInfo, pWordDetInfo) ;
  
}

int r2mem_vbv3::GetLastAudio(float** pAudBuff, int iStart, int iEnd){
  
  for (int i = 0 ; i < m_pMicInfo_Bf->iMicNum ; i ++) {
    m_pData_In[i] = pAudBuff[m_pMicInfo_Bf->pMicIdLst[i]] ;
  }
  return r2_vbv_getlastaudio(m_hEngine_Vbv, iStart, iEnd, m_pData_In);
}

int r2mem_vbv3::GetEn_LastFrm(){
  
  return r2_vbv_geten_lastfrm(m_hEngine_Vbv);
}

int r2mem_vbv3::GetEn_Shield(){
  
  return r2_vbv_geten_shield(m_hEngine_Vbv);
}

int r2mem_vbv3::GetRealSl(int iFrmNum, float pSlInfo[3]){
  
  return r2_vbv_getsl(m_hEngine_Vbv, iFrmNum * m_iFrmSize , 0, pSlInfo);
}

int r2mem_vbv3::reset(){
  
  return r2_vbv_reset(m_hEngine_Vbv) ;
}

int r2mem_vbv3::InitVbvEngine(){
  
  int iMicNum = m_pMicInfo_Bf->iMicNum ;
  float *pMicPos = R2_SAFE_NEW_AR1(pMicPos, float, iMicNum * 3);
  float *pMicDelay = R2_SAFE_NEW_AR1(pMicDelay, float, iMicNum);
  for (int i = 0 ; i < iMicNum; i ++) {
    int iMicId = m_pMicInfo_Bf->pMicIdLst[i] ;
    memcpy(pMicPos + i * 3, m_pMicPos + iMicId * 3, sizeof(float) * 3);
    pMicDelay[i] = m_pMicDelay[iMicId];
  }
  
  m_hEngine_Vbv = r2_vbv_create(iMicNum, pMicPos, pMicDelay, m_pVtNnetPath.c_str(), m_pVtPhoneTablePath.c_str()) ;
  
  R2_SAFE_DEL_AR1(pMicPos);
  R2_SAFE_DEL_AR1(pMicDelay);
  
  return  0 ;
}






