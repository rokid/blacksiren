//
//  r2mem_vad2.cpp
//  r2audio
//
//  Created by hadoop on 8/5/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#include "legacy/r2mem_vad2.h"
#include "legacy/r2math.h"

r2mem_vad2::r2mem_vad2(float fBaseRange, float fMinDynaRange, float fMaxDynaRange)
{
  m_iFrmSize = R2_AUDIO_FRAME_MS * R2_AUDIO_SAMPLE_RATE / 1000 ;
  m_hEngine_Vad = VD_NewVad(1) ;
  
  m_iLastFrame = 0 ;
  m_iVadState = 0 ;
  
  int MaxSpeechFrameNum = 1000 ;
  VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MAXSPEECHFRAMENUM, &MaxSpeechFrameNum) ;
  
  m_iVadEndParam = 45;
  m_iVadEndPitchParam = 1 ;
  VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINSILFRAMENUM, &m_iVadEndParam);
  VD_SetVadParam(m_hEngine_Vad, VD_PARAM_ENDPITCH_FRAMENUM, &m_iVadEndPitchParam);
  
  m_iLen_In = 0 ;
  m_iLen_In_Total = R2_AUDIO_SAMPLE_RATE * 5 ;
  m_pData_In = R2_SAFE_NEW_AR1(m_pData_In, float, m_iLen_In_Total);
  
  m_iLen_Out = 0 ;
  m_iLen_Out_Total = R2_AUDIO_SAMPLE_RATE * 5 ;
  m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, float, m_iLen_Out_Total);

  
  m_iLen_NoNew = R2_AUDIO_SAMPLE_RATE * 3 ;
  m_pData_NoNew = R2_SAFE_NEW_AR1(m_pData_NoNew, float, m_iLen_NoNew) ;
}

r2mem_vad2::~r2mem_vad2(void){
  
  R2_SAFE_DEL_AR1(m_pData_In);
  R2_SAFE_DEL_AR1(m_pData_Out);
  
  VD_DelVad(m_hEngine_Vad);
  
  R2_SAFE_DEL_AR1(m_pData_NoNew) ;
  
}

int r2mem_vad2::AddInData(float* pData_In, int iLen_In){
  
  if (iLen_In + m_iLen_In > m_iLen_In_Total) {
    m_iLen_In_Total = (iLen_In + m_iLen_In) * 2 ;
    float* pTmp = m_pData_In ;
    m_pData_In = R2_SAFE_NEW_AR1(m_pData_In, float, m_iLen_In_Total);
    memcpy(m_pData_In, pTmp, m_iLen_In * sizeof(float));
    R2_SAFE_DEL_AR1(pTmp);
  }
  
  memcpy(m_pData_In + m_iLen_In, pData_In, iLen_In * sizeof(float));
  m_iLen_In += iLen_In ;
  
  return  0 ;
}

int r2mem_vad2::AddOutData(float* pData_Out, int iLen_Out){
  
  if (m_iLen_Out + iLen_Out > m_iLen_Out_Total ) {
    m_iLen_Out_Total = (iLen_Out + m_iLen_Out) * 2 ;
    float* pTmp = m_pData_Out ;
    m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, float, m_iLen_Out_Total);
    memcpy(m_pData_Out, pTmp, m_iLen_Out * sizeof(float));
    R2_SAFE_DEL_AR1(pTmp);
  }
  
  memcpy(m_pData_Out + m_iLen_Out, pData_Out, iLen_Out * sizeof(float));
  m_iLen_Out += iLen_Out ;
  
  return  0 ;
}

