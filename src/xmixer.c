#include <alsa/asoundlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib.h>
#include <canberra-gtk.h>

#include "keybinder.h"
#include "alsa.h"
#include "config.h"

static Alsamixer *mixer = NULL;

static ca_context *context = NULL;

static GtkBuilder * ui_builder = NULL;


static GtkStatusIcon * xmixer = NULL;
static GtkWidget *osd = NULL;
static GtkWidget *rosd = NULL;

/*set_state/set_mode will trigger "toggled" signal*/
static gulong mute_button_handler_id = 0;

static guint timer_handle = 0;

void set_mixer_status(GtkStatusIcon *status)
{
    int mute = alsa_mixer_get_master_mute(mixer);
    int volume = alsa_mixer_get_master_volume(mixer);
    static const gchar* icon_names[] = {
        "audio-volume-muted",
        "audio-volume-low",
        "audio-volume-high",
        "audio-volume-medium"
    };

    int id = 0;
    if(mute) id = 0;
    else if(volume <= 30) id = 1;
    else if(volume >= 70) id = 2;
    else id = 3;

    GdkPixbuf* pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
            icon_names[id], 22,
            GTK_ICON_LOOKUP_FORCE_SIZE|GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
    gtk_status_icon_set_from_pixbuf(status, pb);
    g_object_unref(pb);

    if(ui_builder) {
        GtkWidget *volume_scale = (GtkWidget *)gtk_builder_get_object(ui_builder, "volume_scale");
        GtkWidget *mute_button = (GtkWidget *)gtk_builder_get_object(ui_builder, "mute_button");
        if(volume_scale && mixer)
            gtk_range_set_value(GTK_RANGE(volume_scale),alsa_mixer_get_master_volume(mixer));
        if(mute_button && mixer) {
            g_signal_handler_block(GTK_TOGGLE_BUTTON(mute_button),mute_button_handler_id);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mute_button),alsa_mixer_get_master_mute(mixer));
            g_signal_handler_unblock(GTK_TOGGLE_BUTTON(mute_button),mute_button_handler_id);
        }
    }
}

gboolean hide_window(gpointer data)
{
    int x,y;
    GtkWidget * window = GTK_WIDGET(data);
    GdkDevice *dev;
    GdkDeviceManager *devmgr;

    devmgr = gdk_display_get_device_manager(gtk_widget_get_display(window));

    dev = gdk_device_manager_get_client_pointer(devmgr);

    gdk_window_get_device_position(gtk_widget_get_window(window), dev, &x, &y, NULL);


    GtkAllocation allocation;
    gtk_widget_get_allocation(window, &allocation);

    if( (x < 0 || x > allocation.width) || (y < 0 || y > allocation.height)){
        //mouse is out of the window, should hide and remove the timeout.
        gtk_widget_hide(window);
        return FALSE;
    }
    //other wise, the timer will continure for next 2000 ms
    return TRUE;
}


void mixer_activated(GtkStatusIcon *statusicon, gpointer data)
{
    GtkWidget *window = (GtkWidget *)data;
    if(gtk_widget_get_visible(window)) {
        if(timer_handle) {
            g_source_remove(timer_handle);
            timer_handle = 0;
        }
        gtk_widget_hide(window);
    } else {
        gtk_widget_show(window);
        timer_handle = g_timeout_add(2000, hide_window, window); 
    }
}

void on_value_changed(GtkWidget *scale,gpointer data)
{
    double vol = gtk_range_get_value(GTK_RANGE(scale));
    alsa_mixer_set_master_volume(mixer,vol);
    set_mixer_status(xmixer);
    xmixer_save_config(alsa_mixer_get_master_mute(mixer), alsa_mixer_get_master_volume(mixer));
}

void on_mute_toggled(GtkWidget *mutebutton, gpointer data)
{
    int mute = alsa_mixer_get_master_mute(mixer);
    alsa_mixer_set_master_mute(mixer,!mute);
    set_mixer_status(xmixer);
    xmixer_save_config(alsa_mixer_get_master_mute(mixer), alsa_mixer_get_master_volume(mixer));
}

