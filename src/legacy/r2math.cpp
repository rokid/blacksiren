#include "legacy/r2math.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#ifdef __ARM_ARCH_ARM__

void setCurrentThreadAffinityMask(cpu_set_t mask) {
  int err, syscallres;
#if defined SYS_gettid
  pid_t pid = syscall(SYS_gettid);
#else
  pid_t pid = gettid();
#endif
  syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
  if (syscallres) {
    err = errno;
    //ALOGI("set affinity failed");
  } else {
    //ALOGI("set affinity done");
  }
}

#endif

char* r2_new_ar1(int size,int dim1) {
  
  if (dim1 == 0) {
    return NULL ;
  }
  char * pData = new char[dim1*size];
  if (pData ==NULL) {
    printf("Error in r2_new_ Memory\r\n");
    return NULL ;
  }
  memset(pData,0,size*dim1);
  return pData ;
}

char**	r2_new_ar2(int size,int dim1,int dim2) {
  
  if (dim1 == 0 || dim2 == 0) {
    return NULL ;
  }
  char** pData = (char**) R2_SAFE_NEW_AR1(pData, char*, dim1);
  if( NULL == pData ) {
    printf("Error in r2_new_ Memory\r\n");
    return NULL ;
  }
  pData[0] = R2_SAFE_NEW_AR1(pData[0], char, size * dim1 * dim2);
  if( NULL == pData[0] ) {
    printf("Error in r2_new_ Memory\r\n");
    delete [] pData ;
    return NULL ;
  }
  memset(pData[0],0,size * dim1 * dim2);
  for(int k=1; k<dim1; k++) {
    pData[k] = pData[k-1] + dim2 * size;
  }
  return pData;
}

#ifdef  R2_MEM_DEBUG

static std::map<std::string, std::string> r2_mem_lst ;

void r2_mem_insert(void* pData, int iSize,int label, const char* pSource, int iLine){
  
  char kk[1024];
  char vv[1024];
  
  if(pData != NULL){
    
    sprintf(kk, "%X_%d",pData,label);
    sprintf(vv, "%s:%d:%dBytes",pSource, iLine, iSize);
    //printf("%s new \n",vv);
    
    r2_mem_lst[kk] = vv ;
  }
  
}

void r2_mem_erase(void* pData,int label){
  
  char kk[1024];
  if (pData != NULL) {
    
    sprintf(kk, "%X_%d",pData,label);
    assert(r2_mem_lst.find(kk) != r2_mem_lst.end());
    //printf("%s del \n",r2_mem_lst[kk].c_str());
    r2_mem_lst.erase(kk);
  }
  
}

void r2_mem_print(){
  
  std::map<std::string, std::string>::iterator it;
  for(it=r2_mem_lst.begin();it!=r2_mem_lst.end();++it){
    ZLOG_ERROR("MemLeak: %s",it->second.c_str());
  }
  
}


void r2_mem_assert(void* pData, int label){
  
  char kk[1024];
  if (pData != NULL) {
    sprintf(kk, "%X_%d",pData,label);
    assert(r2_mem_lst.find(kk) != r2_mem_lst.end());
  }
  
}

#endif


std::string r2_getkey(const char *path,const char *title,const char *key){
  
  int ll = 10240 ;
  char* szLine = new char[ll];
  std::string strtitle = "[" ;
  strtitle += title ;
  strtitle += "]" ;
  
  FILE *fp = fopen(path, "r");
  if(fp == NULL) {
    return "" ;
  }
  bool flag = false ;
  while(!feof(fp)) {
    fgets(szLine,ll,fp);
    strtok(szLine,"\r\n");
    if (strlen(szLine) > 0 && szLine[0] == '[') {
      if (strcasecmp(szLine,strtitle.c_str()) == 0) {
        flag = true ;
      } else {
        flag = false ;
      }
      continue;
    }
    if(flag) {
      char * pTemp = strchr(szLine,'=') ;
      if (pTemp != NULL) {
        *pTemp = '\0';
        pTemp ++ ;
        char* kk = strtok(szLine,"\r\n\t");
        char* vv = strtok(pTemp,"\r\n\t");
        while(*kk == ' ') {
          kk ++ ;
        }
        while(strlen(kk) > 0 && *(kk+strlen(kk) - 1) == ' ') {
          *(kk+strlen(kk) - 1) = '\0';
        }
        while(*vv == ' ') {
          vv ++ ;
        }
        while(strlen(vv) > 0 && *(vv+strlen(vv) - 1) == ' ') {
          *(vv+strlen(vv) - 1) = '\0';
        }
        if(strcasecmp(kk,key) ==0) {
          fclose(fp);
          strtitle = vv ;
          delete szLine ;
          return strtitle ;
        }
      }
    }
  }
  fclose(fp);
  delete szLine ;
  
  return "" ;
}

