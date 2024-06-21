//
//  VideoFileDecoder.c
//  Mixer
//
//  Created by guohaoyang on 2024/6/20.
//

#include "VideoFileDecoder.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

static int VideoFileDecoderDecode(VideoFileDecoder *decoder);

struct VideoFileDecoder {
    char                *fPath;
    AVFormatContext     *fmtCtx;
    
    //------视频相关
    int                 vWidth;
    int                 vHeight;
    AVStream            *vInStream;
    int                 vIdx;
    const AVCodec       *vDec;
    AVCodecContext      *vCtx;
    AVPacket            *vPkt;
    AVFrame             *vFrame;
    
    VideoFileDecoderCallback callback;
    
    //------音频相关
    int                 aIdx;
    const AVCodec       *aDec;
    AVCodecContext      *aCtx;
    SwrContext          *swr_context;
};

void VideoFileDecoderCreate(VideoFileDecoder **decoder,
                            const char *path,
                            VideoFileDecoderCallback callback)
{
    *decoder = calloc(1, sizeof(VideoFileDecoder));
    AVFormatContext     *fmtCtx = NULL;
    
    int ret = 0;
    //打开多媒体文件，并获得流信息
    if ( (ret = avformat_open_input(&fmtCtx, path, NULL, NULL)) < 0){
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        VideoFileDecoderDestroy(decoder);
        return;
    }
    (*decoder)->fmtCtx = fmtCtx;
    
    if ( (ret = avformat_find_stream_info(fmtCtx, NULL) ) < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        VideoFileDecoderDestroy(decoder);
        return;
    }
    
    //查找最好的视频流
    int vIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (vIdx < 0) {
        av_log(fmtCtx, AV_LOG_ERROR, "Does not include video stream! \n");
        VideoFileDecoderDestroy(decoder);
        return;
    }
    (*decoder)->vIdx = vIdx;
    
    //根据流中的code_id，获得解码器
    AVStream *vInStream = fmtCtx->streams[vIdx];
    const AVCodec *vDec= avcodec_find_decoder(vInStream->codecpar->codec_id);
    (*decoder)->vInStream = vInStream;
    (*decoder)->vDec = vDec;
    if (!vDec) {
        av_log(NULL, AV_LOG_ERROR, "Couldnt find dec %d", vInStream->codecpar->codec_id);
        VideoFileDecoderDestroy(decoder);
        return;
    }
    
    //创建解码器上下文
    AVCodecContext *vCtx = avcodec_alloc_context3(vDec);
    if (!vCtx) {
        av_log(NULL, AV_LOG_ERROR, "NO MEMORY\n");
        VideoFileDecoderDestroy(decoder);
        return;
    }
    (*decoder)->vCtx = vCtx;
    
    //从视频流中拷贝解码器参数到解码器上下文中
    ret = avcodec_parameters_to_context(vCtx, vInStream->codecpar);
    if (ret < 0) {
        av_log(vCtx, AV_LOG_ERROR, "Could not copy codecpar to codec ctx!");
        VideoFileDecoderDestroy(decoder);
        return;
    }
    
    //绑定解码器上下文
    ret = avcodec_open2(vCtx, vDec, NULL);
    if (ret < 0) {
        av_log(vCtx, AV_LOG_ERROR, "Dont open codec: %s \n", av_err2str(ret));
        VideoFileDecoderDestroy(decoder);
        return;
    }
    
    //查找最好的音频流
    int aIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (aIdx < 0) {
        av_log(fmtCtx, AV_LOG_ERROR, "Does not include video stream! \n");
        VideoFileDecoderDestroy(decoder);
        return;
    }
    (*decoder)->aIdx = aIdx;
    
    //根据流中的code_id，获得解码器
    AVStream *aInStream = fmtCtx->streams[aIdx];
    const AVCodec *aDec= avcodec_find_decoder(aInStream->codecpar->codec_id);
    (*decoder)->aDec = aDec;
    if (!aDec) {
        av_log(NULL, AV_LOG_ERROR, "Couldnt find dec %d", aInStream->codecpar->codec_id);
        VideoFileDecoderDestroy(decoder);
        return;
    }
    
    //创建解码器上下文
    AVCodecContext *aCtx = avcodec_alloc_context3(aDec);
    if (!aCtx) {
        av_log(NULL, AV_LOG_ERROR, "NO MEMORY\n");
        VideoFileDecoderDestroy(decoder);
        return;
    }
    (*decoder)->aCtx = aCtx;
    
    //从视频流中拷贝解码器参数到解码器上下文中
    ret = avcodec_parameters_to_context(aCtx, aInStream->codecpar);
    if (ret < 0) {
        av_log(aCtx, AV_LOG_ERROR, "Could not copy codecpar to codec ctx!");
        VideoFileDecoderDestroy(decoder);
        return;
    }
    
    //绑定解码器上下文
    ret = avcodec_open2(aCtx, aDec, NULL);
    if (ret < 0) {
        av_log(aCtx, AV_LOG_ERROR, "Dont open codec: %s \n", av_err2str(ret));
        VideoFileDecoderDestroy(decoder);
        return;
    }
    SwrContext *swr_context = NULL;
    swr_alloc_set_opts2(
                        &swr_context,
                        &aInStream->codecpar->ch_layout,
                        AV_SAMPLE_FMT_S16,
                        aInStream->codecpar->sample_rate,
                        &aInStream->codecpar->ch_layout,
                        aInStream->codecpar->format,
                        aInStream->codecpar->sample_rate,
                        0,
                        NULL);
    (*decoder)->swr_context = swr_context;
    
    //根据视频的宽/高创建纹理
    (*decoder)->vWidth = vCtx->width;
    (*decoder)->vHeight = vCtx->height;

    (*decoder)->vPkt = av_packet_alloc();
    (*decoder)->vFrame = av_frame_alloc();
    
    unsigned long len = strlen(path);
    (*decoder)->fPath = malloc(len);
    strcpy((*decoder)->fPath, path);
    
    (*decoder)->callback = callback;
}

