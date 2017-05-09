#ifndef R2_MEM_VT_H
#define R2_MEM_VT_H

#include "zvtapi.h"

#include "r2math.h"

class r2mem_vt
{
public:
  r2mem_vt(const char* pWorkDir);
public:
  ~r2mem_vt(void);
  
  int setdosecslcallback(dosecsl_callback fun_cb, void* param_cb);
  int setcheckdirtycallback(checkdirty_callback fun_cb, void* param_cb);
  int setgetdatacallback(getdata_callback fun_cb, void* param_cb);
  
  
  int reset();
  int setsilshield(float fSilShield);
  int process(float* pData_in, int iLen_in, int iFlag, float* pScore);
  
  
  WordType getwordtype();
  int getwordpos(int *start, int *end);
  
public:
  
  r2_vt_htask m_hEngine_Vt ;
  
  
};


#endif