int r2_getkey_int(const char *path,const char *title,const char *key, int di){
  
  std::string vv = r2_getkey(path,title,key);
  if (vv.size() == 0) {
    return di ;
  }
  return atoi(vv.c_str());
}

float r2_getkey_float(const char* path, const char* title, const char *key, float df){
  
  std::string vv = r2_getkey(path, title, key);
  if (vv.size() == 0) {
    return  df;
  }
  return atof(vv.c_str());
}

bool r2_getkey_bool(const char *path,const char *title,const char *key, bool db){
  
  std::string vv = r2_getkey(path,title,key);
  if (vv.size() == 0) {
    return db ;
  }
  return atoi(vv.c_str()) > 0;
}

std::vector<std::string> r2_getkeylst(const char *path,const char *title,const char *key){
  
  std::vector<std::string> res ;
  
  int ll = 10240 ;
  char* szLine = new char[ll];
  std::string strtitle = "[" ;
  strtitle += title ;
  strtitle += "]" ;
  
  FILE *fp = fopen(path, "r");
  if(fp == NULL) {
    return res ;
  }
  bool flag = false ;
  while(!feof(fp)) {
    fgets(szLine,ll,fp);
    strtok(szLine,"\r\n");
    if (strlen(szLine) > 0 && szLine[0] == '[') {
      if (strcasecmp(szLine,strtitle.c_str()) == 0) {
        flag = true ;
      } else {
        flag = false ;
      }
      continue;
    }
    if(flag) {
      char * pTemp = strchr(szLine,'=') ;
      if (pTemp != NULL) {
        *pTemp = '\0';
        pTemp ++ ;
        char* kk = strtok(szLine,"\r\n\t");
        char* vv = strtok(pTemp,"\r\n\t");
        while(*kk == ' ') {
          kk ++ ;
        }
        while(strlen(kk) > 0 && *(kk+strlen(kk) - 1) == ' ') {
          *(kk+strlen(kk) - 1) = '\0';
        }
        while(*vv == ' ') {
          vv ++ ;
        }
        while(strlen(vv) > 0 && *(vv+strlen(vv) - 1) == ' ') {
          *(vv+strlen(vv) - 1) = '\0';
        }
        if(strcasecmp(kk,key) ==0) {
          res.push_back(vv);
        }
      }
    }
  }
  fclose(fp);
  delete szLine ;
  return res ;
}

std::vector<std::string> r2_strsplit(const char* str, const char* split){
  
  std::vector<std::string> rt ;
  char * pTmp = new char[strlen(str) + 5] , *pTok = NULL;
  strcpy(pTmp,str);
  char* pV = strtok_s(pTmp,split,&pTok);
  while(pV != NULL){
    rt.push_back(pV);
    pV = strtok_s(NULL,split,&pTok);
  }
  delete pTmp ;
  return rt ;
}

int r2_mkdir(const char* path){
  
  char tmpPath[512];
  memset(tmpPath,0,sizeof(char)*512);
  for (int i = 0 ; i < strlen(path) ; i ++) {
    if (*(path+i) == '/') {
      mkdir(tmpPath,0777);
    }
    tmpPath[i] = path[i] ;
  }
  mkdir(tmpPath,0777);
  return 0 ;
}

static int g_count = 0 ;

std::string r2_getdatatime(){
  
  time_t now;
  struct tm *tm_now;
  char    datetime[200];
  
  time(&now);
  tm_now = localtime(&now);
  strftime(datetime, 200, "%Y-%m-%d_%H-%M-%S_", tm_now);
  std::string dt = datetime ;
  
  sprintf(datetime, "%d", g_count ++ );
  dt += datetime ;
  
  return  dt ;
  
  
}

