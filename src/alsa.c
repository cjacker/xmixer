#include <alsa/asoundlib.h>
#include "alsa.h"

gboolean alsa_mixer_get_capture_mute(Alsamixer *mixer)
{
    int onoff;
    if(snd_mixer_selem_has_capture_switch(mixer->capture))
        snd_mixer_selem_get_capture_switch(mixer->capture, 0, &onoff);
    return !onoff;
}

void alsa_mixer_set_capture_mute(Alsamixer *mixer, gboolean mute)
{
    int chn;
    for (chn=0;chn<=SND_MIXER_SCHN_LAST;chn++) {
        if(snd_mixer_selem_has_capture_switch(mixer->capture))
            snd_mixer_selem_set_capture_switch(mixer->capture, chn, !mute);
    }
}

gboolean alsa_mixer_get_master_mute(Alsamixer *mixer)
{
    int pswitch;
    if(snd_mixer_selem_has_playback_switch(mixer->master)) 
        snd_mixer_selem_get_playback_switch(mixer->master,0,&pswitch);
    return !pswitch;
}

void alsa_mixer_set_master_mute(Alsamixer *mixer, gboolean mute)
{
    int chn;
    for (chn=0;chn<=SND_MIXER_SCHN_LAST;chn++) {
        if(snd_mixer_selem_has_playback_switch(mixer->master)) 
            snd_mixer_selem_set_playback_switch(mixer->master, chn, !mute);
    }
}

int alsa_mixer_get_master_volume(Alsamixer *mixer)
{
  long al,ar;
  if(snd_mixer_selem_has_playback_volume(mixer->master)) {
    snd_mixer_selem_get_playback_volume(mixer->master, SND_MIXER_SCHN_FRONT_LEFT, &al);
    snd_mixer_selem_get_playback_volume(mixer->master, SND_MIXER_SCHN_FRONT_RIGHT, &ar);
  }
  return (al+ar)>>1;
}

void alsa_mixer_set_master_volume(Alsamixer *mixer,int vol)
{
  if(snd_mixer_selem_has_playback_volume(mixer->master)) {
    snd_mixer_selem_set_playback_volume(mixer->master, SND_MIXER_SCHN_FRONT_LEFT, vol);
    snd_mixer_selem_set_playback_volume(mixer->master, SND_MIXER_SCHN_FRONT_RIGHT, vol);
  }
}


int alsa_mixer_get_capture_volume(Alsamixer *mixer)
{
  long al,ar;
  if(snd_mixer_selem_has_capture_volume(mixer->capture)) {
    snd_mixer_selem_get_capture_volume(mixer->capture, SND_MIXER_SCHN_FRONT_LEFT, &al);
    snd_mixer_selem_get_capture_volume(mixer->capture, SND_MIXER_SCHN_FRONT_RIGHT, &ar);
  }
  return (al+ar)>>1;
}

void alsa_mixer_set_capture_volume(Alsamixer *mixer,int vol)
{
  if(snd_mixer_selem_has_capture_volume(mixer->capture)) {
    snd_mixer_selem_set_capture_volume(mixer->capture, SND_MIXER_SCHN_FRONT_LEFT, vol);
    snd_mixer_selem_set_capture_volume(mixer->capture, SND_MIXER_SCHN_FRONT_RIGHT, vol);
  }
}

gboolean listenmixer (gpointer data)
{
    Alsamixer *mixer = (Alsamixer *)data;
    int finished = poll(mixer->m_fds, mixer->m_count, 10);
    if (finished > 0) {

        unsigned short revents;

        if (snd_mixer_poll_descriptors_revents(mixer->s_mixer, mixer->m_fds, mixer->m_count, &revents) >= 0) {
            if (revents & POLLNVAL) {
                return FALSE;
            }
            if (revents & POLLERR) {
                return FALSE;
            }
            if (revents & POLLIN) {
                snd_mixer_handle_events(mixer->s_mixer);
            }
        }
    }
    return TRUE;
}

gboolean alsa_mixer_init(Alsamixer *mixer, int(*master_change)(snd_mixer_elem_t *elem, unsigned int mask), int(*capture_change)(snd_mixer_elem_t *elem, unsigned int mask))
{
    int err;
    err = snd_mixer_open(&(mixer->s_mixer),0);
    if(err){
        fprintf(stderr,"Mixer open error!\n");
        return FALSE;
    }

    err = snd_mixer_attach(mixer->s_mixer,"default");
    if(err){
        fprintf(stderr,"Mixer attach error!\n");
        return FALSE;
    }
    err = snd_mixer_selem_register(mixer->s_mixer,NULL,NULL);
    if(err){
        fprintf(stderr,"Mixer register error!\n");
        return FALSE;
    }

    //snd_mixer_set_callback(mixer->s_mixer, mixer_callback);

    err = snd_mixer_load(mixer->s_mixer);
    if(err){
        fprintf(stderr,"Mixer load error!\n");
        return FALSE;
    }

    mixer->master = snd_mixer_first_elem(mixer->s_mixer);
    while(strcmp("Master",snd_mixer_selem_get_name(mixer->master))) {
        mixer->master = snd_mixer_elem_next(mixer->master);
    }

    mixer->capture = snd_mixer_first_elem(mixer->s_mixer);
    while(strcmp("Capture",snd_mixer_selem_get_name(mixer->capture))) {
        mixer->capture = snd_mixer_elem_next(mixer->capture);
    }

    if(strcmp("Master", snd_mixer_selem_get_name(mixer->master))) {
        mixer->master = NULL;
    }

    if(strcmp("Capture", snd_mixer_selem_get_name(mixer->capture))) {
        mixer->capture = NULL;
    }

    if(mixer->master){
        snd_mixer_selem_set_playback_volume_range(mixer->master, 0, 100);
        snd_mixer_elem_set_callback(mixer->master, master_change);
    }
    if(mixer->capture){
        snd_mixer_selem_set_capture_volume_range(mixer->capture, 0, 100);
        snd_mixer_elem_set_callback(mixer->capture, capture_change);
    }

    if((mixer->m_count = snd_mixer_poll_descriptors_count(mixer->s_mixer)) < 0) {
        fprintf(stderr, "pool error\n");
        return FALSE;
    }

    mixer->m_fds = (struct pollfd*)calloc(mixer->m_count, sizeof(struct pollfd));
    if (mixer->m_fds == NULL) {
        fprintf(stderr, "calloc = NULL\n");
        return FALSE;
    }

    mixer->m_fds->events = POLLIN;
    if ((err = snd_mixer_poll_descriptors(mixer->s_mixer, mixer->m_fds, mixer->m_count)) < 0) {
        fprintf(stderr, "snd_mixer_poll_descriptors_count() err=\n");
        return FALSE;
    }
    if (err != mixer->m_count) {
        fprintf(stderr,"snd_mixer_poll_descriptors_count() err= %d m_count= %d", err, mixer->m_count);
        return FALSE;
    }
    
    g_timeout_add(500, listenmixer, mixer);
    return TRUE;
}


void alsa_destroy(Alsamixer *mixer)
{
    snd_mixer_close(mixer->s_mixer);
}


