#include <glib/gstdio.h>
#include "ui.h"
#include "conf.h"
#include "conf-defaults.h"

#define CONF_DIR "xdr-gtk"
#define CONF_FILE "xdr-gtk.conf"

static const gchar *group_window     = "window";
static const gchar *group_connection = "connection";
static const gchar *group_tuner      = "tuner";
static const gchar *group_interface  = "interface";
static const gchar *group_signal     = "signal";
static const gchar *group_rds        = "rds";
static const gchar *group_antenna    = "antenna";
static const gchar *group_logs       = "logs";
static const gchar *group_keyboard   = "keyboard";
static const gchar *group_presets    = "presets";
static const gchar *group_scheduler  = "scheduler";
static const gchar *group_pattern    = "pattern";
static const gchar *group_scan       = "scan";

static const gchar *key_x                  = "x";
static const gchar *key_y                  = "y";
static const gchar *key_network            = "network";
static const gchar *key_serial             = "serial";
static const gchar *key_host               = "host";
static const gchar *key_port               = "port";
static const gchar *key_password           = "password";
static const gchar *key_volume             = "volume";
static const gchar *key_rfgain             = "rfgain";
static const gchar *key_ifgain             = "ifgain";
static const gchar *key_agc                = "agc";
static const gchar *key_deemphasis         = "deemphasis";
static const gchar *key_initial_freq       = "initial_freq";
static const gchar *key_event_action       = "event_action";
static const gchar *key_utc                = "utc";
static const gchar *key_mw_10k_steps       = "mw_10k_steps";
static const gchar *key_auto_connect       = "auto_connect";
static const gchar *key_disconnect_confirm = "disconnect_confirm";
static const gchar *key_auto_reconnect     = "auto_reconnect";
static const gchar *key_hide_decorations   = "hide_decorations";
static const gchar *key_hide_interference  = "hide_interference";
static const gchar *key_hide_statusbar     = "hide_statusbar";
static const gchar *key_restore_position   = "restore_position";
static const gchar *key_unit               = "unit";
static const gchar *key_display            = "display";
static const gchar *key_mode               = "mode";
static const gchar *key_height             = "height";
static const gchar *key_offset             = "offset";
static const gchar *key_grid               = "grid";
static const gchar *key_avg                = "avg";
static const gchar *key_color_mono         = "color_mono";
static const gchar *key_color_stereo       = "color_stereo";
static const gchar *key_color_rds          = "color_rds";
static const gchar *key_pty_set            = "pty_set";
static const gchar *key_reset              = "reset";
static const gchar *key_reset_timeout      = "reset_timeout";
static const gchar *key_ps_info_error      = "ps_info_error";
static const gchar *key_ps_data_error      = "ps_data_error";
static const gchar *key_ps_progressive     = "ps_progressive";
static const gchar *key_rt_info_error      = "rt_info_error";
static const gchar *key_rt_data_error      = "rt_data_error";
static const gchar *key_show_alignment     = "show_alignment";
static const gchar *key_swap_rotator       = "swap_rotator";
static const gchar *key_count              = "count";
static const gchar *key_clear_rds          = "clear_rds";
static const gchar *key_auto_switch        = "auto_switch";
static const gchar *key_ant_start          = "ant_start";
static const gchar *key_ant_stop           = "ant_stop";
static const gchar *key_rdsspy_port        = "rdsspy_port";
static const gchar *key_rdsspy_auto        = "rdsspy_auto";
static const gchar *key_rdsspy_run         = "rdsspy_run";
static const gchar *key_rdsspy_exec        = "rdsspy_exec";
static const gchar *key_srcp               = "srcp";
static const gchar *key_srcp_port          = "srcp_port";
static const gchar *key_rds_logging        = "rds_logging";
static const gchar *key_replace_spaces     = "replace_spaces";
static const gchar *key_log_dir            = "log_dir";
static const gchar *key_screen_dir         = "screen_dir";
static const gchar *key_tune_up            = "tune_up";
static const gchar *key_tune_down          = "tune_down";
static const gchar *key_tune_fine_up       = "tune_fine_up";
static const gchar *key_tune_fine_down     = "tune_fine_down";
static const gchar *key_tune_jump_up       = "tune_jump_up";
static const gchar *key_tune_jump_down     = "tune_jump_down";
static const gchar *key_tune_back          = "tune_back";
static const gchar *key_tune_reset         = "tune_reset";
static const gchar *key_screenshot         = "screenshot";
static const gchar *key_bw_up              = "bw_up";
static const gchar *key_bw_down            = "bw_down";
static const gchar *key_bw_auto            = "bw_auto";
static const gchar *key_rotate_cw          = "rotate_cw";
static const gchar *key_rotate_ccw         = "rotate_ccw";
static const gchar *key_switch_antenna     = "switch_antenna";
static const gchar *key_rds_ps_mode        = "rds_ps_mode";
static const gchar *key_scan_toggle        = "scan_toggle";
static const gchar *key_presets            = "presets";
static const gchar *key_freqs              = "freqs";
static const gchar *key_timeouts           = "timeouts";
static const gchar *key_default_timeout    = "default_timeout";
static const gchar *key_size               = "size";
static const gchar *key_fill               = "fill";
static const gchar *key_width              = "width";
static const gchar *key_start              = "start";
static const gchar *key_end                = "end";
static const gchar *key_step               = "step";
static const gchar *key_bw                 = "bw";
static const gchar *key_continuous         = "continuous";
static const gchar *key_relative           = "relative";
static const gchar *key_peak_hold          = "peak_hold";
static const gchar *key_mark_tuned         = "mark_tuned";
static const gchar *key_marks              = "marks";

