#include "../audio/lio_soundcard.h"
#include "../video/lio_camera.h"
#include "../video/format_convert.h"
#include <pthread.h>

#define TIME 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void *camera_pthread(void *args)
{
    LioCamera *lio_camera = args;
    FILE *fp = fopen("./video.yuv", "wb");

    int width = lio_camera->fmt.fmt.pix.width;
    int height = lio_camera->fmt.fmt.pix.height;
    unsigned char yuv_buff[width * height * 3 / 2];
    unsigned char *yuyv_buff;
    LioCameraStartStream(lio_camera);
    int count = 0;
    while (count < TIME * 10)
    {
        pthread_mutex_lock(&mutex);
        yuyv_buff = LioCameraFetchStream(lio_camera);
        pthread_mutex_unlock(&mutex);
        yuyv422_to_yuv420(yuyv_buff, yuv_buff, width, height);
        fwrite(yuv_buff, width * height * 1.5, 1, fp);
        LioCameraPutStream(lio_camera);
        count++;
    }
    LioCameraStopStream(lio_camera);
    LioCameraDestroy(lio_camera);
    return NULL;
};

void *audio_pthread(void *args)
{
    LioSoundCard *lio_soundcard = args;
    FILE *fp = fopen("audio.pcm", "wb");
    int sum = 44100 * TIME;
    int count = 0;
    while (count < sum)
    {
        pthread_mutex_lock(&mutex);
        LioSoundCardFetchFrame(lio_soundcard);
        pthread_mutex_unlock(&mutex);
        fwrite(lio_soundcard->rw_buf.rw_buffer, lio_soundcard->read_buffer_size, 1, fp);
        count += 1024;
    }
    LioSoundCardClose(lio_soundcard);
    return NULL;
};
int main(void)
{

    LioCamera lio_camera;
    LioSoundCard lio_soundcard;
    LioCameraOpen(&lio_camera, "/dev/video0");
    LioCameraSetFormat(&lio_camera, V4L2_PIX_FMT_YUYV, 1280, 720);
    LioCameraSetFps(&lio_camera, 10, 1);
    LioCameraBufRequest(&lio_camera, 4);

    LioSoundCardInit(&lio_soundcard, SND_PCM_STREAM_CAPTURE, 44100, 1024, SND_PCM_ACCESS_RW_INTERLEAVED, SND_PCM_FORMAT_S16_LE, 2);

    pthread_t pthread_camera, pthread_audio;
    pthread_create(&pthread_camera, NULL, camera_pthread, &lio_camera);
    pthread_create(&pthread_audio, NULL, audio_pthread, &lio_soundcard);
    // pthread_detach(pthread_camera);
    // pthread_detach(pthread_audio);
    pthread_join(pthread_camera, NULL);
    pthread_join(pthread_audio, NULL);
    return 0;
    // pthread_exit(NULL);
    //  return 0;
}