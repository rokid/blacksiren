//
//  r2mem_vad2.h
//  r2audio
//
//  Created by hadoop on 8/5/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#ifndef __r2audio__r2mem_vad2__
#define __r2audio__r2mem_vad2__

#include "NNVadIntf.h"
#include "r2math.h"

#ifndef r2vad_audio_begin
#define  r2vad_audio_begin		0x0001
#define  r2vad_audio			0x0002
#define  r2vad_audio_end		0x0004
#endif

class r2mem_vad2
{
public:
  r2mem_vad2(float fBaseRange, float fMinDynaRange, float fMaxDynaRange);
public:
  ~r2mem_vad2(void);
  
  int AddInData(float* pData_In, int iLen_In);
  int AddOutData(float* pData_Out, int iLen_Out);
  
  int process(float* pData_In, int iLen_In, int bIsEnd, int bIsAec, int bForceStart, float*& pData_Out, int& iLen_Out);
  int reset();
  int forcestart(int bIsAec);
  
  float  getenergy_Lastframe();
  float  getenergy_Threshold();
  
  
  int m_iVadEndParam ;
  int m_iVadEndPitchParam ;
  int setvadendparam(int iFrmNum);
  
public:
  int     m_iFrmSize ;
  
  int     m_iLen_In ;
  int     m_iLen_In_Total ;
  float * m_pData_In ;
  
  int     m_iLen_Out ;
  int     m_iLen_Out_Total ;
  float* m_pData_Out ;
  
  //vad
  VD_HANDLE m_hEngine_Vad ;
  int m_iLastFrame ;
  int m_iVadState ;
  
  //No New
  int m_iLen_NoNew ;
  float* m_pData_NoNew ;
  
};


#endif /* defined(__r2audio__r2mem_vad2__) */
