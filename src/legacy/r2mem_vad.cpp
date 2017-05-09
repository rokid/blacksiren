#include "legacy/r2mem_vad.h"

#include "legacy/r2math.h"
#include "r2ssp.h"

r2mem_vad::r2mem_vad(int iMicNum, r2_mic_info* pMicInfo_In, int iMicId_Vad)
{
  
  m_pMicInfo_In = pMicInfo_In ;
  
  m_iMicNum = iMicNum ;
  m_iMicId_Vad = iMicId_Vad ;
  
  m_iFrmSize = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS ;
  
  m_iLen_In = 0 ;
  m_iLen_In_Total = m_iFrmSize * 100 ;
  m_pData_In = R2_SAFE_NEW_AR2(m_pData_In,float,iMicNum,m_iLen_In_Total);
  
  
  m_iLen_Out = 0 ;
  m_iLen_Out_Total = m_iFrmSize * 100 ;
  m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out,float,iMicNum,m_iLen_Out_Total);
  
  m_iBlockNum = 1000 ;
  m_iBlockPos = 0 ;
  m_iLastFrame = 0 ;
  m_iCurFrame = 0 ;
  m_pData_Block = R2_SAFE_NEW_AR2(m_pData_Block,float,iMicNum,m_iBlockNum * m_iFrmSize);
  
  m_iVadState = 0 ;
  
  m_hEngine_Vad = VD_NewVad(1);
  
  //        int nMinVocFrmeNum = 30 ;
  //        VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINVOCFRAMENUM, &nMinVocFrmeNum);
  //        float fMinVocRatio = 0.67f;
  //        VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINVOCRATIO, &fMinVocRatio);
  //
  //        int nMinSpeechFrameNum = 50 ;
  //       VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINSPEECHFRAMENUM, &nMinSpeechFrameNum);
  //
  int nMinSilFrameNum = 60;
  VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINSILFRAMENUM, &nMinSilFrameNum);
  //        float fMinSilRatio = 0.8f;
  //        VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINSILRATIO, &fMinSilRatio);
  
  //    float	l_fBaseRange = 1.2f; // 1.5f;
  //    VD_SetVadParam(m_hEngine_Vad, VD_PARAM_BASERANGE, &l_fBaseRange);
  //    float	l_fMinDynaRange = 2.5f; //3.5f;
  //    VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINDYNARANGE, &l_fMinDynaRange);
  //    float	l_fMaxDynaRange = 4.0f; //6.0f;
  //    VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MAXDYNARANGE, &l_fMaxDynaRange);
  
  //    int l_bEnablePitch = 0 ;
  //    VD_SetVadParam(m_hEngine_Vad, VD_PARAM_ENBLEPITCH, &l_bEnablePitch);
  
//  float fVal = 4000.0;
//  VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINAECENERGY, &fVal);
//  int nVal = 100000;
//  VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MAXENERGY, &nVal);
  
  
  //ns
  m_hEngine_Ns = r2ssp_ns_create();
  r2ssp_ns_init(m_hEngine_Ns, 16000);
  r2ssp_ns_set_mode(m_hEngine_Ns, 2);
  
  m_pData_Ns = R2_SAFE_NEW_AR1(m_pData_Ns, float, m_iFrmSize);
  
}

r2mem_vad::~r2mem_vad(void){
  
  R2_SAFE_DEL_AR2(m_pData_In);
  R2_SAFE_DEL_AR2(m_pData_Out);
  R2_SAFE_DEL_AR2(m_pData_Block);
  
  R2_SAFE_DEL_AR1(m_pData_Ns);
  
  VD_DelVad(m_hEngine_Vad);
  r2ssp_ns_free(m_hEngine_Ns);
}

int r2mem_vad::process(float** pData_In, int iLen_In, int bIsEnd, int bIsAec,  float**& pData_Out, int& iLen_Out){
  
  assert(iLen_In == 0 || (iLen_In > 0 && pData_In != NULL)) ;
  R2_MEM_ASSERT(this,0);
  
  assert(iLen_In % m_iFrmSize == 0) ;
  
  AddInData(pData_In, 0, iLen_In) ;
  
  int iFrmNum = m_iLen_In / m_iFrmSize ;
  
  m_iLen_In = 0 ;
  m_iLen_Out = 0 ;
  
  int rt = 0 ;
  
  for (int i = 0 ; i < iFrmNum ; i ++ ) {
    //Store to Block
    m_iBlockPos = (m_iBlockPos + 1) % m_iBlockNum ;
    m_iCurFrame ++ ;
    for (int j = 0 ; j < m_pMicInfo_In->iMicNum ; j ++) {
      int iMicId = m_pMicInfo_In->pMicIdLst[j] ;
      memcpy(m_pData_Block[iMicId] + m_iBlockPos * m_iFrmSize, m_pData_In[iMicId] + i * m_iFrmSize , sizeof(float) * m_iFrmSize) ;
    }
  }
  
  pData_Out = m_pData_Out ;
  iLen_Out = m_iLen_Out ;
  
  return rt ;
  
}

