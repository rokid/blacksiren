/*
 * r2ssp.h
 *
 * r2 speech signal processing library
 * from ztspeech
 *
 *  Created on: 2015-1-13
 *      Author: gaopeng
 */

#ifndef R2SSP_H_
#define R2SSP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef long long r2ssp_handle;


int r2ssp_ssp_init();
int r2ssp_ssp_exit();

/**
 * acoustic echo cancellation API
 *
 */

#define ZTSSP_AEC_FRAME_SAMPLE 256

#define ZTSSP_AEC_MODE_DEFAULT	0

/**
 * create aec instance
 *
 * input:
 * 	mode: 0 (default)
 *
 * return:
 *  aec handle
 *
 */
r2ssp_handle r2ssp_aec_create(int mode);

/**
 * free aec instance
 *
 * input:
 *  hAec: aec handle
 *
 * return:
 *  0: success; other: failed
 *
 */
int r2ssp_aec_free(r2ssp_handle hAec);

/**
 * set aec multithread affinity
 *
 * input:
 *  hAec: aec handle
 *  thread_affinities: bind thread on which core
 *  	such as: [1, 3], means bind thread 0 on core 1, and bind thread 1 on core 3
 *  thread_affi_num: length of thread_affinities
 *
 * return:
 *  0: success; other: failed
 *
 */
int r2ssp_aec_set_thread_affinities(r2ssp_handle hAec, int *thread_affinities, int thread_num);

/**
 * init aec instance
 *
 * input:
 *  hAec: aec handle
 *  nSampleRate: sample rate of speech signal, such as: 16000
 *
 * return:
 *  0: success; other: failed
 *
 */
int r2ssp_aec_init(r2ssp_handle hAec, int nSampleRate, int nChannelNum, int nSpeakerNum);

/**
 * buffer one frame reference signal
 *
 * input:
 *  hAec: aec handle
 *  refSamples: reference signal data
 *  length: length of refSamples in shorts, must equal to ZTSSP_AEC_FRAME_SAMPLE
 *
 * return:
 *  0: success; other: failed
 *
 */
int r2ssp_aec_buffer_farend(r2ssp_handle hAec, const float *refSamples, int length);

/**
 * process one frame microphone signal, output echo canceled signal
 * for each frame, call 'r2ssp_aec_buffer_farend', then call 'r2ssp_aec_process'
 *
 * input:
 *  hAce: aec handle
 *  mixSamples: microphone signal (with mixed echo)
 *  length: length os mixSamples in shorts, must equal to ZTSSP_AEC_FRAME_SAMPLE
 *  outSamples (out): output signal, at least length ZTSSP_AEC_FRAME_SAMPLE
 *  delayMs: time delay between mix and ref signal, set to 0
 *
 * return:
 *  0: success; 1: ref all zero, skip aec; other: failed
 *
 */
int r2ssp_aec_process(r2ssp_handle hAec, const float *mixSamples, int length,
		float *outSamples, int delayMs);

int r2ssp_aec_reset(r2ssp_handle hAec);

/**
 * noise suppression API
 *
 */

r2ssp_handle r2ssp_ns_create();

int r2ssp_ns_free(r2ssp_handle hNs);

int r2ssp_ns_init(r2ssp_handle hNs, int nSampleRate);

int r2ssp_ns_set_mode(r2ssp_handle hNs, int nMode);

int r2ssp_ns_process(r2ssp_handle hNs, const float *pFrame, float *pOutFrame);

/**
 * beam-forming API
 *
 */

r2ssp_handle r2ssp_bf_create(float *pMics, int nMicNum);

int r2ssp_bf_free(r2ssp_handle hBf);

int r2ssp_bf_init(r2ssp_handle hBf, int nFrameSizeMs, int nSampleRate);

int r2ssp_bf_steer(r2ssp_handle hBf, float targetAngle, float targetAngle2,
		float interfAngle, float interfAngle2);

int r2ssp_bf_process(r2ssp_handle hBf, const float *pInFrames, int nChunkSize,
		int nChannels, float *pOutFrame);

/**
  * beamform sound localization API
  *
  */

typedef struct r2_sound_candidate_ {
    int serial;
    float azimuth;
    float elevation;
    float confidence;
    int ticks;
} r2_sound_candidate;

r2ssp_handle r2ssp_bfsl_create(float *pMics, int nMicNum);

int r2ssp_bfsl_free(r2ssp_handle hBfSl);

int r2ssp_bfsl_init(r2ssp_handle hBfSl, int nFrameSizeMs, int nSampleRate);

int r2ssp_bfsl_reset(r2ssp_handle hBfSl);

