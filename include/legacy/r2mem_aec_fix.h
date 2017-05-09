//
//  r2mem_aec_fix.h
//  r2ad2
//
//  Created by hadoop on 1/20/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2mem_aec_fix__
#define __r2ad2__r2mem_aec_fix__

#include "r2math.h"

class r2mem_aec_fix
{
public:
  r2mem_aec_fix(int iMicNum, float fAecShield, r2_mic_info* pMicInfo_AecRef);
public:
  ~r2mem_aec_fix(void);
  
  int process(float** pData_In, int& iLen_In);
  
protected:
  
  int m_iMicNum ;
  
  float m_fAecShield ;
  r2_mic_info* m_pMicInfo_AecRef ;
  
  int m_iNoAecDur_Total ;
  int m_iNoAecDur_Cur ;
  
  
  
  
};


#endif /* __r2ad2__r2mem_aec_fix__ */
