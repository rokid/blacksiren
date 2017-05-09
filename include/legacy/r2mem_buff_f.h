//
//  r2mem_buff_f.h
//  r2ad2
//
//  Created by hadoop on 9/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2mem_buff_f__
#define __r2ad2__r2mem_buff_f__


#include "r2math.h"


class r2mem_buff_f
{
public:
  r2mem_buff_f(int size);
public:
  ~r2mem_buff_f(void);
  
public:
  int PutBuff(const float* pData , int iDataNum);
  int GetBuff(float* pData, int start, int end);
  int Reset();
  
public:
  bool    m_bWorking ;
  int     m_iCurSize ;
  float*  m_pData ;
  int     m_iTotalSize ;
};

#endif /* __r2ad2__r2mem_buff_f__ */
