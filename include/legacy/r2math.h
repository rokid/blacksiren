#ifndef R2_MATH_H
#define R2_MATH_H

#include <string>
#include <vector>
#include <sched.h>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>

#if defined(__arm__) || defined(__aarch64__)
#define __ARM_ARCH_ARM__
#endif


#if defined(__ANDROID__) || defined(ANDROID)
#include <android/log.h>
#define ZLOG_INFO(...) {char info[512]; sprintf(info,__VA_ARGS__);__android_log_print(ANDROID_LOG_INFO,"RKUPL_r2audio",info);}
#define ZLOG_ERROR(...) {char info[512]; sprintf(info,__VA_ARGS__);__android_log_print(ANDROID_LOG_ERROR,"RKUPL_r2audio",info);}
#else
#define ZLOG_INFO(...){ char info[512]; sprintf(info,__VA_ARGS__); printf(info);printf("\n");}
#define ZLOG_ERROR(...){ char info[512]; sprintf(info,__VA_ARGS__); printf(info);printf("\n");}
#endif

#ifndef __ARM_ARCH_ARM__
#include <xmmintrin.h>
#include "mkl_cblas.h"
#include "mkl_lapacke.h"
#include "fftw/fftw3.h"
#else
#include <fftw3.h>
#include <blis/blis.h>
#include <blis/cblas.h>
#endif

#ifndef r2_max
#define r2_max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef r2_min
#define r2_min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#ifdef __ARM_ARCH_ARM__
#if defined(__ANDROID__) || defined(ANDROID)
  //#define DEBUG_FILE_LOCATION "/storage/sd/debug"
  #define DEBUG_FILE_LOCATION "/data/debug"
#else
  #define DEBUG_FILE_LOCATION "/opt/debug"
#endif
#else
#define DEBUG_FILE_LOCATION "/Users/Shared/R2AudioDebug"
#define  R2_MEM_DEBUG
#endif


#ifndef R2_AUDIO_SAMPLE_RATE
#define  R2_AUDIO_SAMPLE_RATE  16000
#define  R2_AUDIO_FRAME_MS  10
#define  R2_AUDIO_AEC_FRAME_MS  16
#define  R2_BFSL_MIN_DIS     0.52f      //30 degree

#define R2_LZERO	(-1.0E10f)		/* ~log(0) */
#define R2_LSMALL	(-0.5E10f)		/* log values < LSMALL are set to LZERO */
#define R2_LEXPZ	(-20.0f)		/* 1.0 + exp(z) == 1.0, log(FLT_EPSILON) */

#endif

#ifdef WIN32
#define strcasecmp stricmp
#else
#define strtok_s strtok_r
#endif

#if 0
#ifdef __ARM_ARCH_ARM__

#ifdef __LP64__
#define CPU_SETSIZE 1024
#else
#define CPU_SETSIZE 32
#endif

#define __CPU_BITTYPE  unsigned long int  /* mandated by the kernel  */
#define __CPU_BITS     (8 * sizeof(__CPU_BITTYPE))
#define __CPU_ELT(x)   ((x) / __CPU_BITS)
#define __CPU_MASK(x)  ((__CPU_BITTYPE)1 << ((x) & (__CPU_BITS - 1)))

#if defined(__arm__)
typedef struct {
  __CPU_BITTYPE __bits[ CPU_SETSIZE / __CPU_BITS];
} cpu_set_t;
#endif



#define CPU_ZERO_S(setsize, set)  __builtin_memset(set, 0, setsize)
#define CPU_SET_S(cpu, setsize, set) \
do { \
size_t __cpu = (cpu); \
if (__cpu < 8 * (setsize)) \
(set)->__bits[__CPU_ELT(__cpu)] |= __CPU_MASK(__cpu); \
} while (0)
#define CPU_ZERO(set)          CPU_ZERO_S(sizeof(cpu_set_t), set)
#define CPU_SET(cpu, set)      CPU_SET_S(cpu, sizeof(cpu_set_t), set)

