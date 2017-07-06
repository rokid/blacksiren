//
//  zvbvapi.h
//  r2vt4
//
//  Created by hadoop on 3/9/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#ifndef __r2vt4__zvbvapi__
#define __r2vt4__zvbvapi__

#include "legacy/zvtapi.h"


/** task handle */
typedef long long r2_vbv_htask ;

/**
 *  创建麦克风阵列激活算法句柄
 *
 *  @param iMicNum           麦克风个数
 *  @param pMicPos           麦克风位置
 *  @param pVtNnetPath       激活模型地址，全路径
 *  @param pVtPhoneTablePath 激活词表地址，全路径
 *
 *  @return 麦克风阵列激活算法句柄
 */
r2_vbv_htask r2_vbv_create(int iMicNum, float* pMicPos, float* pMicDelay, const char* pVtNnetPath, const char* pVtPhoneTablePath);

/**
 *  释放麦克风阵列激活算法句柄
 *
 *  @param hTask 麦克风阵列激活算法句柄
 *
 *  @return 0:成功；1：失败
 */
int r2_vbv_free(r2_vbv_htask hTask);

/**
 *  设置激活词（休眠词）
 *
 *  @param hTask    麦克风阵列激活算法句柄
 *  @param pWordLst 激活词列表
 *  @param iWordNum 激活词个数
 *
 *  @return 0:成功；1：失败
 */
int r2_vbv_setwords(r2_vbv_htask hTask, const WordInfo* pWordLst, int iWordNum);

/**
 *  获取当前系统的激活词信息
 *
 *  @param hTask    麦克风阵列激活算法句柄
 *  @param pWordLst 激活词列表地址
 *  @param iWordNum 激活词个数地址
 *
 *  @return 0:成功；1：失败
 */
int r2_vbv_getwords(r2_vbv_htask hTask, const WordInfo** pWordLst, int* iWordNum);

/**
 *  激活词处理函数
 *
 *  @param hTask    麦克风阵列激活算法句柄
 *  @param pWavBuff 语音内容
 *  @param iWavLen  语音长度
 *  @param iVtFlag    AEC:0x0001  VAD_END:0x0002
 *  @param bDirtyReset  for Dirty Algorithm
 *
 *  @return 激活事件 0:没有激活词；非0:检测到激活词
 */
int r2_vbv_process(r2_vbv_htask hTask, const float** pWavBuff, int iWavLen, int iVtFlag, bool bDirtyReset);

/**
 *  获取激活词的检测信息
 *  只有在r2_vbv_process返回非0时调用
 *
 *  @param hTask        麦克风阵列激活算法句柄
 *  @param pWordInfo    激活词的信息
 *  @param pWordDetInfo 激活词的检测信息
 *
 *  @return 0:成功；1:失败
 */
int r2_vbv_getdetwordinfo(r2_vbv_htask hTask, const WordInfo** pWordInfo, const WordDetInfo** pWordDetInfo);

/**
 *  重置麦克风阵列激活算法句柄
 *
 *  @param hTask 麦克风阵列激活算法句柄
 *
 *  @return 0:成功；1:失败
 */
int r2_vbv_reset(r2_vbv_htask hTask);

/**
 *  获取历史数据(复用数据缓存)
 *
 *  @param hTask    麦克风阵列激活算法句柄
 *  @param iStart   开始时间，从当前时刻向前的point个数
 *  @param iEnd     结束时间，从当前时刻向前的point个数， iStart > iEnd
 *  @param pWavBuff 数据buff
 *
 *  @return 0:成功；1:失败
 */
int r2_vbv_getlastaudio(r2_vbv_htask hTask, int iStart, int iEnd, float** pWavBuff) ;

/**
 *  获取寻向信息(复用寻向算法)
 *
 *  @param hTask   麦克风阵列激活算法句柄
 *  @param iStart  开始时间，从当前时刻向前的point个数
 *  @param iEnd    结束时间，从当前时刻向前的point个数， iStart > iEnd
 *  @param pSlInfo 寻向信息
 *
 *  @return 0:成功；1:失败
 */
int r2_vbv_getsl(r2_vbv_htask hTask, int iStart, int iEnd, float pSlInfo[3]);

/**
 *  获取当前帧的能量
 *
 *  @param hTask 麦克风阵列激活算法句柄
 *
 *  @return 能量
 */
float  r2_vbv_geten_lastfrm(r2_vbv_htask hTask);

/**
 *  获取噪声平均能量
 *
 *  @param hTask 麦克风阵列激活算法句柄
 *
 *  @return 能量
 */
float  r2_vbv_geten_shield(r2_vbv_htask hTask);


#endif /* __r2vt4__zvbvapi__ */
