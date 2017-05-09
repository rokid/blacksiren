//
//  zvtapi.h
//  r2vt4
//
//  Created by hadoop on 3/6/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#ifndef __r2vt4__zvtapi__
#define __r2vt4__zvtapi__

#ifdef __cplusplus
extern "C" {
#endif
  
  //Pre Det
#define R2_VT_WORD_PRE          0x0001
  //WORD_DET
#define R2_VT_WORD_DET          0x0002
  //Cancel Det
#define R2_VT_WORD_CANCEL       0x0004
  //Det With Cmd
#define R2_VT_WORD_DET_CMD      0x0008
  //Det Without Cmd
#define R2_VT_WORD_DET_NOCMD    0x0010
  
#define R2_VT_WORD_DET_BEST     0x0020
  
  enum WordType{
    WORD_AWAKE = 1 ,
    WORD_SLEEP ,
    WORD_HOTWORD ,
    WORD_OTHER 
  };
  
  /** task handle */
  typedef long long r2_vt_htask ;
  
  /**
   激活算法初始化
   */
  int r2_vt_sysinit(const char* pWorkDir);
  /**
   激活算法退出
   */
  int r2_vt_sysexit();
  
  /**
   *  创建激活算法句柄
   *
   *  @return 激活算法句柄
   */
  r2_vt_htask r2_vt_create(int iCn);
  
  /**
   *  释放激活算法句柄
   *
   *  @param hTask 激活算法句柄
   *
   *  @return 0:成功；1：失败
   */
  int r2_vt_free(r2_vt_htask hTask);
  
  /**
   *  设置激活词（休眠词）
   *
   *  @param hTask 激活算法句柄
   *  @param iType 激活词类别 WORD_AWAKE：激活词；WORD_SLEEP：休眠词；WORD_HOTWORD：热词； WORD_OTHER：保留字段未使用
   *  @param pWord 激活词内容，多个激活词用"#"分开，对于中文来说，激活词为“若琪”和“若小琪”，这里为"若琪|ruo4 qi2#若小琪|ruo4 xiao3 qi2#"
   *               下个版本会用json格式描述，格式待定
   *
   *  @return 0:成功；1：失败
   */
  int r2_vt_setwords(r2_vt_htask hTask, WordType iType, const char* pWord);
  
  /**
   *  获取当前系统的激活词信息
   *
   *  @param hTask 激活算法句柄
   *  @param iType 激活词类别
   *
   *  @return 返回对应激活词类别的激活词内容，和r2_vt_setwords格式对应
   */
  const char* r2_vt_getwords(r2_vt_htask hTask, WordType iType);
  
  /**
   *  激活词处理函数
   *
   *  @param hTask    激活算法句柄
   *  @param pWavBuff 语音内容
   *  @param iWavLen  语音长度
   *  @param iFlag    vad标签，1:这是当前vad的最后一帧
   *
   *  @return 激活事件 0:没有激活词；非0:检测到激活词
   */
  int r2_vt_process(r2_vt_htask hTask, const float** pWavBuff, int iWavLen, int iFlag);
  
  /**
   *  获取激活词的类型，在r2_vt_process为非0时才可调用
   *
   *  @param hTask 激活算法句柄
   *
   *  @return 激活词类型
   */
  WordType r2_vt_getwordtype(r2_vt_htask hTask);
  
  /**
   *  获取激活词内容
   *
   *  @param hTask 激活算法句柄
   *
   *  @return 激活词内容
   */
  const char* r2_vt_getwordcontent(r2_vt_htask hTask);
  
  /**
   *  获取激活词的位置
   *
   *  @param hTask  激活算法句柄
   *  @param iStart 激活词开始点
   *  @param iEnd   激活词结束点
   *
   *  @return 0:成功；1:失败
   */
  int r2_vt_getwordpos(r2_vt_htask hTask, int* iStart, int* iEnd);
  
  /**
   *  激活算法重置
   *
   *  @param hTask 激活算法句柄
   *
   *  @return 0:成功；1:失败
   */
  int r2_vt_reset(r2_vt_htask hTask);
  
  //set sil shield
  int r2_vt_setsilshield(r2_vt_htask hTask, float fSilShield);
  
  //secsl_cb
  typedef int (*dosecsl_callback) (int nFrmStart_Sil, int nFrmEnd_Sil, int nFrmStart_Vt, int nFrmEnd_Vt, void* param_cb);
  int r2_vt_set_dosecsl_cb(r2_vt_htask hTask, dosecsl_callback fun_cb, void* param_cb);
  
  //dirty_cb
  typedef bool (*checkdirty_callback)(void* param_cb);
  int rt_vt_set_checkdirty_cb(r2_vt_htask hTask, checkdirty_callback fun_cb, void* param_cb);
  
  //data_cb
  typedef int (*getdata_callback)(int nFrmStart, int nFrmEnd, float* pData, void* param_cb);
  int rt_vt_set_getdata_cb(r2_vt_htask hTask, getdata_callback fun_cb, void* param_cb);
  
  int r2vt_mem_print();
  
#ifdef __cplusplus
};
#endif


#endif /* __r2vt4__zvtapi__ */
