#ifndef R2_MEM_AEC_H
#define R2_MEM_AEC_H

#include "r2ssp.h"
#include "r2math.h"


class r2mem_aec
{
public:
  r2mem_aec(int iMicNum, r2_mic_info* pMicInfo_Aec, r2_mic_info* pMicInfo_AecRef, r2_mic_info*  m_pCpuInfo_Aec);
public:
  ~r2mem_aec(void);
  
public:
  int reset();
  int process(float** pData_in, int iLen_in, float**& pData_Out, int& iLen_Out);
  int processfrm();
  
public:
  int m_iMicNum ;
  
  r2_mic_info* m_pMicInfo_Aec ;
  r2_mic_info* m_pMicInfo_AecRef ;
  
  int m_iFrmLen_Aec ;
  float * m_pData_Aec_Ref ;
  float * m_pData_Aec_In ;
  float * m_pData_Aec_Out ;
  
  int m_iLen_In ;
  float** m_pData_In ;
  
  int m_iFrmLen_Out ;
  int m_iLen_Out ;
  int m_iLen_Out_Total ;
  float ** m_pData_Out ;
  
  r2ssp_handle m_hEngine_Aec ;
  
  int m_iRt ;
  
  
  
};

#endif