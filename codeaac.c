#include "codeaac.h"

void AACEnCoderInit(AACEnCoder *aac_encoder, int64_t bit_rate, uint64_t channel_layout, int sample_rate, int profile, enum AVSampleFormat sample_fmt)
{

    aac_encoder->codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!aac_encoder->codec)
    {
        perror("can't find encoder");
        exit(0);
    }

    aac_encoder->codec_ctx = avcodec_alloc_context3(aac_encoder->codec);
    if (!aac_encoder->codec_ctx)
    {
        perror("can't alloc code context");
        exit(0);
    }
    aac_encoder->codec_ctx->codec_id = AV_CODEC_ID_AAC;
    aac_encoder->codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    aac_encoder->codec_ctx->bit_rate = bit_rate;
    aac_encoder->codec_ctx->channel_layout = channel_layout;
    aac_encoder->codec_ctx->sample_rate = sample_rate;
    aac_encoder->codec_ctx->channels = av_get_channel_layout_nb_channels(channel_layout);
    aac_encoder->codec_ctx->profile = profile;
    aac_encoder->codec_ctx->sample_fmt = sample_fmt;

    if (!AACEncoderCheck(aac_encoder))
    {
        exit(0);
    }

    if (avcodec_open2(aac_encoder->codec_ctx, aac_encoder->codec, NULL) < 0)
    {
        perror("could not open codec");
        exit(0);
    }

    // 6.分配packet
    aac_encoder->pkt = av_packet_alloc();
    if (!aac_encoder->pkt)
    {
        perror("could not allocate the packet\n");
        exit(1);
    }

    // 7.分配frame
    aac_encoder->frame = av_frame_alloc();
    if (!aac_encoder->frame)
    {
        perror("Could not allocate audio frame\n");
        exit(1);
    }

    aac_encoder->frame->nb_samples = aac_encoder->codec_ctx->frame_size;
    aac_encoder->frame->format = aac_encoder->codec_ctx->sample_fmt;
    aac_encoder->frame->channel_layout = aac_encoder->codec_ctx->channel_layout;
    aac_encoder->frame->channels = aac_encoder->codec_ctx->channels;
    printf("frame nb_samples:%d\n", aac_encoder->frame->nb_samples);

    // 8.为frame分配buffer
    if (av_frame_get_buffer(aac_encoder->frame, 0) < 0)
    {
        perror("Could not allocate audio data buffers\n");
        exit(1);
    }

    aac_encoder->frame->pts = 0;
}

bool AACEnCoderCheckFormat(AACEnCoder *aac_encoder)
{
    /*check format*/
    const enum AVSampleFormat *p = aac_encoder->codec->sample_fmts;
    if (p == NULL)
    {
        printf("the code not set audio format\n");
        return true;
    }
    while (*p != AV_SAMPLE_FMT_NONE)
    {
        if (*p == aac_encoder->codec_ctx->sample_fmt)
            return true;
        p++;
    }
    if (*p == AV_SAMPLE_FMT_NONE)
    {
        printf("can't support sample format: %s\n", av_get_sample_fmt_name(aac_encoder->codec_ctx->sample_fmt));
        return false;
    }
}

bool AACEnCoderCheckSampleRate(AACEnCoder *aac_encoder)
{
    const int *p = aac_encoder->codec->supported_samplerates;
    if (p == NULL)
    {
        printf("the code not set sample_rate\n");
        return true;
    }
    while (*p != 0)
    {
        if (*p == aac_encoder->codec_ctx->sample_rate)
            return true;
        p++;
    }
    if (*p == 0)
    {
        printf("can't support sample rate: %dhz\n", aac_encoder->codec_ctx->sample_rate);
        return false;
    }
}

bool AACEnCoderCheckChannelLayout(AACEnCoder *aac_encoder)
{

    const uint64_t *p = aac_encoder->codec->channel_layouts;
    if (p == NULL)
    {
        printf("the code not set channel_layout\n");
        return true;
    }
    while (*p != 0)
    {
        if (*p == aac_encoder->codec_ctx->sample_rate)
            return true;
        p++;
    }
    if (*p == 0)
    {
        printf("can't support channel layout %s\n", av_get_channel_name(aac_encoder->codec_ctx->channel_layout));
        return false;
    }
}

bool AACEnCoderCheckProfile(AACEnCoder *aac_encoder)
{
    const AVProfile *p = aac_encoder->codec->profiles;
    if (p == NULL)
    {
        printf("the codec not set profile\n");
        return true;
    }
    while (p->profile != FF_PROFILE_UNKNOWN)
    {
        if (p->profile == aac_encoder->codec_ctx->profile)
        {
            return true;
        }
        p++;
    }
    printf("can't support profile %s\n", av_get_profile_name(aac_encoder->codec, aac_encoder->codec_ctx->profile));
    return false;
}

bool AACEncoderCheck(AACEnCoder *aac_encoder)
{
    if (!AACEnCoderCheckFormat(aac_encoder) || !AACEnCoderCheckSampleRate(aac_encoder) || !AACEnCoderCheckChannelLayout(aac_encoder) || !AACEnCoderCheckProfile(aac_encoder))
    {
        return false;
    }
    printf("AAC encoder config\n");
    printf("bit_rate:%ldkbps\n", aac_encoder->codec_ctx->bit_rate / 1024);
    printf("sample_rate:%d\n", aac_encoder->codec_ctx->sample_rate);
    printf("sample_fmt:%s\n", av_get_sample_fmt_name(aac_encoder->codec_ctx->sample_fmt));
    printf("channel layout:%s\tchannels:%d\n", av_get_channel_name(aac_encoder->codec_ctx->channel_layout), aac_encoder->codec_ctx->channels);
    printf("profile:%s\n", av_get_profile_name(aac_encoder->codec, aac_encoder->codec_ctx->profile));
    return true;
}

