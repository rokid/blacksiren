//
//  r2mem_bf.cpp
//  r2audio
//
//  Created by hadoop on 7/22/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#include "legacy/r2mem_bf.h"

r2mem_bf::r2mem_bf(int iMicNum, float* pMicPosLst, float* pMicDelay, r2_mic_info* pMicInfo_Bf){
  
  m_iFrmSize = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS ;
  
  m_iMicNum = iMicNum ;
  m_pMicInfo_Bf = r2_copymicinfo(pMicInfo_Bf) ;
  
  m_pMics_Bf = R2_SAFE_NEW_AR1(m_pMics_Bf,float,m_pMicInfo_Bf->iMicNum * 3);
  m_pMicI2sDelay = R2_SAFE_NEW_AR1(m_pMicI2sDelay, float, m_pMicInfo_Bf->iMicNum) ;
  
  for (int i = 0 ; i < m_pMicInfo_Bf->iMicNum ; i ++)	{
    int iMicId = m_pMicInfo_Bf->pMicIdLst[i] ;
    memcpy(m_pMics_Bf + i * 3 , pMicPosLst + iMicId * 3 , sizeof(float) * 3) ;
    m_pMicI2sDelay[i] = pMicDelay[iMicId];
  }
  
  m_hEngine_Bf = r2ssp_bf_create(m_pMics_Bf, m_pMicInfo_Bf->iMicNum);
  
  r2ssp_bf_init(m_hEngine_Bf,R2_AUDIO_FRAME_MS,R2_AUDIO_SAMPLE_RATE);
  r2ssp_bf_set_mic_delays(m_hEngine_Bf, m_pMicI2sDelay, m_pMicInfo_Bf->iMicNum);
  
  memset(m_fSlInfo, 0, sizeof(float) * 3);
  steer(m_fSlInfo[0], m_fSlInfo[1]);
  
  m_pData_Bf = R2_SAFE_NEW_AR1(m_pData_Bf,float,m_pMicInfo_Bf->iMicNum * m_iFrmSize);
  
//  float fSlInfo[3] ;
//  memset(fSlInfo, 0, sizeof(float)*3);
//  steer(fSlInfo);
  
  m_iLen_Out_Total = R2_AUDIO_SAMPLE_RATE ;
  m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, float, m_iLen_Out_Total);
}

r2mem_bf::~r2mem_bf(void)
{
  
  R2_SAFE_DEL_AR1(m_pData_Out);
  
  R2_SAFE_DEL_AR1(m_pData_Bf);
  
  R2_SAFE_DEL_AR1(m_pMics_Bf);
  R2_SAFE_DEL_AR1(m_pMicI2sDelay);
  
  r2ssp_bf_free(m_hEngine_Bf);
  
  r2_free_micinfo(m_pMicInfo_Bf);
}

int r2mem_bf::reset(){
  
  r2ssp_bf_free(m_hEngine_Bf);
  m_hEngine_Bf = r2ssp_bf_create(m_pMics_Bf, m_pMicInfo_Bf->iMicNum);
  
  r2ssp_bf_init(m_hEngine_Bf,R2_AUDIO_FRAME_MS,R2_AUDIO_SAMPLE_RATE);
  r2ssp_bf_set_mic_delays(m_hEngine_Bf, m_pMicI2sDelay, m_pMicInfo_Bf->iMicNum);
  
  memset(m_fSlInfo, 0, sizeof(float) * 3);
  steer(m_fSlInfo[0], m_fSlInfo[1]);
  
  return 0 ;
}

int r2mem_bf::process(float** pData_In, int iLen_In, float*& pData_Out, int& iLen_Out){
  
  assert(iLen_In == 0 || (iLen_In > 0 && pData_In != NULL)) ;
  R2_MEM_ASSERT(this,0);
  
  if (iLen_In > m_iLen_Out_Total) {
    m_iLen_Out_Total = iLen_In * 2 ;
    R2_SAFE_DEL_AR1(m_pData_Out);
    m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, float, m_iLen_Out_Total);
  }
  
  for (int i = 0 ; i < iLen_In ; i += m_iFrmSize){
    for (int j = 0 ; j < m_pMicInfo_Bf->iMicNum ; j ++){
      int iMicId = m_pMicInfo_Bf->pMicIdLst[j];
      memcpy(m_pData_Bf + j * m_iFrmSize, pData_In[iMicId] + i, sizeof(float) * m_iFrmSize);
    }
    r2ssp_bf_process(m_hEngine_Bf, m_pData_Bf, m_iFrmSize * m_pMicInfo_Bf->iMicNum, m_pMicInfo_Bf->iMicNum, m_pData_Out + i);
  }
  
  iLen_Out = iLen_In ;
  pData_Out = m_pData_Out ;
  
  return  0 ;
}

int r2mem_bf::steer(float fAzimuth, float fElevation, int bSteer){
  
  m_fSlInfo[0] = fAzimuth ;
  m_fSlInfo[1] = fElevation ;
  
  if (bSteer > 0) {
    r2ssp_bf_steer(m_hEngine_Bf, m_fSlInfo[0], m_fSlInfo[1], m_fSlInfo[0]  + 3.1415926f, 0);
  }
  
  return 0 ;
}

const char* r2mem_bf::getinfo_sl(){
  
  char info[256];
  int iAzimuth = (m_fSlInfo[0] - 3.1415936f) * 180 / 3.1415936f + 0.1f ;
  while (iAzimuth < 0) {
    iAzimuth += 360 ;
  }
  while (iAzimuth >= 360) {
    iAzimuth -= 360 ;
  }
  
  sprintf(info,"%5f %5f",(float)iAzimuth , m_fSlInfo[1] * 180 / 3.1415936f);
  m_strSlInfo = info ;
  return  m_strSlInfo.c_str() ;
}


bool r2mem_bf::check(float fAzimuth, float fElevation){
  
  float fDelt = fabs(m_fSlInfo[0] - fAzimuth) ;
  if (fDelt > 3.14) {
    fDelt = fabs(fDelt - 6.28f);
  }
  
  if (fDelt > R2_BFSL_MIN_DIS) {
    
    //printf("%f %f %f------------------------------\n",fAzimuth, m_fAzimuth,fabs(fAzimuth - m_fAzimuth));
    
    return false ;
  }else {
    //printf("%f %f %f------------------------------\n",fAzimuth, m_fAzimuth,fabs(fAzimuth - m_fAzimuth));
    return true ;
  }
}

