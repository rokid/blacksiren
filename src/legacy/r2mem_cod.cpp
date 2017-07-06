#include "legacy/r2mem_cod.h"

r2mem_cod::r2mem_cod(r2cod_type iCodeType){
  
  m_iCodeType = iCodeType ;
  
  //Raw
  m_iLen_In = 0 ;
  m_iLen_In_Total = R2_AUDIO_SAMPLE_RATE * 20 ;
  m_pData_In = R2_SAFE_NEW_AR1(m_pData_In, float, m_iLen_In_Total);
  
  //Cod
  m_iLen_Cod = 0 ;
  m_iLen_Frm_Cod = R2_AUDIO_SAMPLE_RATE / 1000 * R2_AUDIO_FRAME_MS * 2 ;
  m_pData_Cod = R2_SAFE_NEW_AR1(m_pData_Cod, unsigned char, m_iLen_Frm_Cod * sizeof(float));
  
  if (m_iCodeType == r2ad_cod_opu) {
    int err = 0 ;
    m_hEngine_Cod = opus_encoder_create(R2_AUDIO_SAMPLE_RATE,
                                        1, OPUS_APPLICATION_VOIP, &err);
    
    opus_encoder_ctl(m_hEngine_Cod, OPUS_SET_VBR(1));
    opus_encoder_ctl(m_hEngine_Cod, OPUS_SET_BITRATE(27800));
    opus_encoder_ctl(m_hEngine_Cod, OPUS_SET_COMPLEXITY(8));
    opus_encoder_ctl(m_hEngine_Cod, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
  }else{
    m_hEngine_Cod = NULL ;
  }
  
  m_bPaused = false ;
  
  //Out
  m_iLen_Out = 0 ;
  m_iLen_Out_Cur = 0 ;
  m_iLen_Out_Total = R2_AUDIO_SAMPLE_RATE * 20 ;
  m_pData_Out = R2_SAFE_NEW_AR1(m_pData_Out, char, m_iLen_Out_Total);
  
  reset() ;
  
  m_iLen_Pause = 0 ;
  
  m_iShield_TooLong = R2_AUDIO_SAMPLE_RATE * 6 ;
  m_iShield_Resume = R2_AUDIO_SAMPLE_RATE * 0.5f ;
  
}


r2mem_cod::~r2mem_cod(void){
  
  R2_SAFE_DEL_AR1(m_pData_In);
  R2_SAFE_DEL_AR1(m_pData_Cod);
  R2_SAFE_DEL_AR1(m_pData_Out);
  
  if (m_hEngine_Cod != NULL) {
    opus_encoder_destroy(m_hEngine_Cod);
  }
}

int r2mem_cod::reset(){
  
  m_iLen_In = 0 ;
  m_iLen_Cod = 0 ;
  m_iLen_Out = 0 ;
  m_iLen_Out_Cur = 0 ;
  
  m_bPaused = false ;
  
  if (m_iCodeType == r2ad_cod_opu ) {
    int err = 0 ;
    opus_encoder_destroy(m_hEngine_Cod);
    m_hEngine_Cod = opus_encoder_create(R2_AUDIO_SAMPLE_RATE,
                                        1, OPUS_APPLICATION_VOIP, &err);
    
    opus_encoder_ctl(m_hEngine_Cod, OPUS_SET_VBR(1));
    opus_encoder_ctl(m_hEngine_Cod, OPUS_SET_BITRATE(27800));
    opus_encoder_ctl(m_hEngine_Cod, OPUS_SET_COMPLEXITY(8));
    opus_encoder_ctl(m_hEngine_Cod, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
  }
  
  return  0 ;
  
}

int r2mem_cod::pause(){
  
  m_iLen_Out_Cur = 0 ;
  m_bPaused = true ;
  m_iLen_Pause = m_iLen_In ;
  
  return  0 ;
}

int r2mem_cod::resume(){
  
  assert(m_bPaused) ;
  m_bPaused = false ;
  
  return  0 ;
}

int r2mem_cod::process(float* pData_In, int iLen_In){
  
  if (iLen_In + m_iLen_In > m_iLen_In_Total) {
    m_iLen_In_Total = (iLen_In + m_iLen_In) * 2 ;
    float* pTmp = R2_SAFE_NEW_AR1(pTmp, float, m_iLen_In_Total);
    memcpy(pTmp, m_pData_In, sizeof(float) * m_iLen_In) ;
    R2_SAFE_DEL_AR1(m_pData_In) ;
    m_pData_In = pTmp ;
  }
  
  for (int i = 0 ; i < iLen_In ; i ++ , m_iLen_In ++ ) {
    m_pData_In[m_iLen_In] = pData_In[i] / 32768.0f ;
  }
  
  return  0 ;
  
}

int r2mem_cod::processdata(){
  
  assert(!m_bPaused) ;
  
  while (m_iLen_Cod + m_iLen_Frm_Cod < m_iLen_In) {
    
    char * pData_Cod = NULL ;
    int iLen_Cod = 0 ;
    
    if (m_iCodeType == r2ad_cod_opu) {
      iLen_Cod = opus_encode_float(m_hEngine_Cod, m_pData_In + m_iLen_Cod , m_iLen_Frm_Cod,
                                   m_pData_Cod + 1, m_iLen_Frm_Cod - 1 );
      
      m_pData_Cod[0] = iLen_Cod ;
      pData_Cod = (char*)m_pData_Cod ;
      iLen_Cod = iLen_Cod + 1;
      
    }else{
      pData_Cod = (char*)(m_pData_In + m_iLen_Cod);
      iLen_Cod = sizeof(float) * m_iLen_Frm_Cod ;
    }
    
    //store
    if (m_iLen_Out + iLen_Cod > m_iLen_Out_Total)	{
      m_iLen_Out_Total = (m_iLen_Out + iLen_Cod) * 2 ;
      char* pTmp = R2_SAFE_NEW_AR1(pTmp, char, m_iLen_Out_Total);
      memcpy(pTmp, m_pData_Out, m_iLen_Out * sizeof(unsigned char));
      R2_SAFE_DEL_AR1(m_pData_Out);
      m_pData_Out = pTmp ;
      
    }
    
    memcpy(m_pData_Out + m_iLen_Out, pData_Cod, sizeof(char) * iLen_Cod) ;
    m_iLen_Out += iLen_Cod ;
    
    
    m_iLen_Cod += m_iLen_Frm_Cod ;
  }
  
  return 0 ;
}

int r2mem_cod::getdatalen(){
  
  processdata() ;
  return m_iLen_Out - m_iLen_Out_Cur ;
}

int r2mem_cod::getdata(char* pData, int iLen){
  
  processdata() ;
  
  int ll = r2_min(iLen, m_iLen_Out - m_iLen_Out_Cur);
  memcpy(pData, m_pData_Out + m_iLen_Out_Cur, ll) ;
  m_iLen_Out_Cur += ll ;
  
  return  ll ;
}

int r2mem_cod::getdata2(char* &pData, int &iLen){
  
  processdata() ;
  
  iLen = m_iLen_Out - m_iLen_Out_Cur ;
  pData = m_pData_Out + m_iLen_Out_Cur ;
  m_iLen_Out_Cur += iLen ;
  
  return  0 ;
  
}

bool r2mem_cod::istoolong(){
  
  if (m_iLen_In > m_iShield_TooLong) {
    return true ;
  }else{
    return false ;
  }
}


bool r2mem_cod::isneedresume(){
  
  if (m_bPaused && m_iLen_In < m_iShield_TooLong && (m_iLen_In - m_iLen_Pause) > m_iShield_Resume) {
    return  true ;
  }else{
    return  false ;
  }

}