int r2mem_vad2::process(float* pData_In, int iLen_In, int bIsEnd, int bIsAec, int bForceStart, float*& pData_Out, int& iLen_Out){
  
  assert(iLen_In == 0 || (iLen_In > 0 && pData_In != NULL)) ;
  R2_MEM_ASSERT(this,0);
  
  AddInData(pData_In, iLen_In);
  int iFrmNum = m_iLen_In / m_iFrmSize ;
  
  m_iLen_In = m_iLen_In - iFrmNum * m_iFrmSize  ;
  m_iLen_Out = 0 ;
  
  int rt = 0 ;
  
  for (int i = 0 ; i < iFrmNum ; i ++) {
    
    if (m_iVadState == 0){
      if (bForceStart) {
        forcestart(bIsAec);
        setvadendparam(2000);
      }
      
      VD_InputFloatWave(m_hEngine_Vad, m_pData_In + i * m_iFrmSize, m_iFrmSize, bIsEnd , bIsAec);
      
      int iStartFrame_b = VD_GetVoiceStartFrame(m_hEngine_Vad) ;
      if (iStartFrame_b >= 0){
        rt = rt | r2vad_audio_begin ;
        m_iLastFrame = 0 ;
        m_iVadState = 1 ;
        
        int iCurFrame = VD_GetVoiceFrameNum(m_hEngine_Vad);
        if (iCurFrame > m_iLastFrame) {
          float *pData =(float*) VD_GetFloatVoice(m_hEngine_Vad) + m_iLastFrame * m_iFrmSize ;
          int iLen = (iCurFrame - m_iLastFrame) * m_iFrmSize ;
          m_iLastFrame = iCurFrame ;
          AddOutData(pData, iLen);
        }
        
//        i ++ ;
//        if (i < iFrmNum) {
//          m_iLen_In = (iFrmNum - i) * m_iFrmSize ;
//          float* pData_Left = R2_SAFE_NEW_AR1(pData_Left, float, m_iLen_In);
//          memcpy(pData_Left, m_pData_In + i * m_iFrmSize , sizeof(float) * m_iLen_In);
//          memcpy(m_pData_In, pData_Left, sizeof(float) * m_iLen_In);
//          R2_SAFE_DEL_AR1(pData_Left);
//        }
//        break ;
        
      }
      
    }else{
      
      VD_InputFloatWave(m_hEngine_Vad, m_pData_In + i * m_iFrmSize, m_iFrmSize, bIsEnd , bIsAec);
      
      int iCurFrame = VD_GetVoiceFrameNum(m_hEngine_Vad);
      if (iCurFrame > m_iLastFrame) {
        float *pData =(float*) VD_GetFloatVoice(m_hEngine_Vad) + m_iLastFrame * m_iFrmSize ;
        int iLen = (iCurFrame - m_iLastFrame) * m_iFrmSize ;
        m_iLastFrame = iCurFrame ;
        AddOutData(pData, iLen);
      }
      
      int iStopFrame_b = VD_GetVoiceStopFrame(m_hEngine_Vad);
      if (iStopFrame_b > 0){
        rt = rt | r2vad_audio_end ;
        VD_RestartVad(m_hEngine_Vad);
        m_iVadState = 0 ;
        
        i ++ ;
        if (i < iFrmNum) {
          m_iLen_In = (iFrmNum - i) * m_iFrmSize ;
          if (m_iLen_In > m_iLen_NoNew) {
            m_iLen_NoNew = m_iLen_In * 2 ;
            R2_SAFE_DEL_AR1(m_pData_NoNew);
            m_pData_NoNew = R2_SAFE_NEW_AR1(m_pData_NoNew, float, m_iLen_NoNew);
          }
          memcpy(m_pData_NoNew, m_pData_In + i * m_iFrmSize , sizeof(float) * m_iLen_In);
          memcpy(m_pData_In, m_pData_NoNew, sizeof(float) * m_iLen_In);
        }
        break ;
      }
    }
  }
  
  pData_Out = m_pData_Out ;
  iLen_Out = m_iLen_Out ;
  
  if (bForceStart) {
    assert(m_iVadState == 1);
  }
  
  if (m_iLen_In > 0 && iFrmNum > 0) {
    memcpy(m_pData_In, m_pData_In + m_iFrmSize * iFrmNum , m_iLen_In * sizeof(float));
  }

  return rt ;
}


int r2mem_vad2::reset(){
  
  m_iLastFrame = 0 ;
  m_iVadState = 0 ;
  
  //VD_ResetVad(m_hEngine_Vad);
  VD_RestartVad(m_hEngine_Vad);
  
  return 0 ;
}

int r2mem_vad2::forcestart(int bIsAec){
  
  if (bIsAec) {
    VD_SetStart(m_hEngine_Vad,1);
  }else{
    VD_SetStart(m_hEngine_Vad,0);
  }
  return  0 ;
  
}

float  r2mem_vad2::getenergy_Lastframe(){
  
  return VD_GetLastFrameEnergy(m_hEngine_Vad);
}

float  r2mem_vad2::getenergy_Threshold(){
  
  return VD_GetThresholdEnergy(m_hEngine_Vad);
}

int r2mem_vad2::setvadendparam(int iFrmNum){
  
  if (iFrmNum <= 0 ) {
    VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINSILFRAMENUM, &m_iVadEndParam);
    VD_SetVadParam(m_hEngine_Vad, VD_PARAM_ENDPITCH_FRAMENUM, &m_iVadEndPitchParam) ;
  }else{
    
    ZLOG_INFO("---------------------set vad end %d0 ms",iFrmNum);
    VD_SetVadParam(m_hEngine_Vad, VD_PARAM_MINSILFRAMENUM, &iFrmNum);
    int kkk = 0 ;
    VD_SetVadParam(m_hEngine_Vad, VD_PARAM_ENDPITCH_FRAMENUM, &kkk) ;
    
  }
  
  return 0 ;
}