void move_window_with_status_icon(GtkStatusIcon *statusicon, GtkWidget *window)
{
    GdkRectangle rec;
    gtk_status_icon_get_geometry(statusicon,NULL,&rec,NULL);
    int window_width, window_height;
    gtk_window_get_size(GTK_WINDOW(window), &window_width, &window_height);

    int x = rec.x + rec.width/2 - window_width/2;
    if(x < 0)
        x = 0;
    else if(x + window_width > gdk_screen_width())
       x = gdk_screen_width() - window_width;

    if(rec.y > gdk_screen_height()/2)
        gtk_window_move(GTK_WINDOW(window),
                x,
                rec.y-(gdk_screen_height() - rec.height-rec.y)-window_height);
    else {
        gtk_window_move(GTK_WINDOW(window),
                x,
                rec.y + rec.height + rec.y);
    }
}


void on_mixer_slider_show(GtkWidget *window, gpointer data)
{
    move_window_with_status_icon(GTK_STATUS_ICON(data), window);
}

GtkWidget * create_mixer_frame(GtkStatusIcon *statusicon)
{        

    //mute_button = gtk_toggle_button_new_with_label("Mute");
    GtkWidget *mute_button = (GtkWidget *)gtk_builder_get_object(ui_builder, "mute_button");
    GtkWidget *window = (GtkWidget *)gtk_builder_get_object(ui_builder, "window");
    GtkWidget *volume_scale = (GtkWidget *)gtk_builder_get_object(ui_builder, "volume_scale"); 
    
    mute_button_handler_id = g_signal_connect(G_OBJECT(mute_button),"toggled",G_CALLBACK(on_mute_toggled),NULL);


    //the position of statusicon may be changed on fly.
    g_signal_connect(G_OBJECT(window),"show",G_CALLBACK(on_mixer_slider_show), statusicon);
   
    g_signal_connect(G_OBJECT(volume_scale),"value-changed",G_CALLBACK(on_value_changed),NULL);

    gtk_widget_set_size_request(window,-1,180);

    gtk_widget_realize(window);

    move_window_with_status_icon(statusicon, window);

    set_mixer_status(xmixer);
    
    return window;
}



GtkWidget *create_osd()
{
    GtkWidget *osd = msd_media_keys_window_new();
    gtk_widget_realize(osd);
    //move it to bottom
    int osd_width, osd_height;
    gtk_window_get_size(GTK_WINDOW(osd), &osd_width, &osd_height);
    GdkRectangle prim_monitor_rect;
    GdkScreen *screen = gdk_screen_get_default();
    gdk_screen_get_monitor_geometry(screen, gdk_screen_get_primary_monitor(screen), &prim_monitor_rect);
    gtk_window_move(GTK_WINDOW(osd), (prim_monitor_rect.width - osd_width)/2, prim_monitor_rect.height - osd_height - 120);
    return osd;
}

void handler_mute (const char *keystring, void *user_data) {
    GtkWidget *window = gtk_builder_get_object(ui_builder, "window");
    if(window && gtk_widget_get_visible(window)) {
        if(timer_handle) {
            g_source_remove(timer_handle);
            timer_handle = 0;
        }
        gtk_widget_hide(window);
    }

    int mute = alsa_mixer_get_master_mute(mixer);
    alsa_mixer_set_master_mute(mixer,!mute);

    ca_context_play(context, 0, CA_PROP_EVENT_ID, "device-added", NULL);

    if(!osd)
        osd = create_osd();

    gtk_widget_show(osd);
    msd_media_keys_window_set_volume_muted(osd, !mute);
    if(!mute)
        msd_media_keys_window_set_volume_level(osd, 0);
    else
        msd_media_keys_window_set_volume_level(osd, alsa_mixer_get_master_volume(mixer));

    set_mixer_status(xmixer);
    xmixer_save_config(alsa_mixer_get_master_mute(mixer), alsa_mixer_get_master_volume(mixer));
}

//this is called when press XF86AudioRecord key
void handler_toggle_capture (const char *keystring, void *user_data) {
    if(!mixer)
        return ;
    int onoff = alsa_mixer_get_capture_mute(mixer);
    alsa_mixer_set_capture_mute(mixer, !onoff);
    ca_context_play(context, 0, CA_PROP_EVENT_ID, "device-added", NULL);
    if(!rosd)
    rosd = create_osd();

    if(onoff)
        msd_media_keys_window_set_action_custom (rosd,
                         "microphone-sensitivity-muted-symbolic",
                         TRUE);
    else
        msd_media_keys_window_set_action_custom (rosd,
                         "microphone-sensitivity-medium-symbolic",
                         TRUE);
    gtk_widget_show(rosd);
}

