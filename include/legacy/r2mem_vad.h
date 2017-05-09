#ifndef R2_MEM_VAD_H
#define R2_MEM_VAD_H

#include "NNVadIntf.h"
#include "r2ssp.h"
#include "r2math.h"

#ifndef r2vad_audio_begin
#define  r2vad_audio_begin		0x0001
#define  r2vad_audio			0x0002
#define  r2vad_audio_end		0x0004
#endif


class r2mem_vad
{
public:
  r2mem_vad(int iMicNum, r2_mic_info* pMicInfo_In, int iMicId_Vad);
public:
  ~r2mem_vad(void);
  
  int process(float** pData_In, int iLen_In, int bIsEnd, int bIsAec, float**& pData_Out, int& iLen_Out);
  
  int reset();
  float  getenergy_Lastframe();
  float  getenergy_Threshold();
  int getlastData(float** pData_Out, int iStart, int iEnd);
  
  int fixmic(r2_mic_info* pMicInfo_Err) ;
  
protected:
  int AddInData(float** pData_In, int iOffset, int iLen_In);
  int AddOutData(float** pData_Out, int iOffset, int iLen_Out);
  
public:
  
  r2_mic_info*  m_pMicInfo_In ;
  
  int m_iMicNum ;
  int m_iMicId_Vad ;
  int m_iFrmSize ;
  
  int m_iLen_In ;
  int m_iLen_In_Total ;
  float ** m_pData_In ;
  
  int m_iLen_Out ;
  int m_iLen_Out_Total ;
  float ** m_pData_Out ;
  
  //Loop
  int m_iBlockNum ;
  int m_iBlockPos ;
  int m_iLastFrame ;
  int m_iCurFrame ;
  float ** m_pData_Block ;
  
  int m_iVadState ;
  
  //vad
  VD_HANDLE m_hEngine_Vad ;
  
  r2ssp_handle  m_hEngine_Ns ;
  float*  m_pData_Ns ;
  
};


#endif