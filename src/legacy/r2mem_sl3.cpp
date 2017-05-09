//
//  r2mem_sl3.cpp
//  r2ad2
//
//  Created by hadoop on 10/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#include "legacy/r2mem_sl3.h"




r2mem_sl3::r2mem_sl3(int iMicNum, float* pMicPosLst, float* pMicDelay, r2_mic_info* pMicInfo_Sl){
  
  m_iMicNum = iMicNum ;
  
  //sl mic
  m_pMicInfo_Sl = r2_copymicinfo(pMicInfo_Sl) ;
  
  //ns
  m_pMem_Ns = R2_SAFE_NEW(m_pMem_Ns, r2mem_ns, iMicNum, pMicInfo_Sl, 2);
  
  //engine
  m_pMics_Sl = R2_SAFE_NEW_AR1(m_pMics_Sl,float,m_pMicInfo_Sl->iMicNum * 3);
  for (int i = 0 ; i < m_pMicInfo_Sl->iMicNum ; i ++)	{
    int iMicId = m_pMicInfo_Sl->pMicIdLst[i] ;
    m_pMics_Sl[i * 3 ] = pMicPosLst[iMicId*3] ;
    m_pMics_Sl[i * 3 +1] = pMicPosLst[iMicId*3+1] ;
    m_pMics_Sl[i * 3 +2] = pMicPosLst[iMicId*3+2];
  }

  m_pMicI2sDelay = R2_SAFE_NEW_AR1(m_pMicI2sDelay, float, m_pMicInfo_Sl->iMicNum) ;
  for (int i = 0 ; i < m_pMicInfo_Sl->iMicNum; i ++) {
    m_pMicI2sDelay[i] = pMicDelay[m_pMicInfo_Sl->pMicIdLst[i]] ;
  }
  
#ifdef USE_LZHU_SL
  m_hEngine_Sl =  r2_sl_create(m_pMicInfo_Sl->iMicNum, m_pMics_Sl, m_pMicI2sDelay);
#else
  m_hEngine_Sl =  r2_sourceloaction_create(m_pMicI2sDelay, m_pMicInfo_Sl->iMicNum, m_pMics_Sl);
#endif
  
  

  m_pData_Sl = R2_SAFE_NEW_AR1(m_pData_Sl, float*, m_pMicInfo_Sl->iMicNum) ;
  m_pCandidate = R2_SAFE_NEW_AR1(m_pCandidate, float, 2);
  
}

r2mem_sl3::~r2mem_sl3(void){
  
  R2_SAFE_DEL(m_pMem_Ns);
  
  R2_SAFE_DEL_AR1(m_pData_Sl) ;
  R2_SAFE_DEL_AR1(m_pCandidate) ;
  
  R2_SAFE_DEL_AR1(m_pMicI2sDelay);

#ifdef USE_LZHU_SL
  r2_sl_free(m_hEngine_Sl);
#else
  r2_sourcelocation_free(m_hEngine_Sl);
#endif
  
  
  r2_free_micinfo(m_pMicInfo_Sl);
  R2_SAFE_DEL_AR1(m_pMics_Sl);
  
}

int r2mem_sl3::putdata(float** pfDataBuff, int iDataLen){
  
  //ns
  m_pMem_Ns->Process(pfDataBuff, iDataLen, pfDataBuff, iDataLen);
  
  for (int i = 0 ; i < m_pMicInfo_Sl->iMicNum ; i ++)	{
    m_pData_Sl[i] = pfDataBuff[m_pMicInfo_Sl->pMicIdLst[i]] ;
  }
  
#ifdef USE_LZHU_SL
  r2_sl_put_data(m_hEngine_Sl, m_pData_Sl, iDataLen);
#else
  r2_sourcelocation_process_data(m_hEngine_Sl, m_pData_Sl, iDataLen);
#endif
  
  return 0 ;
}

int r2mem_sl3::getsl(int iStartPos, int iEndPos, float& fAzimuth, float& fElevation ){
  
#ifdef USE_LZHU_SL
  r2_sl_get_candidate(m_hEngine_Sl, iStartPos, iEndPos, m_pCandidate, 1);
#else
  r2_sourcelocation_get_candidate(m_hEngine_Sl, iStartPos, iEndPos, m_pCandidate, 1);
#endif
  
  fAzimuth = m_pCandidate[0];
  fElevation = m_pCandidate[1];
  
  //fAzimuth = 3.1415965 * (290 - 180) / 180 ;
  
  return 0 ;
}

int r2mem_sl3::reset(){
  
#ifdef USE_LZHU_SL
  r2_sl_free(m_hEngine_Sl);
  m_hEngine_Sl =  r2_sl_create(m_pMicInfo_Sl->iMicNum, m_pMics_Sl, m_pMicI2sDelay);
#else
  r2_sourcelocation_free(m_hEngine_Sl);
  m_hEngine_Sl =  r2_sourceloaction_create(m_pMicI2sDelay, m_pMicInfo_Sl->iMicNum, m_pMics_Sl);
#endif
  

  
  return 0 ;
}


int r2mem_sl3::fixmic(r2_mic_info* pMicInfo_Err){
  
  //Fix Mic Info
  int iCur = 0 ;
  for (int i = 0 ; i < m_pMicInfo_Sl->iMicNum ; i ++) {
    bool bExit = false ;
    for (int j = 0 ; j < pMicInfo_Err->iMicNum ; j ++) {
      if (m_pMicInfo_Sl->pMicIdLst[i] == pMicInfo_Err->pMicIdLst[j]) {
        bExit = true ;
        break ;
      }
    }
    if (!bExit) {
      if (i != iCur) {
        m_pMicInfo_Sl->pMicIdLst[iCur] = m_pMicInfo_Sl->pMicIdLst[i] ;
        memcpy(m_pMics_Sl + iCur * 3, m_pMics_Sl + i * 3, sizeof(float) * 3) ;
        m_pMicI2sDelay[iCur] = m_pMicI2sDelay[i] ;
      }
      iCur ++ ;
    }
  }
  m_pMicInfo_Sl->iMicNum = iCur ;
  
  //reset bf
  reset() ;
  
  return 0 ;
  
}