static const gint default_presets[PRESETS] =
{
    CONF_PRESET_1,
    CONF_PRESET_2,
    CONF_PRESET_3,
    CONF_PRESET_4,
    CONF_PRESET_5,
    CONF_PRESET_6,
    CONF_PRESET_7,
    CONF_PRESET_8,
    CONF_PRESET_9,
    CONF_PRESET_10,
    CONF_PRESET_11,
    CONF_PRESET_12
};

static gchar *path;
static GKeyFile *keyfile;

static void     conf_read();
static gboolean conf_read_boolean(GKeyFile*, const gchar*, const gchar*, gboolean);
static gint     conf_read_integer(GKeyFile*, const gchar*, const gchar*, gint);
static gdouble  conf_read_double(GKeyFile*, const gchar*, const gchar*, gdouble);
static gchar*   conf_read_string(GKeyFile*, const gchar*, const gchar*, const gchar*);
static void     conf_read_color(GKeyFile*, const gchar*, const gchar*, GdkColor*, const gchar*);
static void     conf_save_color(GKeyFile*, const gchar*, const gchar*, const GdkColor*);
static gchar**  conf_read_hosts(GKeyFile*, const gchar*, const gchar*, const gchar*);
static void     conf_read_integers(GKeyFile*, const gchar*, const gchar*, gint, const gint*, gint*);
static GList*   conf_uniq_int_list_read(GKeyFile*, const gchar*, const gchar*);
static void     conf_uniq_int_list_save(GKeyFile*, const gchar*, const gchar*, GList*);


void
conf_init(const gchar *custom_path)
{
    gchar *directory;
    if(custom_path)
        path = g_strdup(custom_path);
    else
    {
        directory = g_build_filename(g_get_user_config_dir(), CONF_DIR, NULL);
        g_mkdir(directory, 0700);
        path = g_build_filename(directory, CONF_FILE, NULL);
        g_free(directory);
    }

    keyfile = g_key_file_new();
    conf_read();
}

