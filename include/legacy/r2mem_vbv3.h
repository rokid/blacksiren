//
//  r2mem_vbv3.h
//  r2ad3
//
//  Created by hadoop on 5/10/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#ifndef __r2ad3__r2mem_vbv3__
#define __r2ad3__r2mem_vbv3__

#include "r2math.h"
#include "zvbvapi.h"


class r2mem_vbv3
{
public:
  r2mem_vbv3(int iMicNum, float* pMicPos, float* pMicDelay, r2_mic_info* pMicInfo_Bf, const char* pVtNnetPath, const char* pVtPhoneTablePath);
  
public:
  ~r2mem_vbv3(void);
  
public:
  
  int SetWords(const WordInfo* pWordLst, int iWordNum);
  int GetWords(const WordInfo** pWordLst, int* iWordNum);
  
  int Process(float** pWavBuff, int iWavLen, bool bDirtyReset, bool bAec, bool bAwake, bool bSleep, bool bHotWord);
  int GetDetWordInfo(const WordInfo** pWordInfo, const WordDetInfo** pWordDetInfo);
  
  int GetLastAudio(float** pAudBuff, int iStart, int iEnd);
  int GetEn_LastFrm();
  int GetEn_Shield();
  
  int GetRealSl(int iFrmNum, float pSlInfo[3]);
  
  int reset();
  
public:
  int InitVbvEngine();
  
public:
  int m_iMicNum ;
  float* m_pMicPos;
  float* m_pMicDelay;
  r2_mic_info* m_pMicInfo_Bf;
  
  int m_iFrmSize ;
  
  float** m_pData_In ;
  std::string m_pVtNnetPath ;
  std::string m_pVtPhoneTablePath ;
  
  // vt sl
  r2_vbv_htask m_hEngine_Vbv ;
  
  bool m_bPre ;
  const WordInfo* m_pWordInfo;
  const WordDetInfo* m_pWordDetInfo ;
  
  bool m_bAec ;
  bool m_bAwake ;
  bool m_bSleep ;
  bool m_bHotWord ;
  
  
  // real sl
  
  
  
};





#endif /* __r2ad3__r2mem_vbv3__ */
