#ifndef _CODEH264_H
#define _CODEH264_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

typedef struct
{
    AVCodec *codec;
    AVCodecContext *codec_ctx;
    AVPacket *pkt;
    AVFrame *frame;
} H264EnCoder;

void H264EnCoderInit(H264EnCoder *h264_encoder, int64_t bit_rate, int width, int height, AVRational rational, int profile, enum AVPixelFormat pixel_format);
bool H264EnCoderCheckFormat(H264EnCoder *h264_encoder);
bool H264EnCoderCheckFramerates(H264EnCoder *h264_encoder);
bool H264EnCoderCheckProfile(H264EnCoder *h264_encoder);
bool H264EnCoderCheck(H264EnCoder *h264_encoder);
bool H264EnCoderFetchFrame(H264EnCoder *h264_encoder);
int H264EnCoderEncode(H264EnCoder *h264_encoder);
bool H264EnCoderFlush(H264EnCoder *h264_encoder);
void H264EnCoderDestroy(H264EnCoder *h264_encoder);
#endif