#ifndef XDR_CONF_H_
#define XDR_CONF_H_
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#define PRESETS 12
#define ANT_COUNT 4
#define HOST_HISTORY_LEN 5

enum Action
{
    ACTION_NONE,
    ACTION_ACTIVATE,
    ACTION_SCREENSHOT
};

enum Signal_Unit
{
    UNIT_DBF,
    UNIT_DBM,
    UNIT_DBUV
};

enum Signal_Display
{
    SIGNAL_NONE,
    SIGNAL_GRAPH,
    SIGNAL_BAR
};

enum Signal_Mode
{
    GRAPH_DEFAULT,
    GRAPH_RESET
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
    gboolean network;
    gchar *serial;
    gchar **host;
    guint16 port;
    gchar *password;

    /* Tuner settings */
    gint volume;
    gboolean rfgain;
    gboolean ifgain;
    gint agc;
    gint deemphasis;

    /* Interface */
    gint initial_freq;
    gboolean utc;
    gboolean auto_connect;
    gboolean mw_10k_steps;
    gboolean disconnect_confirm;
    gboolean auto_reconnect;
    enum Action event_action;
    gboolean hide_decorations;
    gboolean hide_interference;
    gboolean hide_radiotext;
    gboolean hide_statusbar;
    gboolean restore_position;
    gboolean grab_focus;

    /* Graph */
    gdouble signal_offset;
    enum Signal_Unit signal_unit;
    enum Signal_Display signal_display;
    enum Signal_Mode signal_mode;
    gint signal_height;
    gboolean signal_grid;
    gboolean signal_avg;
    GdkColor color_mono;
    GdkColor color_stereo;
    GdkColor color_rds;

    /* RDS */
    enum RDS_Mode rds_pty_set;
    gboolean rds_reset;
    gint rds_reset_timeout;
    enum RDS_Err_Correction rds_ps_info_error;
    enum RDS_Err_Correction rds_ps_data_error;
    gboolean rds_ps_progressive;
    enum RDS_Err_Correction rds_rt_info_error;
    enum RDS_Err_Correction rds_rt_data_error;

    /* Antenna */
    gboolean ant_show_alignment;
    gboolean ant_swap_rotator;
    gint ant_count;
    gboolean ant_clear_rds;
    gboolean ant_auto_switch;
    gint ant_start[ANT_COUNT];
    gint ant_stop[ANT_COUNT];
    gint ant_offset[ANT_COUNT];

    /* Logs */
    gint rdsspy_port;
    gboolean rdsspy_auto;
    gboolean rdsspy_run;
    gchar *rdsspy_exec;
    gboolean srcp;
    gint srcp_port;
    gboolean rds_logging;
    gboolean replace_spaces;
    gchar *log_dir;
    gchar *screen_dir;

    /* Keyboard */
    guint key_tune_up;
    guint key_tune_down;
    guint key_tune_fine_up;
    guint key_tune_fine_down;
    guint key_tune_jump_up;
    guint key_tune_jump_down;
    guint key_tune_back;
    guint key_tune_reset;
    guint key_screenshot;
    guint key_bw_up;
    guint key_bw_down;
    guint key_bw_auto;
    guint key_rotate_cw;
    guint key_rotate_ccw;
    guint key_switch_antenna;
    guint key_rds_ps_mode;
    guint key_scan_toggle;
    guint key_scan_prev;
    guint key_scan_next;

    /* Presets */
    gint presets[PRESETS];

    /* Scheduler */
    gsize scheduler_n;
    gint *scheduler_freqs;
    gint *scheduler_timeouts;
    gint scheduler_default_timeout;

    /* Pattern */
    gint pattern_color;
    gint pattern_size;
    gboolean pattern_inv;
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
    gboolean scan_mark_tuned;
    gboolean scan_update;
    GList *scan_marks;
} settings_t;

settings_t conf;

void conf_init(const gchar*);
void conf_write();

void conf_uniq_int_list_add(GList**, gint);
void conf_uniq_int_list_toggle(GList**, gint);
void conf_uniq_int_list_remove(GList**, gint);
void conf_uniq_int_list_clear_range(GList**, gint, gint);
void conf_uniq_int_list_clear(GList**);

void conf_update_string_const(gchar**, const gchar*);
void conf_update_string(gchar**, gchar*);

void conf_add_host(const gchar*);

#endif