static void
conf_read()
{
    gboolean file_exists = TRUE;
    gsize length;

    if(!g_key_file_load_from_file(keyfile, path, G_KEY_FILE_KEEP_COMMENTS, NULL))
    {
        file_exists = FALSE;
        ui_dialog(NULL,
                  GTK_MESSAGE_INFO,
                  "Configuration",
                  "Unable to read the configuration.\n\nUsing default settings.");
    }

    /* Window */
    conf.win_x = conf_read_integer(keyfile, group_window, key_x, CONF_WINDOW_X);
    conf.win_y = conf_read_integer(keyfile, group_window, key_y, CONF_WINDOW_Y);

    /* Connection */
    conf.network  = conf_read_boolean(keyfile, group_connection, key_network,  CONF_CONNECTION_NETWORK);
    conf.serial   = conf_read_string (keyfile, group_connection, key_serial,   CONF_CONNECTION_SERIAL);
    conf.host     = conf_read_hosts  (keyfile, group_connection, key_host,     CONF_CONNECTION_HOST);
    conf.port     = conf_read_integer(keyfile, group_connection, key_port,     CONF_CONNECTION_PORT);
    conf.password = conf_read_string (keyfile, group_connection, key_password, CONF_CONNECTION_PASSWORD);

    /* Tuner settings */
    conf.volume     = conf_read_integer(keyfile, group_tuner, key_volume,     CONF_TUNER_VOLUME);
    conf.rfgain     = conf_read_boolean(keyfile, group_tuner, key_rfgain,     CONF_TUNER_RFGAIN);
    conf.ifgain     = conf_read_boolean(keyfile, group_tuner, key_ifgain,     CONF_TUNER_IFGAIN);
    conf.agc        = conf_read_integer(keyfile, group_tuner, key_agc,        CONF_TUNER_AGC);
    conf.deemphasis = conf_read_integer(keyfile, group_tuner, key_deemphasis, CONF_TUNER_DEEMPHASIS);

    /* Interface */
    conf.initial_freq       = conf_read_integer(keyfile, group_interface, key_initial_freq,       CONF_INTERFACE_INITIAL_FREQ);
    conf.event_action       = conf_read_integer(keyfile, group_interface, key_event_action,       CONF_INTERFACE_EVENT_ACTION);
    conf.utc                = conf_read_boolean(keyfile, group_interface, key_utc,                CONF_INTERFACE_UTC);
    conf.mw_10k_steps       = conf_read_boolean(keyfile, group_interface, key_mw_10k_steps,       CONF_INTERFACE_MW_10K_STEPS);
    conf.auto_connect       = conf_read_boolean(keyfile, group_interface, key_auto_connect,       CONF_INTERFACE_AUTO_CONNECT);
    conf.disconnect_confirm = conf_read_boolean(keyfile, group_interface, key_disconnect_confirm, CONF_INTERFACE_DISCONNECT_CONFIRM);
    conf.auto_reconnect     = conf_read_boolean(keyfile, group_interface, key_auto_reconnect,     CONF_INTERFACE_AUTO_RECONNECT);
    conf.hide_decorations   = conf_read_boolean(keyfile, group_interface, key_hide_decorations,   CONF_INTERFACE_HIDE_DECORATIONS);
    conf.hide_interference  = conf_read_boolean(keyfile, group_interface, key_hide_interference,  CONF_INTERFACE_HIDE_INTERFERENCE);
    conf.hide_statusbar     = conf_read_boolean(keyfile, group_interface, key_hide_statusbar,     CONF_INTERFACE_HIDE_STATUSBAR);
    conf.restore_position   = conf_read_boolean(keyfile, group_interface, key_restore_position,   CONF_INTERFACE_RESTORE_POSITION);

    /* Graph */
    conf.signal_unit    = conf_read_integer(keyfile, group_signal, key_unit,     CONF_SIGNAL_UNIT);
    conf.signal_display = conf_read_integer(keyfile, group_signal, key_display,  CONF_SIGNAL_DISPLAY);
    conf.signal_mode    = conf_read_integer(keyfile, group_signal, key_mode,     CONF_SIGNAL_MODE);
    conf.signal_height  = conf_read_integer(keyfile, group_signal, key_height,   CONF_SIGNAL_HEIGHT);
    conf.signal_offset  = conf_read_double (keyfile, group_signal, key_offset,   CONF_SIGNAL_OFFSET);
    conf.signal_grid    = conf_read_boolean(keyfile, group_signal, key_grid,     CONF_SIGNAL_GRID);
    conf.signal_avg     = conf_read_boolean(keyfile, group_signal, key_avg,      CONF_SIGNAL_AVG);
    conf_read_color(keyfile, group_signal, key_color_mono,   &conf.color_mono,   CONF_SIGNAL_COLOR_MONO);
    conf_read_color(keyfile, group_signal, key_color_stereo, &conf.color_stereo, CONF_SIGNAL_COLOR_STEREO);
    conf_read_color(keyfile, group_signal, key_color_rds,    &conf.color_rds,    CONF_SIGNAL_COLOR_RDS);

    /* RDS */
    conf.rds_pty_set        = conf_read_integer(keyfile, group_rds, key_pty_set,        CONF_RDS_PTY_SET);
    conf.rds_reset          = conf_read_boolean(keyfile, group_rds, key_reset,          CONF_RDS_RESET);
    conf.rds_reset_timeout  = conf_read_integer(keyfile, group_rds, key_reset_timeout,  CONF_RDS_RESET_TIMEOUT);
    conf.rds_ps_info_error  = conf_read_integer(keyfile, group_rds, key_ps_info_error,  CONF_RDS_PS_INFO_ERROR);
    conf.rds_ps_data_error  = conf_read_integer(keyfile, group_rds, key_ps_data_error,  CONF_RDS_PS_DATA_ERROR);
    conf.rds_ps_progressive = conf_read_boolean(keyfile, group_rds, key_ps_progressive, CONF_RDS_PS_PROGRESSIVE);
    conf.rds_rt_info_error  = conf_read_integer(keyfile, group_rds, key_rt_info_error,  CONF_RDS_RT_INFO_ERROR);
    conf.rds_rt_data_error  = conf_read_integer(keyfile, group_rds, key_rt_data_error,  CONF_RDS_RT_DATA_ERROR);

    /* Antenna */
    conf.ant_show_alignment = conf_read_boolean(keyfile, group_antenna, key_show_alignment, CONF_ANTENNA_SHOW_ALIGNMENT);
    conf.ant_swap_rotator   = conf_read_boolean(keyfile, group_antenna, key_swap_rotator,   CONF_ANTENNA_SWAP_ROTATOR);
    conf.ant_count          = conf_read_integer(keyfile, group_antenna, key_count,          CONF_ANTENNA_COUNT);
    conf.ant_clear_rds      = conf_read_boolean(keyfile, group_antenna, key_clear_rds,      CONF_ANTENNA_CLEAR_RDS);
    conf.ant_auto_switch    = conf_read_boolean(keyfile, group_antenna, key_auto_switch,    CONF_ANTENNA_AUTO_SWITCH);
    conf_read_integers(keyfile, group_antenna, key_ant_start, ANT_COUNT, NULL, conf.ant_start);
    conf_read_integers(keyfile, group_antenna, key_ant_stop, ANT_COUNT, NULL, conf.ant_stop);

    /* Logs */
    conf.rdsspy_port    = conf_read_integer(keyfile, group_logs, key_rdsspy_port,    CONF_LOGS_RDSSPY_PORT);
    conf.rdsspy_auto    = conf_read_boolean(keyfile, group_logs, key_rdsspy_auto,    CONF_LOGS_RDSSPY_AUTO);
    conf.rdsspy_run     = conf_read_boolean(keyfile, group_logs, key_rdsspy_run,     CONF_LOGS_RDSSPY_RUN);
    conf.rdsspy_exec    = conf_read_string (keyfile, group_logs, key_rdsspy_exec,    CONF_LOGS_RDSSPY_EXEC);
    conf.srcp           = conf_read_boolean(keyfile, group_logs, key_srcp,           CONF_LOGS_SRCP);
    conf.srcp_port      = conf_read_integer(keyfile, group_logs, key_srcp_port,      CONF_LOGS_SRCP_PORT);
    conf.rds_logging    = conf_read_boolean(keyfile, group_logs, key_rds_logging,    CONF_LOGS_RDS_LOGGING);
    conf.replace_spaces = conf_read_boolean(keyfile, group_logs, key_replace_spaces, CONF_LOGS_REPLACE_SPACES);
    conf.log_dir        = conf_read_string (keyfile, group_logs, key_log_dir,        CONF_LOGS_LOG_DIR);
    conf.screen_dir     = conf_read_string (keyfile, group_logs, key_screen_dir,     CONF_LOGS_SCREEN_DIR);

    /* Keyboard */
    conf.key_tune_up        = conf_read_integer(keyfile, group_keyboard, key_tune_up,        CONF_KEY_TUNE_UP);
    conf.key_tune_down      = conf_read_integer(keyfile, group_keyboard, key_tune_down,      CONF_KEY_TUNE_DOWN);
    conf.key_tune_fine_up   = conf_read_integer(keyfile, group_keyboard, key_tune_fine_up,   CONF_KEY_FINE_TUNE_UP);
    conf.key_tune_fine_down = conf_read_integer(keyfile, group_keyboard, key_tune_fine_down, CONF_KEY_FINE_TUNE_DOWN);
    conf.key_tune_jump_up   = conf_read_integer(keyfile, group_keyboard, key_tune_jump_up,   CONF_KEY_JUMP_TUNE_UP);
    conf.key_tune_jump_down = conf_read_integer(keyfile, group_keyboard, key_tune_jump_down, CONF_KEY_JUMP_TUNE_DOWN);
    conf.key_tune_back      = conf_read_integer(keyfile, group_keyboard, key_tune_back,      CONF_KEY_TUNE_BACK);
    conf.key_tune_reset     = conf_read_integer(keyfile, group_keyboard, key_tune_reset,     CONF_KEY_TUNE_RESET);
    conf.key_screenshot     = conf_read_integer(keyfile, group_keyboard, key_screenshot,     CONF_KEY_SCREENSHOT);
    conf.key_bw_up          = conf_read_integer(keyfile, group_keyboard, key_bw_up,          CONF_KEY_BW_UP);
    conf.key_bw_down        = conf_read_integer(keyfile, group_keyboard, key_bw_down,        CONF_KEY_BW_DOWN);
    conf.key_bw_auto        = conf_read_integer(keyfile, group_keyboard, key_bw_auto,        CONF_KEY_BW_AUTO);
    conf.key_rotate_cw      = conf_read_integer(keyfile, group_keyboard, key_rotate_cw,      CONF_KEY_ROTATE_CW);
    conf.key_rotate_ccw     = conf_read_integer(keyfile, group_keyboard, key_rotate_ccw,     CONF_KEY_ROTATE_CCW);
    conf.key_switch_antenna = conf_read_integer(keyfile, group_keyboard, key_switch_antenna, CONF_KEY_SWITCH_ANTENNA);
    conf.key_rds_ps_mode    = conf_read_integer(keyfile, group_keyboard, key_rds_ps_mode,    CONF_KEY_RDS_PS_MODE);
    conf.key_scan_toggle    = conf_read_integer(keyfile, group_keyboard, key_scan_toggle,    CONF_KEY_SCAN_TOGGLE);

    /* Presets */
    conf_read_integers(keyfile, group_presets, key_presets, PRESETS, default_presets, conf.presets);

    /* Scheduler */
    conf.scheduler_freqs = g_key_file_get_integer_list(keyfile, group_scheduler, key_freqs, &conf.scheduler_n, NULL);
    conf.scheduler_timeouts = g_key_file_get_integer_list(keyfile, group_scheduler, key_timeouts, &length, NULL);
    conf.scheduler_n = (conf.scheduler_n > length) ? length : conf.scheduler_n;
    conf.scheduler_default_timeout = conf_read_integer(keyfile, group_scheduler, key_default_timeout, CONF_SCHEDULER_TIMEOUT);

    /* Pattern */
    conf.pattern_size = conf_read_integer(keyfile, group_pattern, key_size, CONF_PATTERN_SIZE);
    conf.pattern_fill = conf_read_boolean(keyfile, group_pattern, key_fill, CONF_PATTERN_FILL);
    conf.pattern_avg  = conf_read_boolean(keyfile, group_pattern, key_avg,  CONF_PATTERN_AVG);

    /* Spectral scan */
    conf.scan_x          = conf_read_integer(keyfile, group_scan, key_x,          CONF_SCAN_X);
    conf.scan_y          = conf_read_integer(keyfile, group_scan, key_y,          CONF_SCAN_Y);
    conf.scan_width      = conf_read_integer(keyfile, group_scan, key_width,      CONF_SCAN_WIDTH);
    conf.scan_height     = conf_read_integer(keyfile, group_scan, key_height,     CONF_SCAN_HEIGHT);
    conf.scan_start      = conf_read_integer(keyfile, group_scan, key_start,      CONF_SCAN_START);
    conf.scan_end        = conf_read_integer(keyfile, group_scan, key_end,        CONF_SCAN_END);
    conf.scan_step       = conf_read_integer(keyfile, group_scan, key_step,       CONF_SCAN_STEP);
    conf.scan_bw         = conf_read_integer(keyfile, group_scan, key_bw,         CONF_SCAN_BW);
    conf.scan_continuous = conf_read_boolean(keyfile, group_scan, key_continuous, CONF_SCAN_CONTINOUS);
    conf.scan_relative   = conf_read_boolean(keyfile, group_scan, key_relative,   CONF_SCAN_RELATIVE);
    conf.scan_peakhold   = conf_read_boolean(keyfile, group_scan, key_peak_hold,  CONF_SCAN_PEAKHOLD);
    conf.scan_mark_tuned = conf_read_boolean(keyfile, group_scan, key_mark_tuned, CONF_SCAN_MARK_TUNED);
    conf.scan_marks      = conf_uniq_int_list_read(keyfile, group_scan, key_marks);

    if(!file_exists)
        conf_write();
}

