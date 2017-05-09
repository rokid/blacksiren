//
//  r2mem_vbv2.cpp
//  r2ad2
//
//  Created by hadoop on 1/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#include "legacy/r2mem_vbv2.h"
#include "legacy/zvtapi.h"

r2mem_vbv2::r2mem_vbv2(const char* pWorkDir, int iMicNum, float* pMicPos, float* pMicDelay, r2_mic_info* pMicInfo_In,  int iMicId_Vad,
                       r2_mic_info* pMicInfo_Bf, r2mem_sl3* pMem_Sl3, const char* pCfgPath){
  
  m_iMicNum = iMicNum ;
  m_iFrmSize = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS ;
  
  //sl + bf
  m_pMem_SlBf2 = R2_SAFE_NEW(m_pMem_SlBf2, r2mem_slbf2, iMicNum, pMicPos, pMicDelay,  pMicInfo_Bf, pMem_Sl3);
  
  //vt
  m_fAzimuth_Vt = 0.0f ;
  m_fElevation_Vt = 0.0f ;
  m_pMem_VtBf = R2_SAFE_NEW(m_pMem_VtBf, r2mem_bf, iMicNum, pMicPos, pMicDelay, pMicInfo_Bf);
  m_pMem_VtData = R2_SAFE_NEW(m_pMem_VtData, r2mem_buff_f, R2_AUDIO_SAMPLE_RATE * 2) ;
  
  //vad
  m_pMem_AudBuff = R2_SAFE_NEW(m_pMem_AudBuff, ZAudBuff, m_iMicNum, R2_AUDIO_SAMPLE_RATE * 10);
  
  //sl3
  m_pMem_Sl3 = pMem_Sl3 ;
  
  //vt
  m_pMem_Vt = R2_SAFE_NEW(m_pMem_Vt, r2mem_vt, pWorkDir);
  m_pMem_Vt->setdosecslcallback(dosecsl_callback_proxy, this);
  m_pMem_Vt->setcheckdirtycallback(checkdirty_callback_proxy, this) ;
  m_pMem_Vt->setgetdatacallback(getdata_callback_proxy, this);
  
  
  m_bPre = false ;
  //m_bAwake = false ;
  m_bAec = false ;
  
  //---------------------------------------
  
#ifdef SLDATA
  m_strDebugFolder =  DEBUG_FILE_LOCATION;
  r2_mkdir(m_strDebugFolder.c_str());
  
  char SlFolder[1024] ;
  sprintf(SlFolder, "%s/SLDATA",m_strDebugFolder.c_str());
  r2_mkdir(SlFolder) ;
#endif
  
  //misa
  
  m_fAzimuth_Vad = 0.0f ;
  m_fElevation_Vad = 0.0f ;
  
  bool bD = r2_getkey_bool(pCfgPath, "r2ssp","r2ssp.aelearning", true);
  unsigned int dMax = r2_getkey_int(pCfgPath, "r2ssp","r2ssp.aelearning.dmax", 6);
  time_t dTimeOut = r2_getkey_int(pCfgPath, "r2ssp","r2ssp.aelearning.dtimeout",60);
  time_t tTimeout = r2_getkey_int(pCfgPath, "r2ssp","r2ssp.aelearning.ttimeout",10);
  m_pAELearning = R2_SAFE_NEW(m_pAELearning,  AELearning, true, dMax, dTimeOut, tTimeout);

}

r2mem_vbv2::~r2mem_vbv2(void){
  
  //Sl
  R2_SAFE_DEL(m_pMem_VtBf);
  R2_SAFE_DEL(m_pMem_VtData);
  
  R2_SAFE_DEL(m_pMem_Vt);
  R2_SAFE_DEL(m_pMem_SlBf2);
  
  R2_SAFE_DEL(m_pMem_AudBuff);
  
  R2_SAFE_DEL(m_pAELearning);
  
}

