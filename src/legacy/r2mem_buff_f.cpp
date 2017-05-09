//
//  r2mem_buff_f.cpp
//  r2ad2
//
//  Created by hadoop on 9/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#include "legacy/r2mem_buff_f.h"

r2mem_buff_f::r2mem_buff_f(int size){
  
  m_bWorking = false  ;
  m_iCurSize = 0 ;
  m_iTotalSize = size ;
  m_pData = R2_SAFE_NEW_AR1(m_pData, float, m_iTotalSize);
  
}

r2mem_buff_f::~r2mem_buff_f(void){
  
  R2_SAFE_DEL_AR1(m_pData) ;
  
}

int r2mem_buff_f::PutBuff(const float* pData , int iDataNum){
  
  
  if (m_iCurSize + iDataNum > m_iTotalSize) {
    m_iTotalSize = (m_iCurSize + iDataNum) * 2 ;
    float* pTmp = R2_SAFE_NEW_AR1(pTmp, float, m_iTotalSize);
    memcpy(pTmp, m_pData, sizeof(float) * m_iCurSize) ;
    R2_SAFE_DEL_AR1(m_pData) ;
    m_pData = pTmp ;
  }

  memcpy(m_pData + m_iCurSize , pData, sizeof(float) * iDataNum);
  m_iCurSize += iDataNum ;
  
  return 0 ;
  
}

int r2mem_buff_f::GetBuff(float* pData, int start, int end){
  
  if (start <= m_iCurSize) {
    memcpy(pData, m_pData + m_iCurSize - start , sizeof(float) * (start - end)) ;
  }else{
    memset(m_pData, 0, sizeof(float) * (start - m_iCurSize));
    memcpy(pData + start - m_iCurSize , m_pData, sizeof(float) * (m_iCurSize - end)) ;
    ZLOG_INFO("xfffffffffffxfffffffffffxfffffffffffxfffffffffffxfffffffffffxfffffffffff");
  }
  
  return 0 ;
  
}

int r2mem_buff_f::Reset(){
  
  m_bWorking = false  ;
  m_iCurSize = 0 ;
  
  return 0 ;
}