void
conf_write()
{
    GError *err = NULL;
    gchar *configuration;
    gsize length, out;
    FILE *fp;

    /* Window */
    g_key_file_set_integer(keyfile, group_window, key_x, conf.win_x);
    g_key_file_set_integer(keyfile, group_window, key_y, conf.win_y);

    /* Connection */
    g_key_file_set_boolean    (keyfile, group_connection, key_network,  conf.network);
    g_key_file_set_string     (keyfile, group_connection, key_serial,   conf.serial);
    g_key_file_set_string_list(keyfile, group_connection, key_host,     (const gchar* const*)conf.host, g_strv_length(conf.host));
    g_key_file_set_integer    (keyfile, group_connection, key_port,     conf.port);
    g_key_file_set_string     (keyfile, group_connection, key_password, conf.password);

    /* Tuner settings */
    g_key_file_set_integer(keyfile, group_tuner, key_volume,     conf.volume);
    g_key_file_set_boolean(keyfile, group_tuner, key_rfgain,     conf.rfgain);
    g_key_file_set_boolean(keyfile, group_tuner, key_ifgain,     conf.ifgain);
    g_key_file_set_integer(keyfile, group_tuner, key_agc,        conf.agc);
    g_key_file_set_integer(keyfile, group_tuner, key_deemphasis, conf.deemphasis);

    /* Interface */
    g_key_file_set_integer(keyfile, group_interface, key_initial_freq,       conf.initial_freq);
    g_key_file_set_integer(keyfile, group_interface, key_event_action,       conf.event_action);
    g_key_file_set_boolean(keyfile, group_interface, key_utc,                conf.utc);
    g_key_file_set_boolean(keyfile, group_interface, key_mw_10k_steps,       conf.mw_10k_steps);
    g_key_file_set_boolean(keyfile, group_interface, key_auto_connect,       conf.auto_connect);
    g_key_file_set_boolean(keyfile, group_interface, key_disconnect_confirm, conf.disconnect_confirm);
    g_key_file_set_boolean(keyfile, group_interface, key_auto_reconnect,     conf.auto_reconnect);
    g_key_file_set_boolean(keyfile, group_interface, key_hide_decorations,   conf.hide_decorations);
    g_key_file_set_boolean(keyfile, group_interface, key_hide_interference,  conf.hide_interference);
    g_key_file_set_boolean(keyfile, group_interface, key_hide_statusbar,     conf.hide_statusbar);
    g_key_file_set_boolean(keyfile, group_interface, key_restore_position,   conf.restore_position);

    /* Signal */
    g_key_file_set_integer(keyfile, group_signal, key_unit,    conf.signal_unit);
    g_key_file_set_integer(keyfile, group_signal, key_display, conf.signal_display);
    g_key_file_set_integer(keyfile, group_signal, key_mode,    conf.signal_mode);
    g_key_file_set_integer(keyfile, group_signal, key_height,  conf.signal_height);
    g_key_file_set_double (keyfile, group_signal, key_offset,  conf.signal_offset);
    g_key_file_set_boolean(keyfile, group_signal, key_grid,    conf.signal_grid);
    g_key_file_set_boolean(keyfile, group_signal, key_avg,     conf.signal_avg);
    conf_save_color(keyfile, group_signal, key_color_mono,    &conf.color_mono);
    conf_save_color(keyfile, group_signal, key_color_stereo,  &conf.color_stereo);
    conf_save_color(keyfile, group_signal, key_color_rds,     &conf.color_rds);

    /* RDS */
    g_key_file_set_integer(keyfile, group_rds, key_pty_set,        conf.rds_pty_set);
    g_key_file_set_boolean(keyfile, group_rds, key_reset,          conf.rds_reset);
    g_key_file_set_integer(keyfile, group_rds, key_reset_timeout,  conf.rds_reset_timeout);
    g_key_file_set_integer(keyfile, group_rds, key_ps_info_error,  conf.rds_ps_info_error);
    g_key_file_set_integer(keyfile, group_rds, key_ps_data_error,  conf.rds_ps_data_error);
    g_key_file_set_boolean(keyfile, group_rds, key_ps_progressive, conf.rds_ps_progressive);
    g_key_file_set_integer(keyfile, group_rds, key_rt_info_error,  conf.rds_rt_info_error);
    g_key_file_set_integer(keyfile, group_rds, key_rt_data_error,  conf.rds_rt_data_error);

    /* Antenna */
    g_key_file_set_boolean     (keyfile, group_antenna, key_show_alignment, conf.ant_show_alignment);
    g_key_file_set_boolean     (keyfile, group_antenna, key_swap_rotator,   conf.ant_swap_rotator);
    g_key_file_set_integer     (keyfile, group_antenna, key_count,          conf.ant_count);
    g_key_file_set_boolean     (keyfile, group_antenna, key_clear_rds,      conf.ant_clear_rds);
    g_key_file_set_boolean     (keyfile, group_antenna, key_auto_switch,    conf.ant_auto_switch);
    g_key_file_set_integer_list(keyfile, group_antenna, key_ant_start,      conf.ant_start, ANT_COUNT);
    g_key_file_set_integer_list(keyfile, group_antenna, key_ant_stop,       conf.ant_stop, ANT_COUNT);

    /* Logs */
    g_key_file_set_integer(keyfile, group_logs, key_rdsspy_port,    conf.rdsspy_port);
    g_key_file_set_boolean(keyfile, group_logs, key_rdsspy_auto,    conf.rdsspy_auto);
    g_key_file_set_boolean(keyfile, group_logs, key_rdsspy_run,     conf.rdsspy_run);
    g_key_file_set_string (keyfile, group_logs, key_rdsspy_exec,    conf.rdsspy_exec);
    g_key_file_set_boolean(keyfile, group_logs, key_srcp,           conf.srcp);
    g_key_file_set_integer(keyfile, group_logs, key_srcp_port,      conf.srcp_port);
    g_key_file_set_boolean(keyfile, group_logs, key_rds_logging,    conf.rds_logging);
    g_key_file_set_boolean(keyfile, group_logs, key_replace_spaces, conf.replace_spaces);
    g_key_file_set_string (keyfile, group_logs, key_log_dir,        conf.log_dir);
    g_key_file_set_string (keyfile, group_logs, key_screen_dir,     conf.screen_dir);

    /* Keyboard */
    g_key_file_set_integer(keyfile, group_keyboard, key_tune_up,        conf.key_tune_up);
    g_key_file_set_integer(keyfile, group_keyboard, key_tune_down,      conf.key_tune_down);
    g_key_file_set_integer(keyfile, group_keyboard, key_tune_fine_up,   conf.key_tune_fine_up);
    g_key_file_set_integer(keyfile, group_keyboard, key_tune_fine_down, conf.key_tune_fine_down);
    g_key_file_set_integer(keyfile, group_keyboard, key_tune_jump_up,   conf.key_tune_jump_up);
    g_key_file_set_integer(keyfile, group_keyboard, key_tune_jump_down, conf.key_tune_jump_down);
    g_key_file_set_integer(keyfile, group_keyboard, key_tune_back,      conf.key_tune_back);
    g_key_file_set_integer(keyfile, group_keyboard, key_tune_reset,     conf.key_tune_reset);
    g_key_file_set_integer(keyfile, group_keyboard, key_screenshot,     conf.key_screenshot);
    g_key_file_set_integer(keyfile, group_keyboard, key_bw_up,          conf.key_bw_up);
    g_key_file_set_integer(keyfile, group_keyboard, key_bw_down,        conf.key_bw_down);
    g_key_file_set_integer(keyfile, group_keyboard, key_bw_auto,        conf.key_bw_auto);
    g_key_file_set_integer(keyfile, group_keyboard, key_rotate_cw,      conf.key_rotate_cw);
    g_key_file_set_integer(keyfile, group_keyboard, key_rotate_ccw,     conf.key_rotate_ccw);
    g_key_file_set_integer(keyfile, group_keyboard, key_switch_antenna, conf.key_switch_antenna);
    g_key_file_set_integer(keyfile, group_keyboard, key_rds_ps_mode,    conf.key_rds_ps_mode);
    g_key_file_set_integer(keyfile, group_keyboard, key_scan_toggle,    conf.key_scan_toggle);

    /* Presets */
    g_key_file_set_integer_list(keyfile, group_presets, key_presets, conf.presets, PRESETS);

    /* Scheduler */
    if(conf.scheduler_n)
    {
        g_key_file_set_integer_list(keyfile, group_scheduler, key_freqs, conf.scheduler_freqs, conf.scheduler_n);
        g_key_file_set_integer_list(keyfile, group_scheduler, key_timeouts, conf.scheduler_timeouts, conf.scheduler_n);
    }
    else
    {
        g_key_file_remove_key(keyfile, group_scheduler, key_freqs, NULL);
        g_key_file_remove_key(keyfile, group_scheduler, key_timeouts, NULL);
    }
    g_key_file_set_integer(keyfile, group_scheduler, key_default_timeout, conf.scheduler_default_timeout);

    /* Pattern */
    g_key_file_set_integer(keyfile, group_pattern, key_size, conf.pattern_size);
    g_key_file_set_boolean(keyfile, group_pattern, key_fill, conf.pattern_fill);
    g_key_file_set_boolean(keyfile, group_pattern, key_avg,  conf.pattern_avg);

    /* Spectral scan */
    g_key_file_set_integer(keyfile, group_scan, key_x,          conf.scan_x);
    g_key_file_set_integer(keyfile, group_scan, key_y,          conf.scan_y);
    g_key_file_set_integer(keyfile, group_scan, key_width,      conf.scan_width);
    g_key_file_set_integer(keyfile, group_scan, key_height,     conf.scan_height);
    g_key_file_set_integer(keyfile, group_scan, key_start,      conf.scan_start);
    g_key_file_set_integer(keyfile, group_scan, key_end,        conf.scan_end);
    g_key_file_set_integer(keyfile, group_scan, key_step,       conf.scan_step);
    g_key_file_set_integer(keyfile, group_scan, key_bw,         conf.scan_bw);
    g_key_file_set_boolean(keyfile, group_scan, key_continuous, conf.scan_continuous);
    g_key_file_set_boolean(keyfile, group_scan, key_relative,   conf.scan_relative);
    g_key_file_set_boolean(keyfile, group_scan, key_peak_hold,  conf.scan_peakhold);
    g_key_file_set_boolean(keyfile, group_scan, key_mark_tuned, conf.scan_mark_tuned);
    conf_uniq_int_list_save(keyfile, group_scan, key_marks,     conf.scan_marks);

    if(!(configuration = g_key_file_to_data(keyfile, &length, &err)))
    {
        ui_dialog(NULL,
                  GTK_MESSAGE_ERROR,
                  "Configuration",
                  "Unable to generate the configuration file.\n%s",
                  err->message);
        g_error_free(err);
        err = NULL;
    }

    if((fp = fopen(path, "w")) == NULL)
    {
        ui_dialog(NULL,
                  GTK_MESSAGE_ERROR,
                  "Configuration",
                  "Unable to save the configuration to a file:\n%s",
                  path);
    }
    else
    {
        out = fwrite(configuration, sizeof(char), length, fp);
        if(out != length)
        {
            ui_dialog(NULL,
                      GTK_MESSAGE_ERROR,
                      "Configuration",
                      "Failed to save the configuration to a file:\n%s\n(wrote only %d of %d bytes)",
                      path, out, length);
        }
        fclose(fp);
    }
    g_free(configuration);
}

