//
//  AudioPlayWorker.c
//  SDLPlayAudio
//
//  Created by guohaoyang on 2024/6/12.
//

#include "AudioPlayWorker.h"

struct AudioPlayWorker {
    Uint64  buffer_size;
    
    Uint8   *audio_chunk;         //音频块
    Uint32  audio_unused_len;     //音频剩下的长度
    Uint8   *audio_pos;           //音频当前的位置
    
    SDL_mutex   *mutex;
};

void AudioPlayWorkerFillAudio(void *udata, Uint8 *stream, int len)
{
    AudioPlayWorker *worker = (AudioPlayWorker *)udata;
    SDL_LockMutex(worker->mutex);
    
    SDL_memset(stream, 0, len);
    if (worker->audio_unused_len == 0) {
        SDL_UnlockMutex(worker->mutex);
        return;
    }
    len = (len > worker->audio_unused_len ? worker->audio_unused_len :len);
    
    SDL_MixAudio(stream, worker->audio_pos, len, SDL_MIX_MAXVOLUME);
    worker->audio_pos += len;
    worker->audio_unused_len -= len;
    
    SDL_UnlockMutex(worker->mutex);
}

void AudioPlayWorkerCreate(AudioPlayWorker **worker)
{
    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.silence = 0;
    spec.samples = 1024;
    spec.callback = AudioPlayWorkerFillAudio;
    *worker = (AudioPlayWorker *)malloc(sizeof(AudioPlayWorker));
    
    int bufferSize = 4096;
    char *buffer = (char *)malloc(bufferSize);
    (*worker)->audio_chunk = (Uint8 *)buffer;
    (*worker)->audio_unused_len = bufferSize;
    (*worker)->audio_pos = (*worker)->audio_chunk;
    (*worker)->buffer_size = bufferSize;
    spec.userdata = *worker;
    if (SDL_OpenAudio(&spec, NULL)) {
        printf("打开音频失败");
        return;
    }
    (*worker)->mutex = SDL_CreateMutex();
}

void AudioPlayWorkerDestroy(AudioPlayWorker **worker)
{
    free((*worker)->audio_chunk);
    if ((*worker)->mutex) {
        SDL_DestroyMutex((*worker)->mutex);
    }
    free(*worker);
}

Uint64 AudioPlayWorkerBufferSize(AudioPlayWorker *worker)
{
    return worker->buffer_size;
}

bool AudioPlayWorkerUpdateBuffer2(AudioPlayWorker *worker,uint8_t *data, int len)
{
    size_t once_size = len;
    SDL_LockMutex(worker->mutex);
    memcpy(worker->audio_chunk, data, len);
    if (once_size == worker->buffer_size) {
        worker->audio_unused_len = (Uint32)once_size;
        worker->audio_pos = worker->audio_chunk;
        SDL_UnlockMutex(worker->mutex);
        
        SDL_PauseAudio(0);
        while (worker->audio_unused_len > 0) {
            SDL_Delay(1);
        }
        return true;
    } else {
        worker->audio_unused_len = (Uint32)once_size;
        worker->audio_pos = worker->audio_chunk;
        SDL_PauseAudio(0);
    }
    
    SDL_UnlockMutex(worker->mutex);
    
    return false;
}
