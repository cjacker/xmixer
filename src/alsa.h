#ifndef ALSA_H
#define ALSA_H
#include <alsa/asoundlib.h>
#include <glib.h>

typedef struct _Alsamixer Alsamixer;

struct _Alsamixer {
    snd_mixer_t *s_mixer;
    snd_mixer_elem_t *master;
    snd_mixer_elem_t *capture;
    int m_count;
    struct pollfd  *m_fds;
};

gboolean alsa_mixer_init(Alsamixer *mixer, int(*master_change)(snd_mixer_elem_t *elem, unsigned int mask), int(*capture_change)(snd_mixer_elem_t *elem, unsigned int mask));
void alsa_mixer_destroy(Alsamixer *mixer);

gboolean alsa_mixer_get_master_mute(Alsamixer *mixer);
void alsa_mixer_set_master_mute(Alsamixer *mixer, gboolean mute);

gboolean alsa_mixer_get_capture_mute(Alsamixer *mixer);
void alsa_mixer_set_capture_mute(Alsamixer *mixer, gboolean mute);

int alsa_mixer_get_master_volume(Alsamixer *mixer);
void alsa_mixer_set_master_volume(Alsamixer *mixer, int vol);

int alsa_mixer_get_capture_volume(Alsamixer *mixer);
void alsa_mixer_set_capture_volume(Alsamixer *mixer, int vol);
#endif