static gboolean
conf_read_boolean(GKeyFile    *key_file,
                  const gchar *group_name,
                  const gchar *key,
                  gboolean     default_value)
{
    gboolean value;
    GError *err = NULL;

    value = g_key_file_get_boolean(key_file, group_name, key, &err);
    if(err)
    {
        value = default_value;
        g_error_free(err);
    }
    return value;
}

static gint
conf_read_integer(GKeyFile    *key_file,
                  const gchar *group_name,
                  const gchar *key,
                  gint         default_value)
{
    gint value;
    GError *err = NULL;

    value = g_key_file_get_integer(key_file, group_name, key, &err);
    if(err)
    {
        value = default_value;
        g_error_free(err);
    }
    return value;
}

static gdouble
conf_read_double(GKeyFile    *key_file,
                 const gchar *group_name,
                 const gchar *key,
                 gdouble      default_value)
{
    gdouble value;
    GError *err = NULL;

    value = g_key_file_get_double(key_file, group_name, key, &err);
    if(err)
    {
        value = default_value;
        g_error_free(err);
    }
    return value;
}

static gchar*
conf_read_string(GKeyFile    *key_file,
                 const gchar *group_name,
                 const gchar *key,
                 const gchar *default_value)
{
    gchar *value;
    GError *err = NULL;

    value = g_key_file_get_string(key_file, group_name, key, &err);
    if(err)
    {
        value = g_strdup(default_value);
        g_error_free(err);
    }
    return value;
}

