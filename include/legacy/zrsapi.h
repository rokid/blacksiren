//
//  zrsapi.h
//  r2vt4
//
//  Created by hadoop on 3/6/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#ifndef __r2vt4__zrsapi__
#define __r2vt4__zrsapi__


#ifdef __cplusplus
extern "C" {
#endif
  
  /**
   *  重采样句柄
   */
  typedef long long r2_rs_htask ;
  
  /**
   *  创建重采样句柄
   *
   *  @param iCn     语音通道数
   *  @param iSrIn   源语音采样率
   *  @param iSrOut  目标语音采样率
   *  @param iFrmOut 输出帧长
   *
   *  @return 重采样句柄
   */
  r2_rs_htask r2_rs_create(int iCn, int iSrIn, int iSrOut, int iFrmOut);
  
  /**
   *  释放重采样句柄
   *
   *  @param hTask 重采样句柄
   *
   *  @return 0：成功；1：失败
   */
  int r2_rs_free(r2_rs_htask hTask);
  
  /**
   *  重采样处理
   *
   *  @param hTask   重采样句柄
   *  @param pWavIn  源语音数据
   *  @param iLenIn  源语音数据长度
   *  @param pWavOut 目标语音数据
   *  @param iLenOut 目标语音长度
   *
   *  @return 0：成功；1：失败
   */
  int r2_rs_process_float(r2_rs_htask hTask,const float** pWavIn, const int iLenIn, float** &pWavOut, int &iLenOut);
  
  /**
   *  重置重采样句柄
   *
   *  @param hTask 重采样句柄
   *
   *  @return 0：成功；1：失败
   */
  int r2_rs_reset(r2_rs_htask hTask);
  
#ifdef __cplusplus
};
#endif





#endif /* __r2vt4__zrsapi__ */
