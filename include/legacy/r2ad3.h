//
//  r2ad3.h
//  r2ad2
//
//  Created by hadoop on 8/4/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#ifndef __r2ad2__r2ad3api__
#define __r2ad2__r2ad3api__

#ifdef __cplusplus
extern "C" {
#endif
  
  /** task handle */
  typedef void* r2ad3_htask;
  
  /************************************************************************/
  /** System Init	, Exit
   */
  int r2ad3_sysinit(const char* pWorkDir);
  int r2ad3_sysexit();
  
  /************************************************************************/
  /** Task Alloc , Free
   */
  r2ad3_htask r2ad3_create();
  int r2ad3_free(r2ad3_htask htask);
  
  // reset
  int r2ad3_reset(r2ad3_htask htask);
  
  /************************************************************************/
  /** Main Procedure
   */
  int r2ad3_putdata(r2ad3_htask htask, char* pData_In, int iLen_In);
  int r2ad3_putdata2(r2ad3_htask htask, char* pData_In, int iLen_In, char* &pData_Out, int &iLen_Out);
  
  int r2ad3_getdatalen(r2ad3_htask htask);
  int r2ad3_getdata(r2ad3_htask htask, char* pData_Out, int iLen_Out);
  
  
  
#ifdef __cplusplus
};
#endif


#endif /* __r2ad2__r2ad3__ */
