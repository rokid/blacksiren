//
//  r2mem_realsl2.cpp
//  r2ad2
//
//  Created by hadoop on 10/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#include "legacy/r2mem_realsl2.h"

r2mem_realsl2::r2mem_realsl2(r2mem_sl3* pMem_Sl3, int iTotalFrmNum){
  
  m_iFrmSize = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS ;
  
  m_iFrmNum_Cur = 0 ;
  m_iFrmNum_Total = iTotalFrmNum ;
  
  m_fAzimuth = 0.0f ;
  m_fElevation = 0.0f ;
  
  m_pMem_Sl3 = pMem_Sl3 ;
  
}

r2mem_realsl2::~r2mem_realsl2(void){
  
  
}

int r2mem_realsl2::reset(){
  
  m_iFrmNum_Cur = 0 ;
  return  0 ;
}

int r2mem_realsl2::process(int iLen){
  
  assert(iLen % m_iFrmSize == 0);
  m_iFrmNum_Cur += (iLen / m_iFrmSize) ;
  
  return 0 ;
}

int r2mem_realsl2::getrealsl(float& fAzimuth, float& fElevation){
  
  if (m_iFrmNum_Cur >= m_iFrmNum_Total) {
    m_pMem_Sl3->getsl(m_iFrmNum_Total * m_iFrmSize, 0, m_fAzimuth, m_fElevation) ;
    m_iFrmNum_Cur = 0 ;
  }
  
  fAzimuth = m_fAzimuth ;
  fElevation = m_fElevation ;
  
  return 0 ;
}