static void
conf_read_color(GKeyFile    *key_file,
                const gchar *group_name,
                const gchar *key,
                GdkColor    *color,
                const gchar *default_value)
{
    gchar *value;
    GError *err = NULL;

    value = g_key_file_get_string(key_file, group_name, key, &err);
    if(err)
    {
        g_error_free(err);
        gdk_color_parse(default_value, color);
        return;
    }

    if(!gdk_color_parse(value, color))
        gdk_color_parse(default_value, color);

    g_free(value);
}

static void
conf_save_color(GKeyFile       *key_file,
                const gchar    *group_name,
                const gchar    *key,
                const GdkColor *color)
{
    gchar *string = gdk_color_to_string(color);
    g_key_file_set_string(key_file, group_name, key, string);
    g_free(string);
}

static gchar**
conf_read_hosts(GKeyFile    *key_file,
                const gchar *group_name,
                const gchar *key,
                const gchar *default_host)
{
    gchar **values;
    GError *err = NULL;

    values = g_key_file_get_string_list(key_file, group_name, key, NULL, &err);
    if(err)
    {
        g_error_free(err);
        values = g_new(gchar*, 2);
        values[0] = g_strdup(default_host);
        values[1] = NULL;
    }
    return values;
}

static void
conf_read_integers(GKeyFile    *key_file,
                   const gchar *group_name,
                   const gchar *key,
                   gint         count,
                   const gint  *defaults,
                   gint        *integers)
{
    gint *values;
    gsize length;
    gint i;

    values = g_key_file_get_integer_list(key_file, group_name, key, &length, NULL);

    for(i=0; i<count; i++)
        integers[i] = (i<length) ? values[i] : (defaults ? defaults[i] : 0);

    g_free(values);
}

