//
//  r2mem_buff.h
//  r2ad
//
//  Created by hadoop on 10/23/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#ifndef __r2ad__r2mem_buff__
#define __r2ad__r2mem_buff__

#include "r2math.h"

class r2mem_buff
{
public:
  r2mem_buff(void);
public:
  ~r2mem_buff(void);
  
public:
  int reset();
  int put(char* pData, int iLen);
  int getdatalen();
  int getdata(char* pData,int iLen);
  
public:
  
  int m_iLen ;
  int m_iLen_Cur ;
  int m_iLen_Total ;
  char* m_pData ;
  
};



//ZAudBuff--------------------------------------------------------------
class ZAudBuff
{
public:
  ZAudBuff(int iMicNum , int iMaxLen);
public:
  ~ZAudBuff(void);
  
public:
  int PutAudio(float** pAudBuff, int iLen);
  int GetLastAudio(float** pAudBuff, int iStart, int iEnd);
  int Reset();
  
public:
  
  int     m_iMicNum ;
  int     m_iMaxLen ;
  
  int     m_iCurPos ;
  float**   m_pAudio ;
  
};

#endif /* defined(__r2ad__r2mem_buff__) */