int r2mem_vad::reset(){
  
  m_iVadState = 0 ;
  m_iBlockPos = 0 ;
  m_iLen_In = 0 ;
  m_iLen_Out = 0 ;
  VD_RestartVad(m_hEngine_Vad);
  
  
  return 0 ;
}


float  r2mem_vad::getenergy_Lastframe(){
  
  return VD_GetLastFrameEnergy(m_hEngine_Vad);
}

float  r2mem_vad::getenergy_Threshold(){
  
  return VD_GetThresholdEnergy(m_hEngine_Vad);
}

int r2mem_vad::getlastData(float** pData_Out, int iStart, int iEnd){
  
  assert(iStart % m_iFrmSize == 0);
  assert(iEnd % m_iFrmSize == 0);
  
  int iStartFrm = iStart / m_iFrmSize ;
  int iEndFrm = iEnd / m_iFrmSize ;
  
  for (int i = 0 ; i < iStartFrm - iEndFrm ; i ++) {
    int pos = m_iBlockPos - iStartFrm + i ;
    while (pos < 0) {
      pos += m_iBlockNum ;
    }
    for (int j = 0; j < m_iMicNum ; j ++) {
      memcpy(pData_Out[j] + i * m_iFrmSize, m_pData_Block[j] + pos * m_iFrmSize , sizeof(float) * m_iFrmSize);
    }
  }
  return 0 ;
}

int r2mem_vad::AddInData(float** pData_In, int iOffset, int iLen_In){
  
  if (iLen_In + m_iLen_In > m_iLen_In_Total) {
    m_iLen_In_Total = (iLen_In + m_iLen_In) * 2 ;
    float** pTmp = m_pData_In ;
    m_pData_In = R2_SAFE_NEW_AR2(m_pData_In, float, m_iMicNum, m_iLen_In_Total);
    for (int i = 0 ; i < m_iMicNum; i ++) {
      memcpy(m_pData_In[i], pTmp[i], sizeof(float) * m_iLen_In);
    }
    R2_SAFE_DEL_AR2(pTmp);
  }
  
  for (int i = 0 ; i < m_iMicNum ; i ++) {
    memcpy(m_pData_In[i] + m_iLen_In, pData_In[i] + iOffset, iLen_In * sizeof(float));
  }
  m_iLen_In += iLen_In ;
  
  return  0 ;
  
  
}


int r2mem_vad::AddOutData(float** pData_Out, int iOffset, int iLen_Out){
  
  if (m_iLen_Out + iLen_Out > m_iLen_Out_Total ) {
    m_iLen_Out_Total = (iLen_Out + m_iLen_Out) * 2 ;
    float** pTmp = m_pData_Out ;
    m_pData_Out = R2_SAFE_NEW_AR2(m_pData_Out, float, m_iMicNum, m_iLen_Out_Total);
    for (int i = 0 ; i < m_iMicNum; i ++) {
      memcpy(m_pData_Out[i], pTmp[i], sizeof(float) * m_iLen_Out);
    }
    R2_SAFE_DEL_AR2(pTmp);
  }
  
  for (int i = 0 ; i < m_iMicNum ; i ++) {
    memcpy(m_pData_Out[i] + m_iLen_Out, pData_Out[i] + iOffset, iLen_Out * sizeof(float));
  }
  m_iLen_Out += iLen_Out ;
  
  return  0 ;
  
  
}

int r2mem_vad::fixmic(r2_mic_info* pMicInfo_Err){
  
  //Fix Mic Info
  bool bExist = false ;
  for (int j = 0 ; j < pMicInfo_Err->iMicNum ; j ++) {
    if (m_iMicId_Vad == pMicInfo_Err->pMicIdLst[j]) {
      bExist = true ;
      break ;
    }
  }
  if (bExist) {
    for (int i = 0 ; i < m_pMicInfo_In->iMicNum ; i ++) {
      bExist = false ;
      for (int j = 0 ; j < pMicInfo_Err->iMicNum ; j ++) {
        if (m_pMicInfo_In->pMicIdLst[i] == pMicInfo_Err->pMicIdLst[j]) {
          bExist = true ;
          break ;
        }
      }
      if (!bExist) {
        m_iMicId_Vad = m_pMicInfo_In->pMicIdLst[i] ;
      }
    }
  }
  
  return  0 ;
  
}
