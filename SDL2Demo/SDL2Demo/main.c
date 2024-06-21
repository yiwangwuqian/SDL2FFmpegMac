//
//  main.c
//  SDL2Demo
//
//  Created by guohaoyang on 2024/6/4.
//

/**
 * 整个项目的代码是从iOS的项目拿过来的，验证有没有流程以及有没有API的使用问题。
 *
 * 奇怪的是在iOS的模拟器上启动有崩溃，而到了电脑这一端没有出现。
 * 目前暂时没有精力去找这个问题并修复它，主要目的是熟悉ffmpeg。
 */
#include <stdio.h> 
#include <SDL2/SDL.h>
#include <unistd.h>
#include <time.h>
#include "VideoFileDecoder.h"
#include "TexturePlayWorker.h"
#include "AudioPlayWorker.h"

static TexturePlayWorker    *textureWorker = NULL;
static AudioPlayWorker      *audioWorker = NULL;

//⚠️这个数组的长度用的定值需要根据策略改成其它方案
static uint8_t bufferData[4098];
static int bufferSize = 0;

static void decodeOnceFrame (enum AVMediaType type, void *data, int len)
{
    switch (type) {
        case AVMEDIA_TYPE_VIDEO:
            if (textureWorker) {
                AVFrame *onceFrame = (AVFrame *)data;
                TexturePlayWorkerUpdateBuffer(textureWorker, onceFrame);
            }
            break;
        case AVMEDIA_TYPE_AUDIO:
            if (len > 0) {
                memcpy(bufferData, data, len);
            } else {
                //这样写也需要优化
                memset(bufferData, 0, bufferSize);
            }
            bufferSize = len;
            
            if (audioWorker) {
                AudioPlayWorkerUpdateBuffer2(audioWorker, bufferData, bufferSize);
            }
            break;
        default:
            break;
    }
}

int main(int argc, const char * argv[])
{
    //1.判断输入参数                          这步我简化了
    const char *path = "";
    if(access(path, F_OK) != 0) {
        printf("文件不存在!");
    }
    
    int ret = -1;
    
    VideoFileDecoder *decoder = NULL;
    VideoFileDecoderCreate(&decoder, path, decodeOnceFrame);
    if (decoder == NULL) {
        goto __END;
    }
    
    SDL_PixelFormatEnum pixformat = 0;
    int video_width = 0;
    int video_height = 0;
    
    av_log_set_level(AV_LOG_DEBUG);
    
    //2.初始化SDL，并创建窗口和Renderer
    //2.1
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Couldnt initialize SDL - %s \n", SDL_GetError());
        return -1;
    }
    
    //9.根据视频的宽/高创建纹理
    VideoFileDecoderGetSize(decoder, &video_width, &video_height);
    pixformat = SDL_PIXELFORMAT_IYUV;
    TexturePlayWorkerCreate(&textureWorker,video_width, video_height, pixformat);
    AudioPlayWorkerCreate(&audioWorker);
    
#ifdef DEBUG
    time_t s_timer = time(NULL);
#endif
    
    //10.从多媒体文件中读取数据，进行解码
    while (!VideoFileDecoderUpdateBuffer(decoder)) {
        //处理SDL事件
        SDL_Event event;
        SDL_PollEvent(&event);//⚠️不调用屏幕就不会刷新
        switch (event.type) {
            case SDL_QUIT:
                break;
            default:
                break;
        }
    }
    
#ifdef DEBUG
    time_t e_timer = time(NULL);
    printf("共耗时 difftime %f\n", difftime(e_timer,s_timer));
#endif
    
__QUIT:
    ret = 0;
__END:
    if (decoder) {
        VideoFileDecoderDestroy(&decoder);
    }
  
    if (textureWorker) {
        TexturePlayWorkerDestroy(&textureWorker);
    }
    SDL_Quit();
    
    return ret;
}
