#include "codeh264.h"

void H264EnCoderInit(H264EnCoder *h264_encoder, int64_t bit_rate, int width, int height, AVRational rational, int profile, enum AVPixelFormat pixel_format)
{

    h264_encoder->codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!h264_encoder->codec)
    {
        perror("can't find encoder");
        exit(0);
    }

    h264_encoder->codec_ctx = avcodec_alloc_context3(h264_encoder->codec);
    if (!h264_encoder->codec_ctx)
    {
        perror("can't alloc code context");
        exit(0);
    }
    h264_encoder->codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    h264_encoder->codec_ctx->bit_rate = bit_rate;
    h264_encoder->codec_ctx->width = width;
    h264_encoder->codec_ctx->height = height;
    h264_encoder->codec_ctx->framerate = rational;
    h264_encoder->codec_ctx->time_base = (AVRational){rational.den, rational.num};
    h264_encoder->codec_ctx->profile = profile;
    h264_encoder->codec_ctx->pix_fmt = pixel_format;
    h264_encoder->codec_ctx->gop_size = 10;
    h264_encoder->codec_ctx->max_b_frames = 1;
    av_opt_set(h264_encoder->codec_ctx->priv_data, "preset", "slow", 0);
    if (!H264EnCoderCheck(h264_encoder))
    {
        exit(0);
    }

    if (avcodec_open2(h264_encoder->codec_ctx, h264_encoder->codec, NULL) < 0)
    {
        perror("could not open codec");
        exit(0);
    }
    // 6.分配packet
    h264_encoder->pkt = av_packet_alloc();
    if (!h264_encoder->pkt)
    {
        perror("could not allocate the packet\n");
        exit(1);
    }

    // 7.分配frame
    h264_encoder->frame = av_frame_alloc();
    if (!h264_encoder->frame)
    {
        perror("Could not allocate audio frame\n");
        exit(1);
    }

    h264_encoder->frame->format = h264_encoder->codec_ctx->pix_fmt;
    h264_encoder->frame->width = h264_encoder->codec_ctx->width;
    h264_encoder->frame->height = h264_encoder->codec_ctx->height;

    // 8.为frame分配buffer
    if (av_frame_get_buffer(h264_encoder->frame, 32) < 0)
    {
        perror("Could not allocate audio data buffers\n");
        exit(1);
    }
    h264_encoder->frame->pts = 0;
}

bool H264EnCoderCheckFormat(H264EnCoder *h264_encoder)
{
    const enum AVPixelFormat *p = h264_encoder->codec->pix_fmts;
    if (!p)
    {
        printf("the code not set pixel format");
        return true;
    }
    while (*p != AV_PIX_FMT_NONE)
    {
        if (*p == h264_encoder->codec_ctx->pix_fmt)
            return true;
        p++;
    }
    if (*p == AV_PIX_FMT_NONE)
    {
        printf("can't support sample format: %s\n", av_get_pix_fmt_name(h264_encoder->codec_ctx->pix_fmt));
        return false;
    }
}

bool H264EnCoderCheckFramerates(H264EnCoder *h264_encoder)
{
    const AVRational *p = h264_encoder->codec->supported_framerates;
    if (!p)
    {
        printf("the code not set frame rate");
        return true;
    }
    while (p->den != 0 && p->num != 0)
    {
        if (p->den == h264_encoder->codec_ctx->framerate.den && p->num == h264_encoder->codec_ctx->framerate.num)
            return true;
        p++;
    }
    if (p->den == 0 && p->num == 0)
    {
        printf("can't support frame rate: %d/%d fps\n", h264_encoder->codec_ctx->framerate.den, h264_encoder->codec_ctx->framerate.num);
        return false;
    }
}

bool H264EnCoderCheckProfile(H264EnCoder *h264_encoder)
{
    const AVProfile *p = h264_encoder->codec->profiles;
    if (p == NULL)
    {
        printf("the codec not set profile\n");
        return true;
    }
    while (p->profile != FF_PROFILE_UNKNOWN)
    {
        if (p->profile == h264_encoder->codec_ctx->profile)
        {
            return true;
        }
        p++;
    }
    printf("can't support profile %s\n", av_get_profile_name(h264_encoder->codec, h264_encoder->codec_ctx->profile));
    return false;
}

