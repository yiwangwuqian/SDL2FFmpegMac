//
//  AudioPlayWorker.h
//  SDLPlayAudio
//
//  Created by guohaoyang on 2024/6/12.
//

#ifndef AudioPlayWorker_h
#define AudioPlayWorker_h

#include "SDL.h"
#include <stdbool.h>

/**
 *  这个播放器只能播放双声道及单声道的视频，
 *  ⚠️超过两个声道的视频参考了ffplay发现需要使用filter，暂不作修改。
 */
typedef struct AudioPlayWorker AudioPlayWorker;

void AudioPlayWorkerCreate(AudioPlayWorker **worker);
void AudioPlayWorkerDestroy(AudioPlayWorker **worker);
//Uint64 AudioPlayWorkerBufferSize(AudioPlayWorker *worker);
//返回值指示是否读完
bool AudioPlayWorkerUpdateBuffer2(AudioPlayWorker *worker,uint8_t *data, int len);

#endif /* AudioPlayWorker_h */