int r2_storefile(const char* pPath, const char* pData, int iLen){
  
  FILE* pFile = fopen(pPath, "wb");
  if (pFile != NULL) {
    fwrite(pData, sizeof(char), iLen, pFile);
    fclose(pFile);
  }
  
  return  0 ;
}


r2_mic_info* r2_getmicinfo(const char* path, const char* title, const char* key){
  
  std::string vv = r2_getkey(path,title,key);
  std::vector<std::string> vs = r2_strsplit(vv.c_str(),",");
  if (vs.size() > 0) {
    r2_mic_info* pMicInfo = R2_SAFE_NEW(pMicInfo, r2_mic_info);
    pMicInfo->iMicNum = vs.size() ;
    pMicInfo->pMicIdLst = R2_SAFE_NEW_AR1(pMicInfo->pMicIdLst, int, pMicInfo->iMicNum);
    for (int i = 0 ; i < pMicInfo->iMicNum; i ++) {
      pMicInfo->pMicIdLst[i] = atof(vs[i].c_str());
    }
    return pMicInfo ;
  }else {
    return 0 ;
  }
}


r2_mic_info* r2_getdefaultmicinfo(int iMicNum){
  
  r2_mic_info* pMicInfo = R2_SAFE_NEW(pMicInfo, r2_mic_info);
  pMicInfo->iMicNum = iMicNum ;
  pMicInfo->pMicIdLst = R2_SAFE_NEW_AR1(pMicInfo->pMicIdLst, int, pMicInfo->iMicNum);
  for (int i = 0 ; i < pMicInfo->iMicNum; i ++) {
    pMicInfo->pMicIdLst[i] = i ;
  }
  return pMicInfo ;
}


int r2_free_micinfo(r2_mic_info* &pMicInfo){
  
  if (pMicInfo != NULL) {
    R2_SAFE_DEL_AR1(pMicInfo->pMicIdLst);
    R2_SAFE_DEL(pMicInfo);
  }
  return  0 ;
}

r2_mic_info* r2_copymicinfo(r2_mic_info* pMicInfo_Old){
  
  r2_mic_info* pMicInfo_New = R2_SAFE_NEW(pMicInfo_New, r2_mic_info);
  pMicInfo_New->iMicNum = pMicInfo_Old->iMicNum ;
  pMicInfo_New->pMicIdLst = R2_SAFE_NEW_AR1(pMicInfo_New->pMicIdLst, int, pMicInfo_New->iMicNum);
  for (int i = 0 ; i < pMicInfo_New->iMicNum; i ++) {
    pMicInfo_New->pMicIdLst[i] = pMicInfo_Old->pMicIdLst[i] ;
  }
  return pMicInfo_New ;
}

int r2_fixerrmix(r2_mic_info* pMicInfo , r2_mic_info* pErrMicInfo){
  
  if (pErrMicInfo == NULL) {
    return  0 ;
  }
  
  int iCur = 0 ;
  for (int i = 0 ; i < pMicInfo->iMicNum ; i ++) {
    bool bExit = false ;
    for (int j = 0 ; j < pErrMicInfo->iMicNum ; j ++) {
      if (pMicInfo->pMicIdLst[i] == pErrMicInfo->pMicIdLst[j]) {
        bExit = true ;
        break ;
      }
    }
    if (!bExit) {
      if (i != iCur) {
        pMicInfo->pMicIdLst[iCur] = pMicInfo->pMicIdLst[i];
      }
      iCur ++ ;
    }
  }
  if (iCur == pMicInfo->iMicNum) {
    return  0 ;
  }else{
    pMicInfo->iMicNum = iCur ;
    return  1 ;
  }
  
}


float r2_logadd(float x, float y){
  
  float z;
  if( x < y ) {
    z = x;
    x = y;
    y = z;
  }
  if( (z = y-x) < R2_LEXPZ )	{
    return (x < R2_LSMALL ? R2_LZERO : x);
  }
  z = x + (float)log(1.0 + expf(z));
  return (z < R2_LSMALL ? R2_LZERO : z);
}
