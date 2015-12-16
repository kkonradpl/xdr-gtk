#ifndef XDR_SETTINGS_H_
#define XDR_SETTINGS_H_
#include <gtk/gtk.h>

#ifdef __APPLE__
#define CONF_FILE "/Library/Preferences/xdr-gtk.conf"
#else
#define CONF_FILE "xdr-gtk.conf"
#endif

#define PRESETS 12
#define ANT_COUNT 4
#define HOST_HISTORY_LEN 5

enum Unit
{
    UNIT_DBF,
    UNIT_DBM,
    UNIT_DBUV,
    UNIT_S
};

enum Action
{
    ACTION_NONE,
    ACTION_ACTIVATE,
    ACTION_SCREENSHOT
};

enum Signal
{
    SIGNAL_NONE,
    SIGNAL_GRAPH,
    SIGNAL_BAR
};

enum Graph_Mode
{
    GRAPH_DEFAULT,
    GRAPH_RESET,
    GRAPH_SEPARATOR
};

enum RDS_Mode
{
    RDS,
    RBDS
};

enum RDS_Err_Correction
{
    NO_ERR_CORR,
    UP_TO_2_BIT_ERR_CORR,
    UP_TO_5_BIT_ERR_CORR
};

typedef struct settings
{
    /* Window */
    gint win_x;
    gint win_y;

    /* Connection */
    gint network;
    gchar* serial;
    gchar** host;
    unsigned short port;
    gchar* password;

    /* Tuner settings */
    gboolean rfgain;
    gboolean ifgain;
    gint agc;
    gint deemphasis;

    /* Interface */
    gint init_freq;
    gdouble signal_offset;
    enum Unit signal_unit;
    gboolean utc;
    gboolean autoconnect;
    gboolean amstep;
    gboolean disconnect_confirm;
    gboolean auto_reconnect;
    gboolean grab_focus;
    enum Action event_action;
    gboolean hide_decorations;
    gboolean hide_status;
    gboolean restore_pos;

    /* Graph */
    enum Signal signal_display;
    enum Graph_Mode graph_mode;
    GdkColor color_mono;
    GdkColor color_stereo;
    GdkColor color_rds;
    gint graph_height;
    gboolean show_grid;
    gboolean signal_avg;

    /* RDS */
    enum RDS_Mode rds_pty;
    gboolean rds_reset;
    gint rds_reset_timeout;
    enum RDS_Err_Correction ps_info_error;
    enum RDS_Err_Correction ps_data_error;
    gboolean rds_ps_progressive;
    enum RDS_Err_Correction rt_info_error;
    enum RDS_Err_Correction rt_data_error;

    /* Antenna */
    gboolean alignment;
    gboolean swap_rotator;
    gint ant_count;
    gboolean ant_switching;
    gint ant_start[ANT_COUNT];
    gint ant_stop[ANT_COUNT];
    gboolean ant_clear_rds;

    /* Logs */
    gint rds_spy_port;
    gboolean rds_spy_auto;
    gboolean rds_spy_run;
    gchar* rds_spy_command;
    gboolean stationlist;
    gint stationlist_port;
    gboolean rds_logging;
    gboolean replace_spaces;
    gchar* log_dir;
    gchar* screen_dir;

    /* Keyboard */
    guint key_tune_up;
    guint key_tune_down;
    guint key_tune_up_5;
    guint key_tune_down_5;
    guint key_tune_up_1000;
    guint key_tune_down_1000;
    guint key_tune_back;
    guint key_reset;
    guint key_screen;
    guint key_bw_up;
    guint key_bw_down;
    guint key_bw_auto;
    guint key_rotate_cw;
    guint key_rotate_ccw;
    guint key_switch_ant;
    guint key_ps_mode;
    guint key_spectral_toggle;

    /* Presets */
    gint presets[PRESETS];

    /* Scheduler */
    gint scheduler_n;
    gint *scheduler_freqs;
    gint *scheduler_timeouts;
    gint scheduler_default_timeout;

    /* Pattern */
    gint pattern_size;
    gboolean pattern_fill;
    gboolean pattern_avg;

    /* Spectral scan */
    gint scan_x;
    gint scan_y;
    gint scan_width;
    gint scan_height;
    gint scan_start;
    gint scan_end;
    gint scan_step;
    gint scan_bw;
    gboolean scan_continuous;
    gboolean scan_relative;
    gboolean scan_peakhold;
    GList *scan_freq_list;
} settings_t;

settings_t conf;

void settings_read();
void settings_write();
void settings_dialog();
void settings_key(GtkWidget*, GdkEventButton*, gpointer);
gboolean settings_key_press(GtkWidget*, GdkEventKey*, gpointer);
void settings_update_string(gchar**, const gchar*);
gchar* settings_read_string(gchar*);
void settings_add_host(const gchar* host);

void settings_scheduler_load();
void settings_scheduler_store();
gboolean settings_scheduler_store_foreach(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer*);

void settings_scheduler_add(GtkWidget*, gpointer);
void settings_scheduler_edit(GtkCellRendererText*, gchar*, gchar*, gpointer);
void settings_scheduler_remove(GtkWidget*, gpointer);
gboolean settings_scheduler_key(GtkWidget*, GdkEventKey*, gpointer);
gboolean settings_scheduler_add_key(GtkWidget*, GdkEventKey*, gpointer);
gboolean settings_scheduler_add_key_idle(gpointer);

void settings_scan_mark_add(gint);
void settings_scan_mark_toggle(gint);
void settings_scan_mark_remove(gint);
void settings_scan_mark_clear(gint, gint);
void settings_scan_mark_clear_all();

#endif
