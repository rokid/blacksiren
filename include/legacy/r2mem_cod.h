#ifndef R2_MEM_COD_H
#define R2_MEM_COD_H

#include "opus/opus.h"
#include "r2math.h"


enum r2cod_type{
  r2ad_cod_opu = 1,
  r2ad_cod_pcm
};

class r2mem_cod
{
public:
  r2mem_cod(r2cod_type iCodeType);
public:
  ~r2mem_cod(void);
  
  int reset();
  int pause();
  int resume();
  int process(float* pData_In, int iLen_In);
  int processdata();
  
  int getdatalen();
  int getdata(char* pData, int iLen);
  int getdata2(char* &pData, int &iLen);
  
  bool istoolong();
  bool isneedresume();
  
  int m_iShield_TooLong ;
  int m_iShield_Resume ;
  
  
public:
  
  r2cod_type m_iCodeType ;
  
  int m_iLen_In ;
  int m_iLen_In_Total ;
  float * m_pData_In;
  
  int m_iLen_Pause ;
  
  int m_iLen_Cod ;
  int m_iLen_Frm_Cod ;
  unsigned char* m_pData_Cod ;
  
  bool m_bPaused ;
  
  int m_iLen_Out ;
  int m_iLen_Out_Cur ;
  int m_iLen_Out_Total ;
  char* m_pData_Out ;
  
  OpusEncoder *m_hEngine_Cod ;
  
};

#endif