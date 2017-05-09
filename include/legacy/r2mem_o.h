//
//  r2mem_o.h
//  r2ad
//
//  Created by hadoop on 10/23/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#ifndef __r2ad__r2mem_o__
#define __r2ad__r2mem_o__

#include "r2math.h"

enum r2_out_type{
  r2_out_int_32 = 1,
  r2_out_float_32,
  r2_out_short_16
};

class r2mem_o
{
public:
  r2mem_o(int iMicNum, r2_out_type iOutType, r2_mic_info* pMicInfo_Out);
public:
  ~r2mem_o(void);
  
public:
  int reset();
  int process(float** pData_In, int iLen_In, char* &pData_Out, int& iLen_Out);
  
public:
  
  int m_iMicNum ;
  r2_out_type m_iOutType;
  r2_mic_info* m_pMicInfo_Out ;
  
  int m_iLen_Out ;
  int m_iLen_Out_Total ;
  char* m_pData_Out ;
  
};

#endif /* defined(__r2ad__r2mem_o__) */