static GList*
conf_uniq_int_list_read(GKeyFile    *key_file,
                        const gchar *group_name,
                        const gchar *key)
{
    GList *list = NULL;
    gint *values;
    gsize length;
    gint i;

    values = g_key_file_get_integer_list(key_file, group_name, key, &length, NULL);
    for(i=0; i<length; i++)
        conf_uniq_int_list_add(&list, values[i]);
    g_free(values);

    return list;
}

static void
conf_uniq_int_list_save(GKeyFile    *key_file,
                        const gchar *group_name,
                        const gchar *key,
                        GList *list)
{
    gint n = g_list_length(conf.scan_marks);
    gint i = 0;
    gint *values = g_new(gint, n);

    while(list)
    {
        values[i++] = GPOINTER_TO_INT(list->data);
        list = list->next;
    }

    if(n)
        g_key_file_set_integer_list(key_file, group_name, key, values, n);
    else
        g_key_file_remove_key(key_file, group_name, key, NULL);

    g_free(values);
}

void
conf_uniq_int_list_add(GList **list,
                       gint    intval)
{
    gpointer value = GINT_TO_POINTER(intval);
    if(!g_list_find(*list, value))
        *list = g_list_prepend(*list, value);
}

void
conf_uniq_int_list_toggle(GList **list,
                          gint    intval)
{
    gpointer value = GINT_TO_POINTER(intval);
    GList *ptr = g_list_find(*list, value);
    if(ptr)
        *list = g_list_delete_link(*list, ptr);
    else
        *list = g_list_prepend(*list, value);
}