bool H264EnCoderCheck(H264EnCoder *h264_encoder)
{
    if (!H264EnCoderCheckFormat(h264_encoder) || !H264EnCoderCheckFramerates(h264_encoder) || !H264EnCoderCheckProfile(h264_encoder))
    {
        return false;
    }
    printf("h264 encoder config\n");
    printf("bit_rate:%ldkbps\n", h264_encoder->codec_ctx->bit_rate / 1024);
    printf("width:%d\theight:%d\n", h264_encoder->codec_ctx->width, h264_encoder->codec_ctx->height);
    printf("frame rates:%d/%d\n", h264_encoder->codec_ctx->framerate.den, h264_encoder->codec_ctx->framerate.num);
    printf("pixel_fmt:%s\n", av_get_pix_fmt_name(h264_encoder->codec_ctx->pix_fmt));
    printf("profile:%s\n", av_get_profile_name(h264_encoder->codec, h264_encoder->codec_ctx->profile));
    return true;
}

bool H264EnCoderFetchFrame(H264EnCoder *h264_encoder)
{
    // av_init_packet(h264_encoder->pkt);
    // h264_encoder->pkt->data = NULL;
    // h264_encoder->pkt->size = 0;

    int ret = avcodec_send_frame(h264_encoder->codec_ctx, h264_encoder->frame);
    if (ret < 0)
    {
        return false;
    }

    h264_encoder->frame->pts++;
    return true;
}

int H264EnCoderEncode(H264EnCoder *h264_encoder)
{
    int ret = avcodec_receive_packet(h264_encoder->codec_ctx, h264_encoder->pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        return 0;
    }
    else if (ret < 0)
    {
        perror("Error encoding audio frame\n");
        return -1;
    }
    return h264_encoder->pkt->size;
}

bool H264EnCoderFlush(H264EnCoder *h264_encoder)
{
    int ret;
    ret = avcodec_send_frame(h264_encoder->codec_ctx, NULL);
    if (ret < 0)
    {
        perror("Error sending the frame to the encoder\n");
        return false;
    }
    return true;
}

void H264EnCoderDestroy(H264EnCoder *h264_encoder)
{
    av_frame_free(&h264_encoder->frame);
    av_packet_free(&h264_encoder->pkt);
    avcodec_free_context(&h264_encoder->codec_ctx);
}

int main(void)
{
    int width = 1280;
    int height = 720;
    FILE *inputFile;
    FILE *outputFile;

    H264EnCoder h264_encoder;
    H264EnCoderInit(&h264_encoder, 400 * 1024, width, height, (AVRational){10, 1}, FF_PROFILE_H264_HIGH_444, AV_PIX_FMT_YUV420P);

    // 打开输入文件
    inputFile = fopen("video.yuv", "rb");
    if (!inputFile)
    {
        printf("无法打开输入文件\n");
        return -1;
    }

    // 打开输出文件
    outputFile = fopen("video.h264", "wb");
    if (!outputFile)
    {
        printf("无法打开输出文件\n");
        return -1;
    }

    // 逐帧读取 YUV 数据并编码为 H.264

    while (1)
    {
        int ret = av_frame_make_writable(h264_encoder.frame);
        if (ret < 0)
        {
            break;
        }
        if (fread(h264_encoder.frame->data[0], 1, width * height, inputFile) <= 0 ||
            fread(h264_encoder.frame->data[1], 1, width * height / 4, inputFile) <= 0 ||
            fread(h264_encoder.frame->data[2], 1, width * height / 4, inputFile) <= 0)
        {
            break;
        }

        if (!H264EnCoderFetchFrame(&h264_encoder))
        {
            printf("fetch error!\n");
            break;
        }

        while (H264EnCoderEncode(&h264_encoder) > 0)
        {
            // 写入编码数据到输出文件
            fwrite(h264_encoder.pkt->data, 1, h264_encoder.pkt->size, outputFile);
        }
    }
    H264EnCoderFlush(&h264_encoder);
    while (H264EnCoderEncode(&h264_encoder) > 0)
    {
        // 写入编码数据到输出文件
        fwrite(h264_encoder.pkt->data, 1, h264_encoder.pkt->size, outputFile);
        av_packet_unref(h264_encoder.pkt);
    }
    // 释放资源
    fclose(inputFile);
    fclose(outputFile);
    H264EnCoderDestroy(&h264_encoder);
    return 0;
}
