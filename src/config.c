#include <glib.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "config.h"
#include "alsa.h"

void xmixer_get_config(gint *mute,gint *volume)
{
    GKeyFile * config = g_key_file_new();
    GError * err = NULL;

    gchar * filename = g_build_filename(g_get_home_dir(),"/.config/xmixerrc",NULL);

    if(!g_key_file_load_from_file(config,filename, G_KEY_FILE_NONE,&err))
        return;

    if(g_key_file_has_key(config,"xmixer","mute",&err))
        *mute = g_key_file_get_integer(config,"xmixer","mute",&err);

    if(g_key_file_has_key(config,"xmixer","volume",&err))
        *volume = g_key_file_get_integer(config,"xmixer","volume",&err);

    g_key_file_free(config);
}

void xmixer_save_config(gint mute, gint volume)
{
    GKeyFile * config = g_key_file_new();
    GError * err = NULL;
    gchar * configdir = g_build_filename(g_get_home_dir(),"/.config",NULL);
    if (! g_file_test(configdir, G_FILE_TEST_EXISTS)) {
        g_mkdir(configdir, 0700);
    }

    gchar * filename = g_build_filename(configdir ,"xmixerrc",NULL);

    g_key_file_set_integer(config,"xmixer","mute", mute);
    g_key_file_set_integer(config,"xmixer","volume", volume);
    gchar * content = NULL;
    gsize length = 0;
    content = g_key_file_to_data(config, &length, &err);
    if(!err){
        FILE *f = fopen(filename,"w");
        if (f != NULL) {
            fprintf(f,content);
            fclose(f);
        }
    }
    g_key_file_free(config);
    g_free (content);
}

