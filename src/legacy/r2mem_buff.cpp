//
//  r2mem_buff.cpp
//  r2ad
//
//  Created by hadoop on 10/23/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#include "legacy/r2mem_buff.h"


r2mem_buff::r2mem_buff(void){
  
  m_iLen = 0  ;
  m_iLen_Cur = 0  ;
  m_iLen_Total = 1000 ;
  
  m_pData = R2_SAFE_NEW_AR1(m_pData, char, m_iLen_Total);
  
}

r2mem_buff::~r2mem_buff(void){
  
  R2_SAFE_DEL_AR1(m_pData);
}

int r2mem_buff::reset(){
  
  m_iLen = 0 ;
  m_iLen_Cur = 0 ;
  
  return  0 ;
}

int r2mem_buff::put(char* pData, int iLen){
  
  R2_MEM_ASSERT(this,0);
  
  if (m_iLen + iLen > m_iLen_Total) {
    m_iLen_Total = (m_iLen + iLen) * 2 ;
    char* pTmp = R2_SAFE_NEW_AR1(pTmp, char, m_iLen_Total);
    memcpy(pTmp, m_pData + m_iLen_Cur, m_iLen - m_iLen_Cur);
    m_iLen_Cur = 0 ;
    m_iLen = m_iLen - m_iLen_Cur ;
    
    R2_SAFE_DEL_AR1(m_pData);
    m_pData = pTmp ;
  }
  
  memcpy(m_pData + m_iLen, pData, iLen);
  m_iLen += iLen ;
  
  return  0 ;
}

int r2mem_buff::getdatalen(){
  
  return  m_iLen - m_iLen_Cur ;
}

int r2mem_buff::getdata(char* pData,int iLen){
  
  
  int ll = r2_min(m_iLen - m_iLen_Cur, iLen);
  memcpy(pData, m_pData + m_iLen_Cur, ll);
  m_iLen_Cur += ll ;
  if (m_iLen_Cur == m_iLen) {
    m_iLen = 0 ;
    m_iLen_Cur = 0 ;
  }
  return 0 ;
}


//ZAudBuff--------------------------------------------------------------
ZAudBuff::ZAudBuff(int iMicNum , int iMaxLen){
  
  m_iMicNum = iMicNum ;
  m_iMaxLen = iMaxLen ;
  m_iCurPos = 0 ;
  
  m_pAudio = R2_SAFE_NEW_AR2(m_pAudio, float, m_iMicNum, m_iMaxLen);
}

ZAudBuff::~ZAudBuff(void){
  
  R2_SAFE_DEL_AR2(m_pAudio);
}

int ZAudBuff::PutAudio(float** pAudBuff, int iLen){
  
  if (iLen >= m_iMaxLen) {
    m_iCurPos = 0 ;
    for (int i = 0 ; i < m_iMicNum ; i ++) {
      memcpy(m_pAudio[i], pAudBuff[i] + iLen - m_iMicNum, sizeof(float) * m_iMaxLen);
    }
  }else{
    int len1 = r2_min(iLen, m_iMaxLen - m_iCurPos) ;
    for (int i = 0 ; i < m_iMicNum ; i ++) {
      memcpy(m_pAudio[i] + m_iCurPos , pAudBuff[i], sizeof(float) * len1);
    }
    m_iCurPos += len1 ;
    if (m_iCurPos == m_iMaxLen) {
      m_iCurPos = 0 ;
    }
    if (iLen > len1) {
      int len2 = iLen - len1 ;
      for (int i = 0 ; i < m_iMicNum ; i ++) {
        memcpy(m_pAudio[i] + m_iCurPos , pAudBuff[i] + len1, sizeof(float) * len2);
      }
      m_iCurPos += len2 ;
    }
  }
  return  0 ;
}

int ZAudBuff::GetLastAudio(float** pAudBuff, int iStart, int iEnd){
  
  assert(iStart < m_iMaxLen);
  assert(iEnd < m_iMaxLen);
  
  if (iStart < m_iCurPos) {
    int iLen = iStart  - iEnd ;
    for (int i = 0 ; i < m_iMicNum  ; i ++) {
      memcpy(pAudBuff[i], m_pAudio[i] + m_iCurPos - iStart, sizeof(float) * iLen);
    }
  }else{
    if (iEnd > m_iCurPos) {
      int iLen = iStart  - iEnd ;
      for (int i = 0 ; i < m_iMicNum  ; i ++) {
        memcpy(pAudBuff[i], m_pAudio[i] + m_iMaxLen + m_iCurPos - iStart, sizeof(float) * iLen);
      }
    }else{
      int iLen1 = iStart - m_iCurPos ;
      int iLen2 = m_iCurPos - iEnd ;
      for (int i = 0 ; i < m_iMicNum ; i ++) {
        memcpy(pAudBuff[i], m_pAudio[i] + m_iMaxLen - iLen1, sizeof(float) * iLen1);
        memcpy(pAudBuff[i] + iLen1, m_pAudio[i], sizeof(float) * iLen2) ;
      }
    }
  }
  return  0 ;
}

int ZAudBuff::Reset(){
  
  for (int i = 0 ; i < m_iMicNum ; i ++) {
    memset(m_pAudio[i], 0, sizeof(float) * m_iMaxLen) ;
  }
  m_iCurPos = 0 ;
  return 0 ;
}
