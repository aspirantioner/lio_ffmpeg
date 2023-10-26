#ifndef _CODEAAC_H
#define _CODEAAC_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>

typedef enum
{
    VARIABLE,
    NONVARIABLE
} IS_VARIABLE_BITSTREAM;

typedef struct
{
    AVCodec *codec;
    AVCodecContext *codec_ctx;
    AVPacket *pkt;
    AVFrame *frame;
} AACEnCoder;

typedef struct
{
    uint8_t header[7];
} ADTSHeader;

void AACEnCoderInit(AACEnCoder *aac_encoder, int64_t bit_rate, uint64_t channel_layout, int sample_rate, int profile, enum AVSampleFormat sample_fmt);
bool AACEnCoderCheckFormat(AACEnCoder *aac_encoder);
bool AACEnCoderCheckSampleRate(AACEnCoder *aac_encoder);
bool AACEnCoderCheckChannelLayout(AACEnCoder *aac_encoder);
bool AACEnCoderCheckProfile(AACEnCoder *aac_encoder);
bool AACEncoderCheck(AACEnCoder *aac_encoder);
bool AACEnCoderFetchFrame(AACEnCoder *aac_encoder, void *frame_buf);
int AACEnCoderEnCode(AACEnCoder *aac_encoder);
bool AACEncoderFlush(AACEnCoder *aac_encoder);
void AACAdtsHeaderGen(ADTSHeader *adts_header, AVCodecContext *codec_ctx, int data_size, IS_VARIABLE_BITSTREAM is_variable);
void AACEncoderDestroy(AACEnCoder *aac_encoder);
#endif