int r2mem_vbv2::process(float** pData_in, int iLen_in, float fSilShield, float fVtAzimuth, bool bAec, bool bAwake, bool bSleep){
  
  assert(iLen_in == 0 || (iLen_in > 0 && pData_in != NULL)) ;
  
  int iAec = 0 ;
  if (bAec) {
    iAec = 1 ;
  }
  
  m_pMem_AudBuff->PutAudio(pData_in, iLen_in) ;
  //Should before Vt
  if (m_pMem_VtData->m_bWorking) {
    float * pData_VtBf = NULL ; int iLen_VtBf = 0 ;
    m_pMem_VtBf->process(pData_in, iLen_in, pData_VtBf, iLen_VtBf);
    m_pMem_VtData->PutBuff(pData_VtBf, iLen_VtBf);
  }
  
  float * pData_SlBf = NULL ; int iLen_SlBf = 0 ;
  m_pMem_SlBf2->process(pData_in,iLen_in, false, pData_SlBf,iLen_SlBf);
  
  int rt = 0 ;
  
  m_pMem_Vt->setsilshield(fSilShield) ;
  
  int rt_vt =  m_pMem_Vt->process(pData_SlBf, iLen_SlBf, 0, NULL) ;
  if (rt_vt != 0) {
    
    if (m_pMem_Vt->getwordtype() == WORD_AWAKE) {
      
      if (rt_vt & R2_VT_WORD_PRE) {
        ZLOG_INFO("--------------r2mem_vbv_vt_awake_pre") ;
        if (bAwake) {
          rt |= r2mem_vbv_vt_awake_pre ;
        }
      }
      
      if (rt_vt & R2_VT_WORD_DET) {
        ZLOG_INFO("--------------r2mem_vbv_vt_awake");
        if (bAwake) {
          rt |= r2mem_vbv_vt_awake ;
        }
      }
      
      if (rt_vt & R2_VT_WORD_CANCEL) {
        ZLOG_INFO("--------------r2mem_vbv_vt_awake_cancel");
        if (bAwake) {
          rt |= r2mem_vbv_vt_awake_cancel ;
        }
        m_pMem_VtData->Reset();
      }
      
      if (rt_vt & R2_VT_WORD_DET_CMD) {
        ZLOG_INFO("--------------r2mem_vbv_vt_awake_cmd");
        if (bAwake) {
          rt |= r2mem_vbv_vt_awake_cmd ;
        }
        m_pMem_VtData->Reset();
      }
      
      if (rt_vt & R2_VT_WORD_DET_NOCMD) {
        ZLOG_INFO("--------------r2mem_vbv_vt_awake_nocmd");
        if (bAwake) {
          if(bAec){
            ZLOG_INFO("--------------change to cmd in aec condition") ;
            rt |= r2mem_vbv_vt_awake_cmd ;
          }else{
            rt |= r2mem_vbv_vt_awake_nocmd ;
          }
          
        }
        m_pMem_VtData->Reset();
      }
    }
    if (m_pMem_Vt->getwordtype() == WORD_SLEEP) {
      if (rt_vt & R2_VT_WORD_DET) {
        if (bSleep) {
          rt |= r2mem_vbv_vt_sleep ;
        }
        ZLOG_INFO("--------------r2mem_vbv_vt_sleep") ;
        m_pMem_VtData->Reset();
      }
    }
  }
  
  return  rt ;
}

int r2mem_vbv2::getwordpos(int* start, int* end){
  
  m_pMem_Vt->getwordpos(start, end) ;
  *start += m_pMem_SlBf2->getleftfrmnum() ;
  *end += m_pMem_SlBf2->getleftfrmnum() ;
  
  return  0 ;
}

int r2mem_vbv2::reset(){
  
  m_pMem_SlBf2->reset() ;
  m_pMem_AudBuff->Reset() ;
  m_pMem_Vt->reset() ;
  
  return  0 ;
}


int r2mem_vbv2::DegreeChange(float fDegree){
  
  int iDegree = (fDegree - 3.1415936f) * 180 / 3.1415936f + 0.5f  ;
//  if (m_iMicNum == 8) {
//    iDegree = 90 - iDegree ;
//  }
  while (iDegree < 0) {
    iDegree += 360 ;
  }
  while (iDegree > 360) {
    iDegree -= 360 ;
  }
  
  return iDegree ;
}

int r2mem_vbv2::dosecsl_callback_proxy(int nFrmStart_Sil, int nFrmEnd_Sil, int nFrmStart_Vt, int nFrmEnd_Vt, void* param_cb ){
  
  r2mem_vbv2* pVbv2 = (r2mem_vbv2*) param_cb ;
  return pVbv2->my_dosecsl_callback(nFrmStart_Sil, nFrmEnd_Sil, nFrmStart_Vt, nFrmEnd_Vt);

}

