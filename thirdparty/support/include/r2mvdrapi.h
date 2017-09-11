//
//  r2mvdrapi.h
//  r2mvdr
//
//  Created by xxx on 8/2/17.
//  Copyright © 2017 hadoop. All rights reserved.
//

#ifndef __r2mvdr__r2mvdrapi__
#define __r2mvdr__r2mvdrapi__


typedef long long r2mvdr_handle;
/*
 *create mvdr
 *
 *input:
 * pMics:mics
 * nMicNum：num of micphones
 * fftLen:FFT length
 *
 *return:
 * 0:success;ohters:failed
 *
 */

r2mvdr_handle r2mvdr_bf_create(float *pMics, int nMicNum,unsigned fftLen);
/*
 *free mvdr
 *
 *input:
 * hBf:handle of mvdr
 *
 *return:
 * 0:success;ohters:failed
 *
 */

int r2mvdr_bf_free(r2mvdr_handle hBf);

/*
 *set delays of left and right micphones
 *
 *input:
 * hBf:handle of mvdr
 * pDelays:delays of right and left micphone
 * nMicNum:num of micphones
 *
 *return:
 * 0:success;ohters:failed
 *
 */

int r2mvdr_bf_set_mic_delays(r2mvdr_handle hBf, float *pDelays, int nMicNum);

/*
 *init mvdr
 *
 *input:
 * hBf:handle of mvdr
 * nMicNum:num of micphones
 * nFrameSize:length of frames:128
 * nSampleRate:samples of speech(16000)
 
 *return:
 * 0:success;ohters:failed
 *
 */

int r2mvdr_bf_init(r2mvdr_handle hBf,int nMicNum ,int nFrameSize, int nSampleRate );
/*
 *steer vector
 *
 *input:
 * hBf:handle of mvdr
 * targetAngle:azimuth of DOA
 * targetAngle2:elevation of DOA
 * interfAngle:0
 * interfAngle2:0
 
 *return:
 * 0:success;ohters:failed
 *
 */

int r2mvdr_bf_steer(r2mvdr_handle hBf, float targetAngle, float targetAngle2,
                    float interfAngle=0, float interfAngle2=0);
/*
 *mvdr process
 *
 *input:
 * hBf:handle of mvdr
 * pInFrames:input data
 * nChunkSize:nFrameSize*Nch
 * nChannels:num of micphones
 * pOutFrame:output data:128
 
 *return:
 * 0:success;ohters:failed
 *
 */

int r2mvdr_bf_process(r2mvdr_handle hBf, const float *pInFrames, int nChunkSize,
                      int nChannels, float *pOutFrame);



#endif /* __r2mvdr__r2mvdrapi__ */
