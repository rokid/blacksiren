//
//  r2mem_realsl2.h
//  r2ad2
//
//  Created by hadoop on 10/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2mem_realsl2__
#define __r2ad2__r2mem_realsl2__

#include "r2mem_sl3.h"
#include "r2math.h"

class r2mem_realsl2
{
public:
  r2mem_realsl2(r2mem_sl3* pMem_Sl3, int iTotalFrmNum);
  
public:
  ~r2mem_realsl2(void);
  
  int reset();
  int process(int iLen);
  int getrealsl(float& fAzimuth, float& fElevation);
  
public:
  
  //data
  int   m_iFrmSize ;
  int   m_iFrmNum_Cur ;
  int   m_iFrmNum_Total ;
  
  //split
  float  m_fAzimuth ;
  float  m_fElevation ;
  
  r2mem_sl3* m_pMem_Sl3 ;
  
};



#endif /* __r2ad2__r2mem_realsl2__ */
