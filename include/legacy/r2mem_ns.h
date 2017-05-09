//
//  r2mem_ns.h
//  r2ad2
//
//  Created by hadoop on 10/26/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2mem_ns__
#define __r2ad2__r2mem_ns__

#include "r2ssp.h"
#include "r2math.h"

class r2mem_ns
{
public:
  r2mem_ns(int iMicNum, r2_mic_info* pMicInfo_Ns, int iNsMode);
  virtual ~r2mem_ns(void);
  
public:
  int Process(float** pData_In, int iLen_In, float** &pData_Out, int &iLen_Out);
  
protected:
  
  int   m_iMicNum ;
  r2_mic_info* m_pMicInfo_Ns ;
  
  
  int   m_iFrmSize ;
  
  r2ssp_handle*  m_pHandle ;
  
  int m_iLen_Out_Total ;
  float** m_pData_Out ;
  
  
};


#endif /* __r2ad2__r2mem_ns__ */
