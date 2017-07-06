//
//  r2ad1api.h
//  r2ad
//
//  Created by hadoop on 10/22/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#ifndef __r2ad__r2ad1api__
#define __r2ad__r2ad1api__

#include <stdio.h>

#define  r2ad1_aec		0x0001

#ifdef __cplusplus
extern "C" {
#endif
  
  /** task handle */
  typedef void* r2ad1_htask;
  
  /************************************************************************/
  /** System Init	, Exit
   */
  int r2ad1_sysinit(const char* pWorkDir);
  int r2ad1_sysexit();
  
  /************************************************************************/
  /** Task Alloc , Free
   */
  r2ad1_htask r2ad1_create();
  int r2ad1_free(r2ad1_htask htask);
  
  // reset
  int r2ad1_reset(r2ad1_htask htask);
  
  /************************************************************************/
  /** Main Procedure
   */
  int r2ad1_putdata(r2ad1_htask htask, char* pData_In, int iLen_In);
  int r2ad1_putdata2(r2ad1_htask htask, char* pData_In, int iLen_In, char* &pData_Out, int &iLen_Out);
  
  int r2ad1_getdatalen(r2ad1_htask htask);
  int r2ad1_getdata(r2ad1_htask htask, char* pData_Out, int iLen_Out);
  
  int r2ad_mem_print();
  
#ifdef __cplusplus
};
#endif


#endif /* defined(__r2ad__r2ad1api__) */
