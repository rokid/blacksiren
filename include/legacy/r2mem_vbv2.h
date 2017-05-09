//
//  r2mem_vbv2.h
//  r2ad2
//
//  Created by hadoop on 1/20/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2mem_vbv2__
#define __r2ad2__r2mem_vbv2__

#include "r2mem_vad.h"
#include "r2mem_sl3.h"
#include "r2mem_slbf2.h"
#include "r2mem_vt.h"
#include "r2mem_cod.h"
#include "r2mem_bf.h"

#include "r2mem_buff_f.h"
#include "r2mem_buff.h"

#include "AELearning.h"
#include "r2math.h"

//#define  r2mem_vbv_vad_begin		0x0001
//#define  r2mem_vbv_vad_end          0x0002

#define  r2mem_vbv_vt_awake_pre     0x0010
#define  r2mem_vbv_vt_awake_nocmd   0x0020
#define  r2mem_vbv_vt_awake_cmd     0x0040
#define  r2mem_vbv_vt_awake_cancel  0x1000

#define  r2mem_vbv_vt_awake       0x0100
#define  r2mem_vbv_vt_sleep       0x0200
#define  r2mem_vbv_vt_sr          0x0400

#define  r2mem_vbv_dirty          0x2000


#ifndef __ARM_ARCH_ARM__
//#define  SLDATA
#else
//#define  SLDATA
#endif


class r2mem_vbv2
{
public:
  r2mem_vbv2(const char* pWorkDir, int iMicNum, float* pMicPos, float* pMicDelay, r2_mic_info* pMicInfo_In,  int iMicId_Vad,
             r2_mic_info* pMicInfo_Bf, r2mem_sl3* pMem_Sl3, const char* pCfgPath);
  
public:
  ~r2mem_vbv2(void);
  
public:
  int process(float** pData_in, int iLen_in, float fSilShield, float fVtAzimuth, bool bAec, bool bAwake, bool bSleep);
  int reset();
  int getwordpos(int* start, int* end);
  
public:
  int m_iMicNum ;
  int m_iFrmSize ;
  
  // vt sl
  float m_fAzimuth_Vt ;
  float m_fElevation_Vt ;
  r2mem_bf* m_pMem_VtBf ;
  r2mem_buff_f*   m_pMem_VtData ;
  
  // Vad Sl for misa
  const char* getvadslinfo();
  std::string m_strVadSlInfo ;
  float m_fAzimuth_Vad ;
  float m_fElevation_Vad ;
  
public:
  
  //sl & bf
  r2mem_slbf2* m_pMem_SlBf2 ;
  
  //vad
  //r2mem_vad* m_pMem_Vad ;
  ZAudBuff*   m_pMem_AudBuff ;
  r2mem_sl3* m_pMem_Sl3 ;
  
  //vt
  r2mem_vt* m_pMem_Vt ;
  bool m_bPre ;
  //bool m_bAwake ;
  bool m_bAec ;
  
  //misa
  AELearning* m_pAELearning;
  
#ifdef SLDATA
  std::string m_strDebugFolder ;
  FILE* m_pSlFile ;
#endif
  
public:
  
  int DegreeChange(float fDegree);
  
  static bool checkdirty_callback_proxy(void* param_cb);
  bool my_checkdirty_callback();
  
  static int getdata_callback_proxy(int nFrmStart, int nFrmEnd, float* pData, void* param_cb);
  int my_getdata_callback(int nFrmStart, int nFrmEnd, float* pData);
  
  static int dosecsl_callback_proxy(int nFrmStart_Sil, int nFrmEnd_Sil, int nFrmStart_Vt, int nFrmEnd_Vt, void* param_cb );
  int my_dosecsl_callback(int nFrmStart_Sil, int nFrmEnd_Sil, int nFrmStart_Vt, int nFrmEnd_Vt);
  
};

#endif /* __r2ad2__r2mem_vbv2__ */