void setCurrentThreadAffinityMask(cpu_set_t mask);
#endif
#endif

#define R2_SAFE_CLOSE_FILE(p) do {if(p) { fclose(p); p = NULL; } } while (0);

#ifndef  R2_MEM_DEBUG

#define  R2_SAFE_NEW(p,type,...) new type(__VA_ARGS__)
#define  R2_SAFE_NEW_AR1(p,type,dim1) (type*)r2_new_ar1(sizeof(type),dim1)
#define  R2_SAFE_NEW_AR2(p,type,dim1,dim2) (type**)r2_new_ar2(sizeof(type),dim1,dim2)
#define  R2_SAFE_DEL(p)  do {if(p) { delete p ;p = NULL; } } while (0);
#define  R2_SAFE_DEL_AR1(p)	do {if(p) { delete [] p; p = NULL; } } while (0);
#define  R2_SAFE_DEL_AR2(p)	do {if(p) {R2_SAFE_DEL_AR1(p[0]); R2_SAFE_DEL_AR1(p); } } while (0);
#define  R2_PRINT_MEM_INFO()
#define  R2_MEM_ASSERT(p,label)

#else

#define  R2_SAFE_NEW(p,type,...) new type(__VA_ARGS__) ; r2_mem_insert(p,1,0,__FILE__,__LINE__) ;
#define  R2_SAFE_NEW_AR1(p,type,dim1) (type*)r2_new_ar1(sizeof(type),dim1) ; r2_mem_insert(p,sizeof(type)* dim1,1,__FILE__,__LINE__) ;
#define  R2_SAFE_NEW_AR2(p,type,dim1,dim2) (type**)r2_new_ar2(sizeof(type),dim1,dim2) ; r2_mem_insert(p,sizeof(type)* dim1 * dim2,2,__FILE__,__LINE__) ;
#define  R2_SAFE_DEL(p)  do {if(p) {r2_mem_erase(p,0); delete p ;p = NULL; } } while (0);
#define  R2_SAFE_DEL_AR1(p)	do {if(p) { r2_mem_erase(p,1); delete [] p; p = NULL; } } while (0);
#define  R2_SAFE_DEL_AR2(p)	do {if(p) { r2_mem_erase(p,2); R2_SAFE_DEL_AR1(p[0]); R2_SAFE_DEL_AR1(p); } } while (0);
#define  R2_PRINT_MEM_INFO() r2_mem_print() ;
#define  R2_MEM_ASSERT(p,label) r2_mem_assert(p, label);

void r2_mem_insert(void* pData, int iSize,int label, const char* pSource, int iLine);
void r2_mem_erase(void* pData,int label);
void r2_mem_print();

void r2_mem_assert(void* pData, int label);

#endif


char*   r2_new_ar1(int size,int dim1);
char**	r2_new_ar2(int size,int dim1,int dim2);


std::string r2_getkey(const char *path,const char *title,const char *key);
int r2_getkey_int(const char *path,const char *title,const char *key, int di);
float r2_getkey_float(const char* path, const char* title, const char *key, float df);
bool r2_getkey_bool(const char *path,const char *title,const char *key, bool db);
std::vector<std::string> r2_getkeylst(const char *path,const char *title,const char *key);
std::vector<std::string> r2_strsplit(const char* str, const char* split);
int r2_mkdir(const char* path);
int r2_gcd(int m,int n);
std::string r2_getdatatime();
int r2_storefile(const char* pPath, const char* pData, int iLen);

struct r2_mic_info{
  int iMicNum ;
  int* pMicIdLst ;
};
r2_mic_info* r2_getmicinfo(const char* path, const char* title, const char* key);
r2_mic_info* r2_getdefaultmicinfo(int iMicNum);
int r2_free_micinfo(r2_mic_info* &pMicInfo);
r2_mic_info* r2_copymicinfo(r2_mic_info* pMicInfo_Old);

int r2_fixerrmix(r2_mic_info* pMicInfo , r2_mic_info* pErrMicInfo);

float r2_logadd(float x, float y);


#endif


