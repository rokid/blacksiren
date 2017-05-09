//
//  zslapi.h
//  r2vt4
//
//  Created by hadoop on 3/6/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#ifndef __r2vt4__zslapi__
#define __r2vt4__zslapi__

#ifdef __cplusplus
extern "C" {
#endif
  
  /** task handle */
  typedef long long r2_sl_htask ;
  
  /************************************************************************/
  /** System Init	, Exit
   */
  int r2_sl_sysinit();
  int r2_sl_sysexit();
  
  /************************************************************************/
  /** Task Alloc , Free
   */
  r2_sl_htask r2_sl_create(int iMicNum, float* pMicPos, float* pMicI2sDelay);
  int r2_sl_free(r2_sl_htask hTask);
  
  /************************************************************************/
  /** R2 SL
   *
   * pWaveBuff: iWavLen * iCn
   * iFlag: 0 End
   */
  float r2_sl_put_data(r2_sl_htask hTask, float** pfDataBuff, int iDataLen);
  float r2_sl_get_candidate(r2_sl_htask hTask, int iStartPos, int iEndPos, float *pfCandidates, int iCandiNum);
  
  int r2_sl_reset(r2_sl_htask hTask);
  
#ifdef __cplusplus
};
#endif



#endif /* __r2vt4__zslapi__ */