int r2ssp_bfsl_process(r2ssp_handle hBfSl, const float *pInFrames, int nChunkSize,
                     int nChannels);

int r2ssp_bfsl_get_candidate_num(r2ssp_handle hBfSl);
    
int r2ssp_bfsl_get_candidate(r2ssp_handle hBfSl, int nCandi, r2_sound_candidate *pCandidate);

/**
 * feature extraction API
 *
 */

/**
 * create a FE handle.
 * nType: 0, fbank feature. other value for future extension.
 * nFrameSizeMs: input frame size in milliseconds. for fbank, must be 10ms.
 * nSampelRate: sample rate. now only support 16000.
 */
//r2ssp_handle r2ssp_fe_create(int nType, int nFrameSizeMs, int nSampleRate);

//int r2ssp_fe_free(r2ssp_handle hFe);

/**
 * return feature length (in floats) of one frame feature.
 * type: 0 for voice trigger, 1 for vad, 2 for energy.
 */
//int r2ssp_fe_get_featurelen(r2ssp_handle hFe, int type);

//int r2ssp_fe_reset(r2ssp_handle hFe);

/**
 * process data in frames.
 */
//int r2ssp_fe_process(r2ssp_handle hFe, const float *pInFrames, int nFrameNum);

/**
 * return available output feature frame number.
 */
//int r2ssp_fe_get_featurenum(r2ssp_handle hFe);

/**
 * return output feature frame number copied into pOutFeatures.
 */
//int r2ssp_fe_get_features(r2ssp_handle hFe, float *pOutFeatures,
//		int nOutFrameNum, int type);

/**
 * resampler API
 *
 */

r2ssp_handle r2ssp_rs_create();

int r2ssp_rs_free(r2ssp_handle hRs);

int r2ssp_rs_init(r2ssp_handle hRs, int inSampleRate, int outSampleRate);

/*
 * return: samples written into pOutFrame
 */
int r2ssp_rs_process(r2ssp_handle hRs, const float *pInFrame, int inFrameLen,
		float *pOutFrame, int outFrameCapacity);

/**
  * auto-gain control API
  *
  */

r2ssp_handle r2ssp_agc_create();

int r2ssp_agc_free(r2ssp_handle hAgc);
    
int r2ssp_agc_init(r2ssp_handle hAgc, int nFrameSizeMs, int nSampleRate);
    
int r2ssp_agc_reset(r2ssp_handle hAgc);
    
int r2ssp_agc_process(r2ssp_handle hAgc, short *pInFrame);
    
int r2ssp_agc_get_diff_dbfs(r2ssp_handle hAgc, int *bNeedUpdate, int *dbfs);

/**
 * band-pass filter, internal fft size is 256 (frequency bin, 0-128)
 */

r2ssp_handle r2ssp_bpf_create();

int r2ssp_bpf_free(r2ssp_handle hBpf);

int r2ssp_bpf_init(r2ssp_handle hBpf, int nFrameSizeMs, int nSampleRate,
                       int nStartFreqBin, int nStopFreqBin, int bBandPass);

int r2ssp_bpf_set_freqscales(r2ssp_handle hBpf, float *pScales, int nLength);

int r2ssp_bpf_process(r2ssp_handle hBpf, const float *pFrame, int nFrameSize, float *pOutFrame);

/**
  * frequency adapter, internal fft size is 256 (frequency bin, 0-128)
  */

r2ssp_handle r2ssp_fa_create();

int r2ssp_fa_free(r2ssp_handle hFa);

int r2ssp_fa_init(r2ssp_handle hFa, int nFrameSizeMs, int nSampleRate);

int r2ssp_fa_is_adapted(r2ssp_handle hFa);

int r2ssp_fa_get_freqscales(r2ssp_handle hFa, float *pScales, int nLength);

int r2ssp_fa_process(r2ssp_handle hFa, const float *pFrames, int nChunkSize, float *pOutFrame);

/**
  * r2ssp for server asr
  */

typedef struct r2ssp_sa_handle_ {
    r2ssp_handle agc_handle;
    r2ssp_handle ns_handle;
} r2ssp_sa_handle;

r2ssp_handle r2ssp_sa_create();

int r2ssp_sa_free(r2ssp_handle hSa);

int r2ssp_sa_init(r2ssp_handle hSa, int nFrameSizeMs, int nSampleRate);

int r2ssp_sa_reset(r2ssp_handle hSa);

int r2ssp_sa_process(r2ssp_handle hSa, short *pInFrame);


#ifdef __cplusplus
}
#endif

#endif /* R2SSP_H_ */
