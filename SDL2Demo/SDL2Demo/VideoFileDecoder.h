//
//  VideoFileDecoder.h
//  Mixer
//
//  Created by guohaoyang on 2024/6/20.
//

#ifndef VideoFileDecoder_h
#define VideoFileDecoder_h

#include <stdbool.h>
#include <libavutil/avutil.h>

//解码数据时的回调，一次一个frame，回调参数可能会重复修改
typedef void (*VideoFileDecoderCallback) (enum AVMediaType type, void *data, int len);

/**
 * 和一个具体文件相关，对应一次文件打开后的所需操作。
 */
typedef struct VideoFileDecoder VideoFileDecoder;

void VideoFileDecoderCreate(VideoFileDecoder **decoder,
                            const char *path,
                            VideoFileDecoderCallback callback);

void VideoFileDecoderDestroy(VideoFileDecoder **decoder);

void VideoFileDecoderGetSize(VideoFileDecoder *decoder, int *width, int *height);

//返回值指示是否读完
bool VideoFileDecoderUpdateBuffer(VideoFileDecoder *decoder);

#endif /* VideoFileDecoder_h */