int r2mem_vbv2::my_dosecsl_callback(int nFrmStart_Sil, int nFrmEnd_Sil, int nFrmStart_Vt, int nFrmEnd_Vt){
  
  assert(nFrmStart_Sil > nFrmEnd_Sil && nFrmStart_Vt > nFrmEnd_Vt) ;
  
  int iOffset = m_pMem_SlBf2->getleftfrmnum() * m_iFrmSize ;
  
  int iStart_Sil = nFrmStart_Sil * m_iFrmSize + iOffset ;
  int iEnd_Sil = nFrmEnd_Sil * m_iFrmSize + iOffset ;
  int iStart_Vt = nFrmStart_Vt * m_iFrmSize + iOffset ;
  int iEnd_Vt = nFrmEnd_Vt * m_iFrmSize + iOffset ;
  
  float** pData1 = R2_SAFE_NEW_AR2(pData1, float, m_iMicNum, iStart_Sil);
  
  //get data and sl
  m_pMem_AudBuff->GetLastAudio(pData1, iStart_Sil , 0) ;
  m_pMem_Sl3->getsl(iStart_Vt, iEnd_Vt,  m_fAzimuth_Vt, m_fElevation_Vt) ;
  
  //Store to buff
  float * pData3 = NULL ;
  int iLen3 = 0 ;
  m_pMem_VtBf->reset();
  m_pMem_VtBf->steer(m_fAzimuth_Vt, m_fElevation_Vt);
  m_pMem_VtBf->process(pData1, iStart_Sil, pData3, iLen3);
  
  //assert(m_pMem_VtData->m_bWorking == false);
  m_pMem_VtData->m_bWorking = true ;
  m_pMem_VtData->PutBuff(pData3, iLen3);
  
  ZLOG_INFO("--------------Second BF: %s", m_pMem_VtBf->getinfo_sl());
  
  R2_SAFE_DEL_AR2(pData1);
  
  return  0 ;
  
}


bool r2mem_vbv2::checkdirty_callback_proxy(void* param_cb){
  
  r2mem_vbv2* pVbv2 = (r2mem_vbv2*) param_cb ;
  return  pVbv2->my_checkdirty_callback();
  
}

bool r2mem_vbv2::my_checkdirty_callback(){
  
  bool bOK = m_pAELearning->check(AELearning::getIndexByDegree(DegreeChange(m_fAzimuth_Vt)));
  if (bOK) {
    ZLOG_INFO("--------------Dirty Check Successfully");
    return false ;
  }else{
    ZLOG_INFO("--------------Dirty Check Failed");
    return true ;
  }
}

int r2mem_vbv2::getdata_callback_proxy(int nFrmStart, int nFrmEnd, float* pData, void* param_cb){
  
  r2mem_vbv2* pVbv2 = (r2mem_vbv2*) param_cb ;
  return  pVbv2->my_getdata_callback(nFrmStart, nFrmEnd, pData );
  
}

int r2mem_vbv2::my_getdata_callback(int nFrmStart, int nFrmEnd, float* pData){
  
  int iOffset = m_pMem_SlBf2->getleftfrmnum() * m_iFrmSize ;
  int iStart = nFrmStart * m_iFrmSize + iOffset ;
  int iEnd = nFrmEnd * m_iFrmSize  + iOffset ;
  
  m_pMem_VtData->GetBuff(pData, iStart , iEnd) ;
  
  return  0 ;
  
}

const char* r2mem_vbv2::getvadslinfo(){
  
  char info[256];
  int iAzimuth = (m_fAzimuth_Vad - 3.1415936f) * 180 / 3.1415936f + 0.1f ;
//  if (m_iMicNum == 8) {
//    iAzimuth = 90 - iAzimuth ;
//  }
  while (iAzimuth < 0) {
    iAzimuth += 360 ;
  }
  while (iAzimuth >= 360) {
    iAzimuth -= 360 ;
  }
  sprintf(info,"%5f %5f",(float)iAzimuth , m_fElevation_Vad * 180 / 3.1415936f);
  m_strVadSlInfo = info ;
  return  m_strVadSlInfo.c_str() ;
  
}
