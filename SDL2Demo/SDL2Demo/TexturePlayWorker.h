//
//  TexturePlayWorker.h
//  Mixer
//
//  Created by guohaoyang on 2024/6/20.
//

#ifndef TexturePlayWorker_h
#define TexturePlayWorker_h

#include "SDL.h"
#include <stdbool.h>
#include <libavutil/frame.h>

/**
 * 这里的代码处理视频画面的显示，主要和SDL相关
 */
typedef struct TexturePlayWorker TexturePlayWorker;

void TexturePlayWorkerCreate(TexturePlayWorker **worker,
                             int videoWidth,
                             int videoHeight,
                             SDL_PixelFormatEnum pixformat);

void TexturePlayWorkerDestroy(TexturePlayWorker **worker);
//返回值指示是否读完
bool TexturePlayWorkerUpdateBuffer(TexturePlayWorker *worker,AVFrame *frame);

#endif /* TexturePlayWorker_h */
