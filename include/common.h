#ifndef BLACK_SIREN_COMMON_H
#define BLACK_SIREN_COMMON_H

#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS  }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#define SIREN_UNUSED(x) (void)(x)

__BEGIN_DECLS

#ifdef __GNUC__
#define PRINTF_FORMAT(a,b) __attribute__ ((format (printf, (a), (b))))
#define STRUCT_PACKED __attribute__ ((packed))
#else
#define PRINTF_FORMAT(a,b)
#define STRUCT_PACKED
#endif


#ifdef CONFIG_RECORDING_PATH
#define CONFIG_STORE_PATH CONFIG_RECORDING_PATH 
#else
#define CONFIG_STORE_PATH ""
#endif

__END_DECLS



#endif