void VideoFileDecoderDestroy(VideoFileDecoder **decoder)
{
    if (*decoder == NULL) {
        return;
    }
    VideoFileDecoder *thisDecoder = *decoder;
    if (thisDecoder->fPath) {
        free(thisDecoder->fPath);
    }
    
    if (thisDecoder->fmtCtx) {
        avformat_close_input(&thisDecoder->fmtCtx);
    }
    
    if (thisDecoder->vCtx) {
        avcodec_free_context(&thisDecoder->vCtx);
    }
    
    if (thisDecoder->vFrame) {
        av_frame_free(&thisDecoder->vFrame);
    }
    
    if (thisDecoder->vPkt) {
        av_packet_free(&thisDecoder->vPkt);
    }
    
    if (thisDecoder->aCtx) {
        avcodec_free_context(&thisDecoder->aCtx);
    }
    
    if (thisDecoder->swr_context) {
        swr_free(&thisDecoder->swr_context);
    }
    
    free(thisDecoder);
    decoder = NULL;
}

void VideoFileDecoderGetSize(VideoFileDecoder *decoder, int *width, int *height)
{
    if (decoder == NULL) {
        return;
    }
    
    if (width != NULL) {
        *width = decoder->vWidth;
    }
    if (height != NULL) {
        *height = decoder->vHeight;
    }
}

bool VideoFileDecoderUpdateBuffer(VideoFileDecoder *decoder)
{
    //从多媒体文件中读取数据，进行解码
    
    //必须得是>=0 不为负数即可
    while (av_read_frame(decoder->fmtCtx, decoder->vPkt) >= 0) {
        if (decoder->vPkt->stream_index == decoder->vIdx || decoder->vPkt->stream_index == decoder->aIdx) {
            //对解码后的视频帧进行渲染
            VideoFileDecoderDecode(decoder);
        }
        av_packet_unref(decoder->vPkt);
        
        return false;
    }
    
    //将解码器缓冲区暂存数据清出来
    VideoFileDecoderDecode(decoder);
    
    return true;
}

#pragma mark- Private

static int VideoFileDecoderDecode(VideoFileDecoder *decoder)
{
    AVPacket *pkt = decoder->vPkt;
    AVCodecContext *ctx = pkt->stream_index == decoder->aIdx ? decoder->aCtx : decoder->vCtx;
    AVFrame *frame = decoder->vFrame;
    
    int ret = -1;
    ret = avcodec_send_packet(ctx, pkt);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder! %s \n", av_err2str(ret));
    }
    
    while (ret >=0 ) {
        ret = avcodec_receive_frame(ctx, frame);
        if (ret == AVERROR(EAGAIN)) {
            ret = 0;
            goto __OUT;
        } else if (ret < 0) {
            ret = -1;
            goto __OUT;
        }
        if (pkt->stream_index == decoder->aIdx) {
            AVFrame *resampled_frame = av_frame_alloc();
            resampled_frame->sample_rate = frame->sample_rate;
            resampled_frame->ch_layout = frame->ch_layout;
            resampled_frame->ch_layout.nb_channels = frame->ch_layout.nb_channels;
            resampled_frame->format = AV_SAMPLE_FMT_S16;

            ret = swr_convert_frame(decoder->swr_context, resampled_frame, frame);

            if (decoder->callback) {
                int size = resampled_frame->nb_samples * resampled_frame->ch_layout.nb_channels * 2;
                decoder->callback(AVMEDIA_TYPE_AUDIO, resampled_frame->data[0], size);
            }
            
            av_frame_free(&resampled_frame);
        } else if (pkt->stream_index == decoder->vIdx) {
            if (decoder->callback) {
                decoder->callback(AVMEDIA_TYPE_VIDEO, frame, 0);
            }
        }
        av_frame_unref(frame);
    }
    
__OUT:
    return ret;
}