void handler_lower (const char *keystring, void *user_data) {
    GtkWidget *window = gtk_builder_get_object(ui_builder, "window");
    if(window && gtk_widget_get_visible(window)) {
        if(timer_handle) {
            g_source_remove(timer_handle);
            timer_handle = 0;
        }
        gtk_widget_hide(window);
    }
    int newvol = alsa_mixer_get_master_volume(mixer)-10;
    if(newvol < 0)
        newvol = 0;
    alsa_mixer_set_master_volume(mixer,newvol);

    ca_context_play(context, 0, CA_PROP_EVENT_ID, "audio-volume-change", NULL);

    if(!osd)
        osd = create_osd();
    gtk_widget_show(osd);
    msd_media_keys_window_set_volume_muted(osd, alsa_mixer_get_master_mute(mixer));
    msd_media_keys_window_set_volume_level(osd, newvol);

    set_mixer_status(xmixer);
    xmixer_save_config(alsa_mixer_get_master_mute(mixer), alsa_mixer_get_master_volume(mixer));
}

void handler_raise (const char *keystring, void *user_data) {
    GtkWidget *window = gtk_builder_get_object(ui_builder, "window");
    if(window && gtk_widget_get_visible(window)) {
        if(timer_handle) {
            g_source_remove(timer_handle);
            timer_handle = 0;
        }
        gtk_widget_hide(window);
    }
    int newvol = alsa_mixer_get_master_volume(mixer)+10;
    if(newvol > 100)
        newvol = 100;
    alsa_mixer_set_master_volume(mixer,newvol);

    ca_context_play(context, 0, CA_PROP_EVENT_ID, "audio-volume-change", NULL);

    if(!osd)
        osd = create_osd();
    gtk_widget_show(osd);
    msd_media_keys_window_set_volume_muted(osd, alsa_mixer_get_master_mute(mixer));
    msd_media_keys_window_set_volume_level(osd, newvol);
    set_mixer_status(xmixer);
    xmixer_save_config(alsa_mixer_get_master_mute(mixer), alsa_mixer_get_master_volume(mixer));
}

//master channel callback
int master_event(snd_mixer_elem_t *elem, unsigned int mask)
{ 
  if (mask & SND_CTL_EVENT_MASK_VALUE) {
    int mute = alsa_mixer_get_master_mute(mixer);
    if(xmixer)
        set_mixer_status(xmixer);
  }
  return 0;
}

int capture_event(snd_mixer_elem_t *elem, unsigned int mask)
{
}


int main(int argc, char **argv)
{
    gtk_init(&argc,&argv);
    xmixer = gtk_status_icon_new();

    mixer = (Alsamixer *)malloc(sizeof(Alsamixer));
    if(!alsa_mixer_init(mixer, master_event, capture_event))
        exit(1);

    gint mute = 0;
    gint volume=70;

    xmixer_get_config(&mute,&volume);

    alsa_mixer_set_master_mute(mixer,mute);
    alsa_mixer_set_master_volume(mixer,volume);

    ui_builder = gtk_builder_new_from_file(XMIXER_DATA "/xmixer.ui");

    GtkWidget *window = create_mixer_frame(xmixer);

    g_signal_connect(G_OBJECT(xmixer),"activate",G_CALLBACK(mixer_activated), window);

    set_mixer_status(xmixer);
 

    ca_context_create(&context);
    ca_context_change_props(context,CA_PROP_CANBERRA_XDG_THEME_NAME,"freedesktop",NULL);
    ca_context_set_driver(context,"alsa");

    keybinder_init();
    keybinder_bind("XF86AudioLowerVolume", handler_lower, NULL);
    keybinder_bind("XF86AudioRaiseVolume", handler_raise, NULL);
    keybinder_bind("XF86AudioMute", handler_mute, NULL);
    keybinder_bind("XF86AudioRecord", handler_toggle_capture, NULL);


    gtk_main();
}