bool AACEnCoderFetchFrame(AACEnCoder *aac_encoder, void *frame_buf)
{
    if (av_frame_make_writable(aac_encoder->frame) != 0)
    {
        perror("av_frame write failed");
        return false;
    }
    if (av_samples_fill_arrays(aac_encoder->frame->data, aac_encoder->frame->linesize, frame_buf, aac_encoder->frame->channels, aac_encoder->frame->nb_samples, aac_encoder->frame->format, 0) < 0)
    {
        perror("frame samples fill arrays failed");
        return false;
    }
    aac_encoder->frame->pts += aac_encoder->frame->nb_samples;

    /* send the frame for encoding */
    int ret;
    ret = avcodec_send_frame(aac_encoder->codec_ctx, aac_encoder->frame);
    if (ret < 0)
    {
        perror("Error sending the frame to the encoder\n");
        return false;
    }
    return true;
}

int AACEnCoderEnCode(AACEnCoder *aac_encoder)
{
    int ret = avcodec_receive_packet(aac_encoder->codec_ctx, aac_encoder->pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        return 0;
    }
    else if (ret < 0)
    {
        perror("Error encoding audio frame\n");
        return -1;
    }
    return aac_encoder->pkt->size;
}

bool AACEncoderFlush(AACEnCoder *aac_encoder)
{
    /* send the frame for encoding */
    int ret;
    ret = avcodec_send_frame(aac_encoder->codec_ctx, NULL);
    if (ret < 0)
    {
        perror("Error sending the frame to the encoder\n");
        return false;
    }
    return true;
}

void AACAdtsHeaderGen(ADTSHeader *adts_header, AVCodecContext *codec_ctx, int data_size, IS_VARIABLE_BITSTREAM is_variable)
{
    uint8_t freq_idx = 0; // 0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
    switch (codec_ctx->sample_rate)
    {
    case 96000:
        freq_idx = 0;
        break;
    case 88200:
        freq_idx = 1;
        break;
    case 64000:
        freq_idx = 2;
        break;
    case 48000:
        freq_idx = 3;
        break;
    case 44100:
        freq_idx = 4;
        break;
    case 32000:
        freq_idx = 5;
        break;
    case 24000:
        freq_idx = 6;
        break;
    case 22050:
        freq_idx = 7;
        break;
    case 16000:
        freq_idx = 8;
        break;
    case 12000:
        freq_idx = 9;
        break;
    case 11025:
        freq_idx = 10;
        break;
    case 8000:
        freq_idx = 11;
        break;
    case 7350:
        freq_idx = 12;
        break;
    default:
        freq_idx = 4;
        break;
    }
    uint8_t chanCfg = codec_ctx->channels;
    uint32_t frame_length = data_size + 7;
    adts_header->header[0] = 0xFF;
    adts_header->header[1] = 0xF1;
    adts_header->header[2] = (codec_ctx->profile << 6) + (freq_idx << 2) + (chanCfg >> 2);
    adts_header->header[3] = ((chanCfg & 3) << 6) + (frame_length >> 11);
    adts_header->header[4] = (frame_length & 0X7FF) >> 3;
    if (is_variable == VARIABLE)
    {
        adts_header->header[5] = ((frame_length & 7) << 5) + 0x1F;
        adts_header->header[6] = 0xFC;
    }
    else if (is_variable == NONVARIABLE)
    {
        adts_header->header[5] = ((frame_length & 7) << 5) + 0x00;
        adts_header->header[6] = 0x00;
    }
}

void AACEncoderDestroy(AACEnCoder *aac_encoder)
{
    av_frame_free(&aac_encoder->frame);
    av_packet_free(&aac_encoder->pkt);
    avcodec_free_context(&aac_encoder->codec_ctx);
}

#define INPUT_FILE "audio.pcm"
#define OUTPUT_FILE "audio.aac"

int main(void)
{
    AACEnCoder aac_encoder;
    AACEnCoderInit(&aac_encoder, 128 * 1024, AV_CH_LAYOUT_STEREO, 44100, FF_PROFILE_AAC_LOW, AV_SAMPLE_FMT_FLTP);
    FILE *in_fp = fopen(INPUT_FILE, "rb");
    FILE *out_fp = fopen(OUTPUT_FILE, "wb");
    float pcm_buf[1024 * 2];
    ADTSHeader adts_header;
    int ret = 0;
    while (fread(pcm_buf, sizeof(float), 2048, in_fp))
    {
        AACEnCoderFetchFrame(&aac_encoder, pcm_buf);
        while (true)
        {
            ret = AACEnCoderEnCode(&aac_encoder);
            if (ret == -1)
            {
                perror("encode error");
                exit(0);
            }
            else if (ret == 0)
            {
                break;
            }
            AACAdtsHeaderGen(&adts_header, aac_encoder.codec_ctx, aac_encoder.pkt->size, NONVARIABLE);
            fwrite(&adts_header, sizeof(ADTSHeader), 1, out_fp);
            fwrite(aac_encoder.pkt->data, 1, aac_encoder.pkt->size, out_fp);
        }
    }
    AACEncoderFlush(&aac_encoder);
    while (true)
    {
        ret = AACEnCoderEnCode(&aac_encoder);
        if (ret == -1)
        {
            perror("encode error");
            exit(0);
        }
        else if (ret == 0)
        {
            break;
        }
        AACAdtsHeaderGen(&adts_header, aac_encoder.codec_ctx, aac_encoder.pkt->size, NONVARIABLE);
        fwrite(&adts_header, sizeof(ADTSHeader), 1, out_fp);
        fwrite(aac_encoder.pkt->data, 1, aac_encoder.pkt->size, out_fp);
    }
    return 0;
}