void
conf_uniq_int_list_remove(GList **list,
                          gint    intval)
{
    gpointer value = GINT_TO_POINTER(intval);
    *list = g_list_remove(*list, value);
}

void
conf_uniq_int_list_clear_range(GList **list,
                               gint    from,
                               gint    to)
{
    GList *ptr = *list;
    GList *next;
    gint freq;

    while(ptr)
    {
        next = ptr->next;
        freq = GPOINTER_TO_INT(ptr->data);
        if(freq >= from && freq <= to)
            *list = g_list_delete_link(*list, ptr);
        ptr = next;
    }
}

void
conf_uniq_int_list_clear(GList **list)
{
    g_list_free(*list);
    *list = NULL;
}

void
conf_update_string_const(gchar       **ptr,
                         const gchar  *str)
{

    g_free(*ptr);
    *ptr = g_strdup((str ? str : ""));
}

void
conf_update_string(gchar **ptr,
                   gchar  *str)
{
    g_free(*ptr);
    *ptr = str ? str : g_strdup("");
}

void
conf_add_host(const gchar *host)
{
    gchar **hosts;
    gint i, j;

    if(host == NULL)
        return;

    hosts = g_new0(gchar*, HOST_HISTORY_LEN+1);
    hosts[0] = g_strdup(host);

    for(i=0, j=1; (j<HOST_HISTORY_LEN && conf.host[i]); i++)
    {
        if(g_ascii_strcasecmp(host, conf.host[i]))
        {
            hosts[j++] = conf.host[i];
            conf.host[i] = NULL;
        }
    }
    g_strfreev(conf.host);
    conf.host = hosts;
}
