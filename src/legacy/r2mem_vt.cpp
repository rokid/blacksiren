#include "legacy/r2mem_vt.h"

r2mem_vt::r2mem_vt(const char* pWorkDir)
{
  m_hEngine_Vt = r2_vt_create(1);
  
}

r2mem_vt::~r2mem_vt(void)
{
  r2_vt_free(m_hEngine_Vt);
  
}

int r2mem_vt::reset(){
  
  r2_vt_reset(m_hEngine_Vt);
  
  return 0 ;
}


int r2mem_vt::setsilshield(float fSilShield){
  
  return  r2_vt_setsilshield(m_hEngine_Vt, fSilShield);
}

int r2mem_vt::process(float* pData_in, int iLen_in, int iFlag, float* pScore){
  
  R2_MEM_ASSERT(this,0);
  
  return  r2_vt_process(m_hEngine_Vt, (const float**)&pData_in, iLen_in, iFlag);
  //return 0 ;
}


WordType r2mem_vt::getwordtype(){
  
  return r2_vt_getwordtype(m_hEngine_Vt) ;
}

int r2mem_vt::getwordpos(int *start, int *end){
  
  return r2_vt_getwordpos(m_hEngine_Vt, start, end);
}

int r2mem_vt::setdosecslcallback(dosecsl_callback fun_cb, void* param_cb){
  
  return r2_vt_set_dosecsl_cb(m_hEngine_Vt,fun_cb,param_cb);
}

int r2mem_vt::setcheckdirtycallback(checkdirty_callback fun_cb, void* param_cb){
  
  return rt_vt_set_checkdirty_cb(m_hEngine_Vt, fun_cb, param_cb);
}

int r2mem_vt::setgetdatacallback(getdata_callback fun_cb, void* param_cb){
  
  return rt_vt_set_getdata_cb(m_hEngine_Vt, fun_cb, param_cb);
}

