//
//  TexturePlayWorker.c
//  Mixer
//
//  Created by guohaoyang on 2024/6/20.
//

#include "TexturePlayWorker.h"

struct TexturePlayWorker {
    int                     videoWidth;
    int                     videoHeight;
    SDL_PixelFormatEnum     pixformat;
    
    SDL_Window              *window;
    SDL_Renderer            *renderer;
    SDL_Texture             *texture;
};

static void render(SDL_Texture *texture,
                   AVFrame *frame,
                   SDL_Renderer *renderer)
{
    SDL_UpdateYUVTexture(texture,
                         NULL,
                         frame->data[0],
                         frame->linesize[0],
                         frame->data[1],
                         frame->linesize[1],
                         frame->data[2],
                         frame->linesize[2]);
    
    SDL_RenderClear(renderer);
    //第三个参数传NULL使用纹理的全部全部渲染 最后一个参数表示渲染到整个窗口
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void TexturePlayWorkerCreate(TexturePlayWorker **worker,
                             int videoWidth,
                             int videoHeight,
                             SDL_PixelFormatEnum pixformat)
{
    SDL_Window *window =
    SDL_CreateWindow("",
                     SDL_WINDOWPOS_UNDEFINED,
                     SDL_WINDOWPOS_UNDEFINED,
                     800,
                     600,
                     SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Failed to Create Window - %s \n", SDL_GetError());
        return;
    }
    
    *worker = calloc(1, sizeof(TexturePlayWorker));
    (*worker)->videoWidth = videoWidth;
    (*worker)->videoHeight = videoHeight;
    (*worker)->pixformat = pixformat;
    
    (*worker)->window = window;
    
    (*worker)->renderer =
    SDL_CreateRenderer((*worker)->window, -1, 0);
    
    (*worker)->texture =
    SDL_CreateTexture((*worker)->renderer,
                      pixformat,
                      SDL_TEXTUREACCESS_STREAMING,
                      videoWidth,
                      videoHeight);
}

void TexturePlayWorkerDestroy(TexturePlayWorker **worker)
{
    if ((*worker)->window) {
        SDL_DestroyWindow((*worker)->window);
    }
    
    if ((*worker)->renderer) {
        SDL_DestroyRenderer((*worker)->renderer);
    }
    
    if ((*worker)->texture) {
        SDL_DestroyTexture((*worker)->texture);
    }
}

bool TexturePlayWorkerUpdateBuffer(TexturePlayWorker *worker,AVFrame *frame)
{
    render(worker->texture, frame, worker->renderer);
    return true;
}
