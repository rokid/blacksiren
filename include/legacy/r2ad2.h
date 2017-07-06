//
//  r2ad2api.h
//  r2ad
//
//  Created by hadoop on 10/22/15.
//  Copyright (c) 2015 hadoop. All rights reserved.
//

#ifndef __r2ad__r2ad2api__
#define __r2ad__r2ad2api__

#include <stdio.h>

#ifndef R2AD_MSG
#define R2AD_MSG

enum r2ad_msg {

    r2ad_vad_start = 100,  //with vt word
    r2ad_vad_data,
    r2ad_vad_end,
    r2ad_vad_cancel,

    r2ad_awake_vad_start,
    r2ad_awake_vad_data,
    r2ad_awake_vad_end,

    r2ad_awake_pre,     //with sl info + energy
    r2ad_awake_nocmd,   //with sl info
    r2ad_awake_cmd,     //with sl info
    r2ad_awake_cancel,  //with sl info

    r2ad_sleep,  //sleep_no_cmd sleep_cmd + sleep word info
    r2ad_hotword,  //hot_no_cmd hot_cmd
    r2ad_sr,

    r2ad_debug_audio,
    r2ad_dirty

};


enum r2v_sys_state {
    r2ssp_state_awake = 1,
    r2ssp_state_sleep
};


struct r2ad_msg_block {
    enum  r2ad_msg    iMsgId ;
    int     iMsgDataLen ;
    char*   pMsgData ;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

/** task handle */
typedef void* r2ad2_htask;

/************************************************************************/
/** System Init	, Exit
 */
int r2ad2_sysinit(const char* pWorkDir);
int r2ad2_sysexit();

/************************************************************************/
/** Task Alloc , Free
 */
r2ad2_htask r2ad2_create();
int r2ad2_free(r2ad2_htask htask);

// reset
int r2ad2_reset(r2ad2_htask htask);

/************************************************************************/
/** New Procedure
 */
int r2ad2_putaudiodata2(r2ad2_htask htask, char* pData_In, int iLen_In, int iAecFlag, int iAwakeFlag, int iSleepFlag, int iAsrFlag);
int r2ad2_getmsg2(r2ad2_htask htask, r2ad_msg_block*** pMsgLst, int *iMsgNum);

/************************************************************************/
/** get frm info
 */
float  r2ad2_getenergy_Lastframe(r2ad2_htask htask);
float  r2ad2_getenergy_Threshold(r2ad2_htask htask);

/************************************************************************/
/** set sl dr
 */
int    r2ad2_steer(r2ad2_htask htask, float fAzimuth, float fElevation);


#ifdef __cplusplus
};
#endif


#endif /* defined(__r2ad__r2ad2api__) */
