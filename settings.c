#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"
#include "settings.h"
#include "graph.h"
#include "rdsspy.h"
#include "version.h"
#include "log.h"
#include "scheduler.h"
#include "stationlist.h"
#ifdef G_OS_WIN32
#include "win32.h"
#endif

#define SCHEDULER_LIST_STORE_FREQ    0
#define SCHEDULER_LIST_STORE_TIMEOUT 1

/* Scheduler page */
GtkWidget *page_scheduler, *scheduler_treeview, *scheduler_scroll;
GtkListStore *scheduler_liststore;
GtkCellRenderer *scheduler_renderer_freq, *scheduler_renderer_timeout;
GtkWidget *scheduler_add_box, *s_scheduler_freq, *s_scheduler_timeout;
GtkWidget *b_scheduler_add, *scheduler_right_align, *scheduler_right_box;
GtkWidget *b_scheduler_remove, *b_scheduler_clear;

void settings_read()
{
    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;
    gsize length;
    gint i, *p;

    if(!g_key_file_load_from_file(keyfile, CONF_FILE, G_KEY_FILE_KEEP_COMMENTS, &error))
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "Configuration file not found.\nUsing default settings.");
        gtk_window_set_title(GTK_WINDOW(dialog), APP_NAME);
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        g_key_file_free(keyfile);

        /* Window */
        conf.win_x = 0;
        conf.win_y = 0;

        /* Connection */
        conf.network = 0;
#ifdef G_OS_WIN32
        conf.serial = g_strdup("COM3");
#else
        conf.serial = g_strdup("ttyUSB0");
#endif
        conf.host = g_new(char*, 2);
        conf.host[0] = g_strdup("localhost");
        conf.host[1] = NULL;
        conf.port = 7373;
        conf.password = g_strdup("");

        /* Tuner settings */
        conf.rfgain = 0;
        conf.ifgain = 0;
        conf.agc = 2;
        conf.deemphasis = 0;

        /* Interface */
        conf.init_freq = 87500;
        conf.signal_offset = 0.0;
        conf.signal_unit = UNIT_DBF;
        conf.event_action = ACTION_NONE;
        conf.utc = TRUE;
        conf.autoconnect = FALSE;
        conf.amstep = FALSE;
        conf.disconnect_confirm = TRUE;
        conf.auto_reconnect = FALSE;
        conf.grab_focus = FALSE;
        conf.hide_decorations = FALSE;
        conf.hide_status = FALSE;
        conf.restore_pos = TRUE;

        /* Graph */
        conf.signal_display = SIGNAL_GRAPH;
        conf.graph_mode = GRAPH_DEFAULT;
        gdk_color_parse("#B5B5FF", &conf.color_mono);
        gdk_color_parse("#8080FF", &conf.color_stereo);
        gdk_color_parse("#3333FF", &conf.color_rds);
        conf.graph_height = 100;
        conf.show_grid = TRUE;
        conf.signal_avg = FALSE;

        /* RDS */
        conf.rds_pty = 0;
        conf.rds_reset = FALSE;
        conf.rds_reset_timeout = 60;
        conf.ps_info_error = 2;
        conf.ps_data_error = 1;
        conf.rds_ps_progressive = FALSE;
        conf.rt_info_error = 1;
        conf.rt_data_error = 1;

        /* Antenna */
        conf.alignment = FALSE;
        conf.swap_rotator = FALSE;
        conf.ant_count = ANT_COUNT;
        conf.ant_clear_rds = TRUE;
        conf.ant_switching = FALSE;

        /* Logs */
        conf.rds_spy_port = 7376;
        conf.rds_spy_auto = FALSE;
        conf.rds_spy_run = FALSE;
        conf.rds_spy_command = g_strdup("");
        conf.stationlist = FALSE;
        conf.stationlist_port = 9031;
        conf.rds_logging = FALSE;
        conf.replace_spaces = TRUE;
        conf.log_dir = g_strdup("");
        conf.screen_dir = g_strdup("");

        /* Keyboard */
        conf.key_tune_up = GDK_Right;
        conf.key_tune_down = GDK_Left;
        conf.key_tune_up_5 = GDK_Up;
        conf.key_tune_down_5 = GDK_Down;
        conf.key_tune_up_1000 = GDK_Page_Up;
        conf.key_tune_down_1000 = GDK_Page_Down;
        conf.key_tune_back = GDK_B;
        conf.key_reset = GDK_R;
        conf.key_screen = GDK_S;
        conf.key_bw_up = GDK_KEY_bracketright;
        conf.key_bw_down = GDK_KEY_bracketleft;
        conf.key_bw_auto = GDK_KEY_backslash;
        conf.key_rotate_cw = GDK_KEY_Home;
        conf.key_rotate_ccw = GDK_KEY_End;
        conf.key_switch_ant = GDK_KEY_Delete;
        conf.key_ps_mode = GDK_KEY_grave;
        conf.key_spectral_toggle = GDK_KEY_Q;

        /* Presets */
        conf.presets[0] = 87600;
        conf.presets[1] = 89400;
        conf.presets[2] = 91300;
        conf.presets[3] = 93100;
        conf.presets[4] = 95000;
        conf.presets[5] = 96800;
        conf.presets[6] = 98600;
        conf.presets[7] = 100500;
        conf.presets[8] = 102400;
        conf.presets[9] = 104200;
        conf.presets[10] = 106100;
        conf.presets[11] = 107900;

        /* Scheduler */
        conf.scheduler_n = 0;
        conf.scheduler_freqs = NULL;
        conf.scheduler_timeouts = NULL;
        conf.scheduler_default_timeout = 30;

        /* Pattern */
        conf.pattern_size = 600;
        conf.pattern_fill = TRUE;
        conf.pattern_avg = FALSE;

        /* Spectral scan */
        conf.scan_x = 0;
        conf.scan_y = 0;
        conf.scan_width = 950;
        conf.scan_height = 150;
        conf.scan_start = 87500;
        conf.scan_end = 108000;
        conf.scan_step = 100;
        conf.scan_bw = 12;
        conf.scan_continuous = FALSE;
        conf.scan_relative = FALSE;
        conf.scan_peakhold = TRUE;
        conf.scan_freq_list = NULL;

        settings_write();
        return;
    }

    /* Window */
    conf.win_x = g_key_file_get_integer(keyfile, "window", "x", NULL);
    conf.win_y = g_key_file_get_integer(keyfile, "window", "y", NULL);

    /* Connection */
    conf.network = g_key_file_get_integer(keyfile, "connection", "network", NULL);
    conf.serial = settings_read_string(g_key_file_get_string(keyfile, "connection", "serial", NULL));
    conf.host = g_key_file_get_string_list(keyfile, "connection", "host", NULL, NULL);
    conf.port = g_key_file_get_integer(keyfile, "connection", "port", NULL);
    conf.password = settings_read_string(g_key_file_get_string(keyfile, "connection", "password", NULL));

    /* Tuner settings */
    conf.rfgain = g_key_file_get_boolean(keyfile, "tuner", "rfgain", NULL);
    conf.ifgain = g_key_file_get_boolean(keyfile, "tuner", "ifgain", NULL);
    conf.agc = g_key_file_get_integer(keyfile, "tuner", "agc", NULL);
    conf.deemphasis = g_key_file_get_integer(keyfile, "tuner", "deemphasis", NULL);

    /* Interface */
    conf.init_freq = g_key_file_get_integer(keyfile, "settings", "init_freq", NULL);
    conf.signal_offset = g_key_file_get_double(keyfile, "settings", "signal_offset", NULL);
    conf.signal_unit = g_key_file_get_integer(keyfile, "settings", "signal_unit", NULL);
    conf.event_action = g_key_file_get_integer(keyfile, "settings", "event_action", NULL);
    conf.utc = g_key_file_get_boolean(keyfile, "settings", "utc", NULL);
    conf.autoconnect = g_key_file_get_boolean(keyfile, "settings", "autoconnect", NULL);
    conf.amstep = g_key_file_get_boolean(keyfile, "settings", "amstep", NULL);
    conf.disconnect_confirm = g_key_file_get_boolean(keyfile, "settings", "disconnect_confirm", NULL);
    conf.auto_reconnect = g_key_file_get_boolean(keyfile, "settings", "auto_reconnect", NULL);
    conf.grab_focus = g_key_file_get_boolean(keyfile, "settings", "grab_focus", NULL);
    conf.hide_decorations = g_key_file_get_boolean(keyfile, "settings", "hide_decorations", NULL);
    conf.hide_status = g_key_file_get_boolean(keyfile, "settings", "hide_status", NULL);
    conf.restore_pos = g_key_file_get_boolean(keyfile, "settings", "restore_pos", NULL);

    /* Graph */
    conf.signal_display = g_key_file_get_integer(keyfile, "settings", "signal_display", NULL);
    conf.graph_mode = g_key_file_get_integer(keyfile, "settings", "graph_mode", NULL);
    conf.graph_height = g_key_file_get_integer(keyfile, "settings", "graph_height", NULL);
    gdk_color_parse(g_key_file_get_string(keyfile, "settings", "color_mono", NULL), &conf.color_mono);
    gdk_color_parse(g_key_file_get_string(keyfile, "settings", "color_stereo", NULL), &conf.color_stereo);
    gdk_color_parse(g_key_file_get_string(keyfile, "settings", "color_rds", NULL), &conf.color_rds);
    conf.show_grid = g_key_file_get_boolean(keyfile, "settings", "show_grid", NULL);
    conf.signal_avg = g_key_file_get_boolean(keyfile, "settings", "signal_avg", NULL);

    /* RDS */
    conf.rds_pty = g_key_file_get_integer(keyfile, "rds", "rds_pty", NULL);
    conf.rds_reset = g_key_file_get_boolean(keyfile, "rds", "rds_reset", NULL);
    conf.rds_reset_timeout = g_key_file_get_integer(keyfile, "rds", "rds_reset_timeout", NULL);
    conf.ps_info_error = g_key_file_get_integer(keyfile, "rds", "ps_info_error", NULL);
    conf.ps_data_error = g_key_file_get_integer(keyfile, "rds", "ps_data_error", NULL);
    conf.rds_ps_progressive = g_key_file_get_boolean(keyfile, "rds", "rds_ps_progressive", NULL);
    conf.rt_info_error = g_key_file_get_integer(keyfile, "rds", "rt_info_error", NULL);
    conf.rt_data_error = g_key_file_get_integer(keyfile, "rds", "rt_data_error", NULL);

    /* Antenna */
    conf.alignment = g_key_file_get_boolean(keyfile, "antenna", "alignment", NULL);
    conf.swap_rotator = g_key_file_get_boolean(keyfile, "antenna", "swap_rotator", NULL);
    conf.ant_count = g_key_file_get_integer(keyfile, "antenna", "ant_count", NULL);
    conf.ant_clear_rds = g_key_file_get_boolean(keyfile, "antenna", "ant_clear_rds", NULL);
    conf.ant_switching = g_key_file_get_boolean(keyfile, "antenna", "ant_switching", NULL);
    p = g_key_file_get_integer_list(keyfile, "antenna", "ant_start", &length, NULL);
    for(i=0; i<ANT_COUNT; i++)
    {
        conf.ant_start[i] = (i<length) ? p[i] : 0;
    }
    g_free(p);
    p = g_key_file_get_integer_list(keyfile, "antenna", "ant_stop", &length, NULL);
    for(i=0; i<ANT_COUNT; i++)
    {
        conf.ant_stop[i] = (i<length) ? p[i] : 0;
    }
    g_free(p);

    /* Logs */
    conf.rds_spy_port = g_key_file_get_integer(keyfile, "logs", "rds_spy_port", NULL);
    conf.rds_spy_auto = g_key_file_get_boolean(keyfile, "logs", "rds_spy_auto", NULL);
    conf.rds_spy_run = g_key_file_get_boolean(keyfile, "logs", "rds_spy_run", NULL);
    conf.rds_spy_command = settings_read_string(g_key_file_get_string(keyfile, "logs", "rds_spy_command", NULL));
    conf.stationlist = g_key_file_get_boolean(keyfile, "logs", "stationlist", NULL);
    conf.stationlist_port = g_key_file_get_integer(keyfile, "logs", "stationlist_port", NULL);
    conf.rds_logging = g_key_file_get_boolean(keyfile, "logs", "rds_logging", NULL);
    conf.replace_spaces = g_key_file_get_boolean(keyfile, "logs", "replace_spaces", NULL);
    conf.log_dir = settings_read_string(g_key_file_get_string(keyfile, "logs", "log_dir", NULL));
    conf.screen_dir = settings_read_string(g_key_file_get_string(keyfile, "logs", "screen_dir", NULL));

    /* Keyboard */
    conf.key_tune_up = g_key_file_get_integer(keyfile, "keyboard", "key_tune_up", NULL);
    conf.key_tune_down = g_key_file_get_integer(keyfile, "keyboard", "key_tune_down", NULL);
    conf.key_tune_up_5 = g_key_file_get_integer(keyfile, "keyboard", "key_tune_up_5", NULL);
    conf.key_tune_down_5 = g_key_file_get_integer(keyfile, "keyboard", "key_tune_down_5", NULL);
    conf.key_tune_up_1000 = g_key_file_get_integer(keyfile, "keyboard", "key_tune_up_1000", NULL);
    conf.key_tune_down_1000 = g_key_file_get_integer(keyfile, "keyboard", "key_tune_down_1000", NULL);
    conf.key_tune_back = g_key_file_get_integer(keyfile, "keyboard", "key_tune_back", NULL);
    conf.key_reset = g_key_file_get_integer(keyfile, "keyboard", "key_reset", NULL);
    conf.key_screen = g_key_file_get_integer(keyfile, "keyboard", "key_screen", NULL);
    conf.key_bw_up = g_key_file_get_integer(keyfile, "keyboard", "key_bw_up", NULL);
    conf.key_bw_down = g_key_file_get_integer(keyfile, "keyboard", "key_bw_down", NULL);
    conf.key_bw_auto = g_key_file_get_integer(keyfile, "keyboard", "key_bw_auto", NULL);
    conf.key_rotate_cw = g_key_file_get_integer(keyfile, "keyboard", "key_rotate_cw", NULL);
    conf.key_rotate_ccw = g_key_file_get_integer(keyfile, "keyboard", "key_rotate_ccw", NULL);
    conf.key_switch_ant = g_key_file_get_integer(keyfile, "keyboard", "key_switch_ant", NULL);
    conf.key_ps_mode = g_key_file_get_integer(keyfile, "keyboard", "key_ps_mode", NULL);
    conf.key_spectral_toggle = g_key_file_get_integer(keyfile, "keyboard", "key_spectral_toggle", NULL);

    /* Presets */
    p = g_key_file_get_integer_list(keyfile, "settings", "presets", &length, NULL);
    for(i=0; i<PRESETS; i++)
    {
        conf.presets[i] = (i<length) ? p[i] : 0;
    }
    g_free(p);

    /* Scheduler */
    conf.scheduler_freqs = g_key_file_get_integer_list(keyfile, "scheduler", "scheduler_freqs", &length, NULL);
    conf.scheduler_n = length;
    conf.scheduler_timeouts = g_key_file_get_integer_list(keyfile, "scheduler", "scheduler_timeouts", &length, NULL);
    conf.scheduler_n = (conf.scheduler_n > length)?length:conf.scheduler_n;
    conf.scheduler_default_timeout = g_key_file_get_integer(keyfile, "scheduler", "scheduler_default_timeout", NULL);

    /* Pattern */
    conf.pattern_size = g_key_file_get_integer(keyfile, "pattern", "pattern_size", NULL);
    conf.pattern_fill = g_key_file_get_boolean(keyfile, "pattern", "pattern_fill", NULL);
    conf.pattern_avg = g_key_file_get_boolean(keyfile, "pattern", "pattern_avg", NULL);

    /* Spectral scan */
    conf.scan_x = g_key_file_get_integer(keyfile, "scan", "scan_x", NULL);
    conf.scan_y = g_key_file_get_integer(keyfile, "scan", "scan_y", NULL);
    conf.scan_width = g_key_file_get_integer(keyfile, "scan", "scan_width", NULL);
    conf.scan_height = g_key_file_get_integer(keyfile, "scan", "scan_height", NULL);
    conf.scan_start = g_key_file_get_integer(keyfile, "scan", "scan_start", NULL);
    conf.scan_end = g_key_file_get_integer(keyfile, "scan", "scan_end", NULL);
    conf.scan_step = g_key_file_get_integer(keyfile, "scan", "scan_step", NULL);
    conf.scan_bw = g_key_file_get_integer(keyfile, "scan", "scan_bw", NULL);
    conf.scan_continuous = g_key_file_get_boolean(keyfile, "scan", "scan_continuous", NULL);
    conf.scan_relative = g_key_file_get_boolean(keyfile, "scan", "scan_relative", NULL);
    conf.scan_peakhold = g_key_file_get_boolean(keyfile, "scan", "scan_peakhold", NULL);
    conf.scan_freq_list = NULL;

    p = g_key_file_get_integer_list(keyfile, "scan", "scan_freq_list", &length, NULL);
    for(i=0; i<length; i++)
    {
        settings_scan_mark_add(p[i]);
    }
    g_free(p);

    g_key_file_free(keyfile);
}

void settings_write()
{
    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;
    gsize length;
    gchar *tmp;
    gint *int_ptr = NULL;
    FILE *f;

    /* Window */
    g_key_file_set_integer(keyfile, "window", "x", conf.win_x);
    g_key_file_set_integer(keyfile, "window", "y", conf.win_y);

    /* Connection */
    g_key_file_set_integer(keyfile, "connection", "network", conf.network);
    g_key_file_set_string(keyfile, "connection", "serial", conf.serial);
    g_key_file_set_string_list(keyfile, "connection", "host", (const gchar* const*)conf.host, g_strv_length(conf.host));
    g_key_file_set_integer(keyfile, "connection", "port", conf.port);
    g_key_file_set_string(keyfile, "connection", "password", conf.password);

    /* Tuner settings */
    g_key_file_set_boolean(keyfile, "tuner", "rfgain", conf.rfgain);
    g_key_file_set_boolean(keyfile, "tuner", "ifgain", conf.ifgain);
    g_key_file_set_integer(keyfile, "tuner", "agc", conf.agc);
    g_key_file_set_integer(keyfile, "tuner", "deemphasis", conf.deemphasis);

    /* Interface */
    g_key_file_set_integer(keyfile, "settings", "init_freq", conf.init_freq);
    g_key_file_set_double(keyfile, "settings", "signal_offset", conf.signal_offset);
    g_key_file_set_integer(keyfile, "settings", "signal_unit", conf.signal_unit);
    g_key_file_set_integer(keyfile, "settings", "event_action", conf.event_action);
    g_key_file_set_boolean(keyfile, "settings", "utc", conf.utc);
    g_key_file_set_boolean(keyfile, "settings", "autoconnect", conf.autoconnect);
    g_key_file_set_boolean(keyfile, "settings", "amstep", conf.amstep);
    g_key_file_set_boolean(keyfile, "settings", "disconnect_confirm", conf.disconnect_confirm);
    g_key_file_set_boolean(keyfile, "settings", "auto_reconnect", conf.auto_reconnect);
    g_key_file_set_boolean(keyfile, "settings", "grab_focus", conf.grab_focus);
    g_key_file_set_boolean(keyfile, "settings", "hide_decorations", conf.hide_decorations);
    g_key_file_set_boolean(keyfile, "settings", "hide_status", conf.hide_status);
    g_key_file_set_boolean(keyfile, "settings", "restore_pos", conf.restore_pos);

    /* Graph */
    g_key_file_set_integer(keyfile, "settings", "signal_display", conf.signal_display);
    g_key_file_set_integer(keyfile, "settings", "graph_mode", conf.graph_mode);
    tmp = gdk_color_to_string(&conf.color_mono);
    g_key_file_set_string(keyfile, "settings", "color_mono", tmp);
    g_free(tmp);
    tmp = gdk_color_to_string(&conf.color_stereo);
    g_key_file_set_string(keyfile, "settings", "color_stereo", tmp);
    g_free(tmp);
    tmp = gdk_color_to_string(&conf.color_rds);
    g_key_file_set_string(keyfile, "settings", "color_rds", tmp);
    g_free(tmp);
    g_key_file_set_integer(keyfile, "settings", "graph_height", conf.graph_height);
    g_key_file_set_boolean(keyfile, "settings", "show_grid", conf.show_grid);
    g_key_file_set_boolean(keyfile, "settings", "signal_avg", conf.signal_avg);

    /* RDS */
    g_key_file_set_integer(keyfile, "rds", "rds_pty", conf.rds_pty);
    g_key_file_set_boolean(keyfile, "rds", "rds_reset", conf.rds_reset);
    g_key_file_set_integer(keyfile, "rds", "rds_reset_timeout", conf.rds_reset_timeout);
    g_key_file_set_integer(keyfile, "rds", "ps_info_error", conf.ps_info_error);
    g_key_file_set_integer(keyfile, "rds", "ps_data_error", conf.ps_data_error);
    g_key_file_set_boolean(keyfile, "rds", "rds_ps_progressive", conf.rds_ps_progressive);
    g_key_file_set_integer(keyfile, "rds", "rt_info_error", conf.rt_info_error);
    g_key_file_set_integer(keyfile, "rds", "rt_data_error", conf.rt_data_error);

    /* Antenna */
    g_key_file_set_boolean(keyfile, "antenna", "alignment", conf.alignment);
    g_key_file_set_boolean(keyfile, "antenna", "swap_rotator", conf.swap_rotator);
    g_key_file_set_integer(keyfile, "antenna", "ant_count", conf.ant_count);
    g_key_file_set_boolean(keyfile, "antenna", "ant_clear_rds", conf.ant_clear_rds);
    g_key_file_set_boolean(keyfile, "antenna", "ant_switching", conf.ant_switching);
    g_key_file_set_integer_list(keyfile, "antenna", "ant_start", conf.ant_start, ANT_COUNT);
    g_key_file_set_integer_list(keyfile, "antenna", "ant_stop", conf.ant_stop, ANT_COUNT);

    /* Logs */
    g_key_file_set_integer(keyfile, "logs", "rds_spy_port", conf.rds_spy_port);
    g_key_file_set_boolean(keyfile, "logs", "rds_spy_auto", conf.rds_spy_auto);
    g_key_file_set_boolean(keyfile, "logs", "rds_spy_run", conf.rds_spy_run);
    g_key_file_set_string(keyfile, "logs", "rds_spy_command", conf.rds_spy_command);
    g_key_file_set_boolean(keyfile, "logs", "stationlist", conf.stationlist);
    g_key_file_set_integer(keyfile, "logs", "stationlist_port", conf.stationlist_port);
    g_key_file_set_boolean(keyfile, "logs", "rds_logging", conf.rds_logging);
    g_key_file_set_boolean(keyfile, "logs", "replace_spaces", conf.replace_spaces);
    g_key_file_set_string(keyfile, "logs", "log_dir", conf.log_dir);
    g_key_file_set_string(keyfile, "logs", "screen_dir", conf.screen_dir);

    /* Keyboard */
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_up", conf.key_tune_up);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_down", conf.key_tune_down);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_up_5", conf.key_tune_up_5);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_down_5", conf.key_tune_down_5);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_up_1000", conf.key_tune_up_1000);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_down_1000", conf.key_tune_down_1000);
    g_key_file_set_integer(keyfile, "keyboard", "key_tune_back", conf.key_tune_back);
    g_key_file_set_integer(keyfile, "keyboard", "key_reset", conf.key_reset);
    g_key_file_set_integer(keyfile, "keyboard", "key_screen", conf.key_screen);
    g_key_file_set_integer(keyfile, "keyboard", "key_bw_up", conf.key_bw_up);
    g_key_file_set_integer(keyfile, "keyboard", "key_bw_down", conf.key_bw_down);
    g_key_file_set_integer(keyfile, "keyboard", "key_bw_auto", conf.key_bw_auto);
    g_key_file_set_integer(keyfile, "keyboard", "key_rotate_cw", conf.key_rotate_cw);
    g_key_file_set_integer(keyfile, "keyboard", "key_rotate_ccw", conf.key_rotate_ccw);
    g_key_file_set_integer(keyfile, "keyboard", "key_switch_ant", conf.key_switch_ant);
    g_key_file_set_integer(keyfile, "keyboard", "key_ps_mode", conf.key_ps_mode);
    g_key_file_set_integer(keyfile, "keyboard", "key_spectral_toggle", conf.key_spectral_toggle);

    /* Presets */
    g_key_file_set_integer_list(keyfile, "settings", "presets", conf.presets, PRESETS);

    /* Scheduler */
    if(conf.scheduler_n)
    {
        g_key_file_set_integer_list(keyfile, "scheduler", "scheduler_freqs", conf.scheduler_freqs, conf.scheduler_n);
        g_key_file_set_integer_list(keyfile, "scheduler", "scheduler_timeouts", conf.scheduler_timeouts, conf.scheduler_n);
    }
    g_key_file_set_integer(keyfile, "scheduler", "scheduler_default_timeout", conf.scheduler_default_timeout);

    /* Pattern */
    g_key_file_set_integer(keyfile, "pattern", "pattern_size", conf.pattern_size);
    g_key_file_set_boolean(keyfile, "pattern", "pattern_fill", conf.pattern_fill);
    g_key_file_set_boolean(keyfile, "pattern", "pattern_avg", conf.pattern_avg);

    /* Spectral scan */
    g_key_file_set_integer(keyfile, "scan", "scan_x", conf.scan_x);
    g_key_file_set_integer(keyfile, "scan", "scan_y", conf.scan_y);
    g_key_file_set_integer(keyfile, "scan", "scan_width", conf.scan_width);
    g_key_file_set_integer(keyfile, "scan", "scan_height", conf.scan_height);
    g_key_file_set_integer(keyfile, "scan", "scan_start", conf.scan_start);
    g_key_file_set_integer(keyfile, "scan", "scan_end", conf.scan_end);
    g_key_file_set_integer(keyfile, "scan", "scan_step", conf.scan_step);
    g_key_file_set_integer(keyfile, "scan", "scan_bw", conf.scan_bw);
    g_key_file_set_boolean(keyfile, "scan", "scan_continuous", conf.scan_continuous);
    g_key_file_set_boolean(keyfile, "scan", "scan_relative", conf.scan_relative);
    g_key_file_set_boolean(keyfile, "scan", "scan_peakhold", conf.scan_peakhold);

    if(conf.scan_freq_list)
    {
        GList *l = conf.scan_freq_list;
        gint n = g_list_length(conf.scan_freq_list);
        gint i = 0;

        int_ptr = g_new(gint, n);
        while(l)
        {
            int_ptr[i++] = GPOINTER_TO_INT(l->data);
            l = l->next;
        }

        g_key_file_set_integer_list(keyfile, "scan", "scan_freq_list", int_ptr, n);
    }

    if(!(tmp = g_key_file_to_data(keyfile, &length, &error)))
    {
        dialog_error(APP_NAME, "Unable to generate the configuration file.");
        g_error_free(error);
        error = NULL;
    }
    g_key_file_free(keyfile);
    g_free(int_ptr);

    if((f = fopen(CONF_FILE, "w")) == NULL)
    {
        dialog_error(APP_NAME, "Unable to save the configuration file.");
    }
    else
    {
        fwrite(tmp, 1, length, f);
        fclose(f);
    }

    g_free(tmp);
}

void settings_dialog()
{
    GtkWidget *dialog, *notebook;
    GtkWidget *page_interface, *page_graph, *page_rds, *page_ant, *page_logs, *page_key, *page_presets, *page_about;
    GtkWidget *table_interface, *box_init_freq, *l_init_freq, *l_init_freq_unit, *s_init_freq, *l_signal_offset, *box_signal_offset, *s_signal_offset, *l_signal_offset_unit, *l_unit, *c_unit, *x_utc, *x_replace, *x_alignment, *x_swap_rotator, *x_autoconnect, *x_amstep, *x_disconnect_confirm, *x_auto_reconnect, *l_event, *c_event, *x_hide_decorations, *x_hide_status, *x_restore_pos;
    GtkWidget *table_graph, *l_display, *c_display, *l_graph_mode, *c_graph_mode, *l_height, *s_height, *l_colors, *box_colors, *c_mono, *c_stereo, *c_rds, *x_avg, *x_grid;
    GtkWidget *s_ant_start[ANT_COUNT], *s_ant_stop[ANT_COUNT];

    GtkWidget *table_logs;
    GtkWidget *x_stationlist, *l_stationlist_port, *s_stationlist_port, *x_rds_logging;

    GtkWidget *table_presets, *s_presets[PRESETS];
    gint row, i, result;
    gchar tmp[10];

    dialog = gtk_dialog_new_with_buttons("Settings",
                                         GTK_WINDOW(gui.window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         NULL);

    gtk_window_set_icon_name(GTK_WINDOW(dialog), "xdr-gtk-settings");
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

    notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), notebook);

    /* Interface page */
    page_interface = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_interface), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_interface, gtk_label_new("Interface"));
    gtk_container_child_set(GTK_CONTAINER(notebook), page_interface, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    table_interface = gtk_table_new(13, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(table_interface), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_interface), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table_interface), 4);
    gtk_container_add(GTK_CONTAINER(page_interface), table_interface);


    row = 0;
    l_init_freq = gtk_label_new("Initial frequency:");
    gtk_misc_set_alignment(GTK_MISC(l_init_freq), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_interface), l_init_freq, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    box_init_freq = gtk_hbox_new(FALSE, 7);
    s_init_freq = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.init_freq, TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
    gtk_box_pack_start(GTK_BOX(box_init_freq), s_init_freq, TRUE, TRUE, 0);
    l_init_freq_unit = gtk_label_new("kHz");
    gtk_box_pack_start(GTK_BOX(box_init_freq), l_init_freq_unit, FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table_interface), box_init_freq, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

//    row++;
//    l_signal_offset = gtk_label_new("Signal level offset:");
//    gtk_misc_set_alignment(GTK_MISC(l_signal_offset), 0.0, 0.5);
//    gtk_table_attach(GTK_TABLE(table_interface), l_signal_offset, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

//    box_signal_offset = gtk_hbox_new(FALSE, 7);
//    s_signal_offset = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.signal_offset, -50.0, +50.0, 1.0, 2.0, 0.0)), 0, 1);
//    gtk_box_pack_start(GTK_BOX(box_signal_offset), s_signal_offset, TRUE, TRUE, 0);
//    l_signal_offset_unit = gtk_label_new("dB");
//    gtk_box_pack_start(GTK_BOX(box_signal_offset), l_signal_offset_unit, FALSE, FALSE, 0);
//    gtk_table_attach(GTK_TABLE(table_interface), box_signal_offset, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    l_unit = gtk_label_new("Signal unit:");
    gtk_misc_set_alignment(GTK_MISC(l_unit), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_interface), l_unit, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    c_unit = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "dBf");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "dBm");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "dBuV");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_unit), "S-meter");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_unit), conf.signal_unit);
    gtk_table_attach(GTK_TABLE(table_interface), c_unit, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    l_event = gtk_label_new("External event:");
    gtk_misc_set_alignment(GTK_MISC(l_event), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_interface), l_event, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    c_event = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_event), "no action");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_event), "grab focus");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_event), "screenshot");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_event), conf.event_action);
    gtk_table_attach(GTK_TABLE(table_interface), c_event, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_utc = gtk_check_button_new_with_label("Use UTC instead of local time");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_utc), conf.utc);
    gtk_table_attach(GTK_TABLE(table_interface), x_utc, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_autoconnect = gtk_check_button_new_with_label("Automatic connect on startup");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_autoconnect), conf.autoconnect);
    gtk_table_attach(GTK_TABLE(table_interface), x_autoconnect, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_amstep = gtk_check_button_new_with_label("10kHz MW steps instead of 9kHz");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_amstep), conf.amstep);
    gtk_table_attach(GTK_TABLE(table_interface), x_amstep, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_disconnect_confirm = gtk_check_button_new_with_label("Show disconnect confirmation");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_disconnect_confirm), conf.disconnect_confirm);
    gtk_table_attach(GTK_TABLE(table_interface), x_disconnect_confirm, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_auto_reconnect = gtk_check_button_new_with_label("Automatic re-connect");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_auto_reconnect), conf.auto_reconnect);
    gtk_table_attach(GTK_TABLE(table_interface), x_auto_reconnect, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_hide_decorations = gtk_check_button_new_with_label("Hide window decorations");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_hide_decorations), conf.hide_decorations);
    gtk_table_attach(GTK_TABLE(table_interface), x_hide_decorations, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_hide_status = gtk_check_button_new_with_label("Hide statusbar (clock)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_hide_status), conf.hide_status);
    gtk_table_attach(GTK_TABLE(table_interface), x_hide_status, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_restore_pos = gtk_check_button_new_with_label("Restore window position");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_restore_pos), conf.restore_pos);
    gtk_table_attach(GTK_TABLE(table_interface), x_restore_pos, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    /* Graph page */
    page_graph = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_graph), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_graph, gtk_label_new("Graph"));
    gtk_container_child_set(GTK_CONTAINER(notebook), page_graph, "tab-expand", FALSE, "tab-fill", FALSE, NULL);

    table_graph = gtk_table_new(6, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(table_graph), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_graph), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table_graph), 4);
    gtk_container_add(GTK_CONTAINER(page_graph), table_graph);

    row = 0;
    l_display = gtk_label_new("Signal display:");
    gtk_misc_set_alignment(GTK_MISC(l_display), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_graph), l_display, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    c_display = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_display), "text only");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_display), "graph");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_display), "bar");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_display), conf.signal_display);
    gtk_table_attach(GTK_TABLE(table_graph), c_display, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    l_graph_mode = gtk_label_new("Frequency change:");
    gtk_misc_set_alignment(GTK_MISC(l_graph_mode), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_graph), l_graph_mode, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    c_graph_mode = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_graph_mode), "no action");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_graph_mode), "reset graph");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_graph_mode), "add separator");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_graph_mode), conf.graph_mode);
    gtk_table_attach(GTK_TABLE(table_graph), c_graph_mode, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    l_colors = gtk_label_new("Graph colors:");
    gtk_misc_set_alignment(GTK_MISC(l_colors), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_graph), l_colors, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    box_colors = gtk_hbox_new(FALSE, 5);
    c_mono = gtk_color_button_new_with_color(&conf.color_mono);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_mono), "Mono color");
    gtk_widget_set_tooltip_text(c_mono, "Mono color");
    gtk_box_pack_start(GTK_BOX(box_colors), c_mono, TRUE, TRUE, 0);

    c_stereo = gtk_color_button_new_with_color(&conf.color_stereo);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_stereo), "Stereo color");
    gtk_widget_set_tooltip_text(c_stereo, "Stereo color");
    gtk_box_pack_start(GTK_BOX(box_colors), c_stereo, TRUE, TRUE, 0);

    c_rds = gtk_color_button_new_with_color(&conf.color_rds);
    gtk_color_button_set_title(GTK_COLOR_BUTTON(c_rds), "RDS color");
    gtk_widget_set_tooltip_text(c_rds, "RDS color");
    gtk_box_pack_start(GTK_BOX(box_colors), c_rds, TRUE, TRUE, 0);

    gtk_table_attach(GTK_TABLE(table_graph), box_colors, 1, 2, row, row+1, 0, 0, 0, 0);

    row++;
    l_height = gtk_label_new("Graph height [px]:");
    gtk_misc_set_alignment(GTK_MISC(l_height), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_graph), l_height, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    s_height = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.graph_height, 50.0, 300.0, 5.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(table_graph), s_height, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_avg = gtk_check_button_new_with_label("Average the signal level");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_avg), conf.signal_avg);
    gtk_table_attach(GTK_TABLE(table_graph), x_avg, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_grid = gtk_check_button_new_with_label("Draw grid");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_grid), conf.show_grid);
    gtk_table_attach(GTK_TABLE(table_graph), x_grid, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    /* RDS page */
    page_rds = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_rds), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_rds, gtk_label_new("RDS"));

    GtkWidget *table_rds = gtk_table_new(13, 2, TRUE);
    gtk_table_set_homogeneous(GTK_TABLE(table_rds), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_rds), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table_rds), 4);
    gtk_container_add(GTK_CONTAINER(page_rds), table_rds);

    row = 0;
    GtkWidget *l_pty = gtk_label_new("PTY set:");
    gtk_misc_set_alignment(GTK_MISC(l_pty), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_pty, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_pty = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_pty), "RDS/EU");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_pty), "RBDS/US");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_pty), conf.rds_pty);
    gtk_table_attach(GTK_TABLE(table_rds), c_pty, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_rds_reset = gtk_check_button_new_with_label("Reset RDS after specified timeout");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rds_reset), conf.rds_reset);
    gtk_table_attach(GTK_TABLE(table_rds), x_rds_reset, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rds_timeout = gtk_label_new("RDS timeout [s]:");
    gtk_misc_set_alignment(GTK_MISC(l_rds_timeout), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_rds_timeout, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *s_reset = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.rds_reset_timeout, 1.0, 10000.0, 10.0, 60.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(table_rds), s_reset, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    gtk_table_attach(GTK_TABLE(table_rds), gtk_hseparator_new(), 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_ps_error = gtk_label_new("RDS PS error correction:");
    gtk_table_attach(GTK_TABLE(table_rds), l_ps_error, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_ps_info_error = gtk_label_new("Group Type / Position:");
    gtk_misc_set_alignment(GTK_MISC(l_ps_info_error), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_ps_info_error, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *c_ps_info_error = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ps_info_error), "no errors");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ps_info_error), "max 2-bit");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ps_info_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_ps_info_error), conf.ps_info_error);
    gtk_table_attach(GTK_TABLE(table_rds), c_ps_info_error, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_ps_data_error = gtk_label_new("Data:");
    gtk_misc_set_alignment(GTK_MISC(l_ps_data_error), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_ps_data_error, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *c_ps_data_error = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ps_data_error), "no errors");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ps_data_error), "max 2-bit");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ps_data_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_ps_data_error), conf.ps_data_error);
    gtk_table_attach(GTK_TABLE(table_rds), c_ps_data_error, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_psprog = gtk_check_button_new_with_label("PS progressive correction");
    gtk_widget_set_tooltip_text(x_psprog, "Replace characters in the RDS PS only with lower or the same error level. This also overrides the error correction to 5/5bit. Useful for static RDS PS. For a quick toggle, click a right mouse button on the displayed RDS PS.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_psprog), conf.rds_ps_progressive);
    gtk_table_attach(GTK_TABLE(table_rds), x_psprog, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    gtk_table_attach(GTK_TABLE(table_rds), gtk_hseparator_new(), 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rt_error = gtk_label_new("Radio Text error correction:");
    gtk_table_attach(GTK_TABLE(table_rds), l_rt_error, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rt_info_error = gtk_label_new("Group Type / Position:");
    gtk_misc_set_alignment(GTK_MISC(l_rt_info_error), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_rt_info_error, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *c_rt_info_error = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rt_info_error), "no errors");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rt_info_error), "max 2-bit");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rt_info_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_rt_info_error), conf.rt_info_error);
    gtk_table_attach(GTK_TABLE(table_rds), c_rt_info_error, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rt_data_error = gtk_label_new("Data:");
    gtk_misc_set_alignment(GTK_MISC(l_rt_data_error), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_rds), l_rt_data_error, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    GtkWidget *c_rt_data_error = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rt_data_error), "no errors");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rt_data_error), "max 2-bit");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_rt_data_error), "max 5-bit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_rt_data_error), conf.rt_data_error);
    gtk_table_attach(GTK_TABLE(table_rds), c_rt_data_error, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);


    /* Antenna page */
    page_ant = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_ant), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_ant, gtk_label_new("Antenna"));

    GtkWidget *table_ant = gtk_table_new(12, 2, FALSE);
    gtk_table_set_homogeneous(GTK_TABLE(table_ant), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_ant), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table_ant), 4);
    gtk_container_add(GTK_CONTAINER(page_ant), table_ant);


    row = 0;
    x_alignment = gtk_check_button_new_with_label("Show antenna input alignment");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_alignment), conf.alignment);
    gtk_table_attach(GTK_TABLE(table_ant), x_alignment, 0, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_swap_rotator = gtk_check_button_new_with_label("Swap antenna rotator buttons");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_swap_rotator), conf.swap_rotator);
    gtk_table_attach(GTK_TABLE(table_ant), x_swap_rotator, 0, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    gtk_table_attach(GTK_TABLE(table_ant), gtk_hseparator_new(), 0, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_count = gtk_label_new("Antenna switch count:");
    gtk_misc_set_alignment(GTK_MISC(l_count), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_ant), l_count, 0, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_ant_count = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ant_count), "hidden");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ant_count), "2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ant_count), "3");
    gtk_combo_box_append_text(GTK_COMBO_BOX(c_ant_count), "4");
    gtk_combo_box_set_active(GTK_COMBO_BOX(c_ant_count), conf.ant_count-1);
    gtk_table_attach(GTK_TABLE(table_ant), c_ant_count, 3, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_ant_clear_rds = gtk_check_button_new_with_label("Reset RDS data after switching");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_ant_clear_rds), conf.ant_clear_rds);
    gtk_table_attach(GTK_TABLE(table_ant), x_ant_clear_rds, 0, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    gtk_table_attach(GTK_TABLE(table_ant), gtk_hseparator_new(), 0, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_ant = gtk_check_button_new_with_label("Automatic antenna switching");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_ant), conf.ant_switching);
    gtk_table_attach(GTK_TABLE(table_ant), x_ant, 0, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    for(i=0; i<ANT_COUNT; i++)
    {
        row++;
        snprintf(tmp, sizeof(tmp), "Ant %c:", 'A'+i);
        gtk_table_attach(GTK_TABLE(table_ant), gtk_label_new(tmp), 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
        s_ant_start[i] = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.ant_start[i], 0.0, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
        gtk_table_attach(GTK_TABLE(table_ant), s_ant_start[i], 1, 2, row, row+1, 0, 0, 0, 0);
        gtk_table_attach(GTK_TABLE(table_ant), gtk_label_new("-"), 2, 3, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
        s_ant_stop[i] = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.ant_stop[i], 0.0, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
        gtk_table_attach(GTK_TABLE(table_ant), s_ant_stop[i], 3, 4, row, row+1, 0, 0, 0, 0);
        gtk_table_attach(GTK_TABLE(table_ant), gtk_label_new("kHz"), 4, 5, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    }


    /* Logs page */
    page_logs = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(page_logs), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_logs, gtk_label_new("Logs"));

    table_logs = gtk_table_new(14, 2, FALSE);
    gtk_table_set_homogeneous(GTK_TABLE(table_logs), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_logs), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table_logs), 4);
    gtk_container_add(GTK_CONTAINER(page_logs), table_logs);

    row = 0;
    GtkWidget *l_fmlist = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(l_fmlist), "See <a href=\"http://fmlist.org\">FMLIST</a>, the worldwide FM radio\n station list. Report to it what stations\n you receive at home and from far away.");
    gtk_label_set_justify(GTK_LABEL(l_fmlist), GTK_JUSTIFY_CENTER);
#ifdef G_OS_WIN32
    g_signal_connect(l_fmlist, "activate-link", G_CALLBACK(win32_uri), NULL);
#endif
    gtk_table_attach(GTK_TABLE(table_logs), l_fmlist, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    gtk_table_attach(GTK_TABLE(table_logs), gtk_hseparator_new(), 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rdsspy_port = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(l_rdsspy_port), "<a href=\"http://rdsspy.com/\">RDS Spy</a> ASCII G Port:");
#ifdef G_OS_WIN32
    g_signal_connect(l_rdsspy_port, "activate-link", G_CALLBACK(win32_uri), NULL);
#endif
    gtk_misc_set_alignment(GTK_MISC(l_rdsspy_port), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_logs), l_rdsspy_port, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *s_rdsspy_port = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.rds_spy_port, 1024.0, 65535.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(table_logs), s_rdsspy_port, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_rdsspy_auto = gtk_check_button_new_with_label("Enable RDS Spy server on startup");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rdsspy_auto), conf.rds_spy_auto);
    gtk_table_attach(GTK_TABLE(table_logs), x_rdsspy_auto, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *x_rdsspy_run = gtk_check_button_new_with_label("Automatic RDS Spy start (rdsspy.exe):");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rdsspy_run), conf.rds_spy_run);
    gtk_table_attach(GTK_TABLE(table_logs), x_rdsspy_run, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *c_rdsspy_command = gtk_file_chooser_button_new("RDS Spy Executable File", GTK_FILE_CHOOSER_ACTION_OPEN);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "RDS Spy Executable File (rdsspy.exe)");
    gtk_file_filter_add_pattern(filter, "rdsspy.exe");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(c_rdsspy_command), filter);
    GtkFileFilter* filter2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filter2, "All files");
    gtk_file_filter_add_pattern(filter2, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(c_rdsspy_command), filter2);
    if(strlen(conf.rds_spy_command))
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(c_rdsspy_command), conf.rds_spy_command);
    }
    gtk_table_attach(GTK_TABLE(table_logs), c_rdsspy_command, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_stationlist = gtk_check_button_new_with_label("Enable SRCP (StationList)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_stationlist), conf.stationlist);
    gtk_table_attach(GTK_TABLE(table_logs), x_stationlist, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    l_stationlist_port = gtk_label_new("SRCP Radio UDP Port:");
    gtk_misc_set_alignment(GTK_MISC(l_stationlist_port), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_logs), l_stationlist_port, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    s_stationlist_port = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.stationlist_port, 1025.0, 65535.0, 1.0, 10.0, 0.0)), 0, 0);
    gtk_table_attach(GTK_TABLE(table_logs), s_stationlist_port, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_rds_logging = gtk_check_button_new_with_label("Enable simple RDS logging");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_rds_logging), conf.rds_logging);
    gtk_table_attach(GTK_TABLE(table_logs), x_rds_logging, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    x_replace = gtk_check_button_new_with_label("Replace spaces with underscores");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(x_replace), conf.replace_spaces);
    gtk_table_attach(GTK_TABLE(table_logs), x_replace, 0, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_log_dir = gtk_label_new("Log directory:");
    gtk_misc_set_alignment(GTK_MISC(l_log_dir), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_logs), l_log_dir, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_log_dir = gtk_file_chooser_button_new("Log directory", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    if(strlen(conf.log_dir))
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(c_log_dir), conf.log_dir);
    }
    gtk_table_attach(GTK_TABLE(table_logs), c_log_dir, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_screen_dir = gtk_label_new("Screenshot directory:");
    gtk_misc_set_alignment(GTK_MISC(l_screen_dir), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_logs), l_screen_dir, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *c_screen_dir = gtk_file_chooser_button_new("Screenshot directory", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    if(strlen(conf.screen_dir))
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(c_screen_dir), conf.screen_dir);
    }
    gtk_table_attach(GTK_TABLE(table_logs), c_screen_dir, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    /* Keyboard page */
    page_key = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page_key), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_key, gtk_label_new("Keyboard"));

    GtkWidget *table_key = gtk_table_new(17, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table_key), 4);
    gtk_table_set_homogeneous(GTK_TABLE(table_key), FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table_key), 1);
    gtk_table_set_col_spacings(GTK_TABLE(table_key), 0);
    GtkWidget* page_key_viewport = gtk_viewport_new(NULL,NULL);
    gtk_container_add(GTK_CONTAINER(page_key_viewport), table_key);
    gtk_container_add(GTK_CONTAINER(page_key), page_key_viewport);

    row = 0;
    GtkWidget *l_tune_up = gtk_label_new("Tune up");
    gtk_misc_set_alignment(GTK_MISC(l_tune_up), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_up, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_up = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_up));
    g_signal_connect(b_tune_up, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_up, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_tune_down = gtk_label_new("Tune down");
    gtk_misc_set_alignment(GTK_MISC(l_tune_down), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_down, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_down = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_down));
    g_signal_connect(b_tune_down, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_down, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_tune_up_5 = gtk_label_new("Fine tune up");
    gtk_misc_set_alignment(GTK_MISC(l_tune_up_5), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_up_5, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_up_5 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_up_5));
    g_signal_connect(b_tune_up_5, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_up_5, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_tune_down_5 = gtk_label_new("Fine tune down");
    gtk_misc_set_alignment(GTK_MISC(l_tune_down_5), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_down_5, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_down_5 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_down_5));
    g_signal_connect(b_tune_down_5, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_down_5, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_tune_up_1000 = gtk_label_new("Tune +1 MHz");
    gtk_misc_set_alignment(GTK_MISC(l_tune_up_1000), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_up_1000, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_up_1000 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_up_1000));
    g_signal_connect(b_tune_up_1000, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_up_1000, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_tune_down_1000 = gtk_label_new("Tune -1 MHz");
    gtk_misc_set_alignment(GTK_MISC(l_tune_down_1000), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_down_1000, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_down_1000 = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_down_1000));
    g_signal_connect(b_tune_down_1000, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_down_1000, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_tune_back = gtk_label_new("Tune back");
    gtk_misc_set_alignment(GTK_MISC(l_tune_back), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_tune_back, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_tune_back = gtk_button_new_with_label(gdk_keyval_name(conf.key_tune_back));
    g_signal_connect(b_tune_back, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_tune_back, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_reset = gtk_label_new("Reset frequency");
    gtk_misc_set_alignment(GTK_MISC(l_reset), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_reset, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_reset = gtk_button_new_with_label(gdk_keyval_name(conf.key_reset));
    g_signal_connect(b_reset, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_reset, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_screen = gtk_label_new("Screenshot");
    gtk_misc_set_alignment(GTK_MISC(l_screen), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_screen, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_screen = gtk_button_new_with_label(gdk_keyval_name(conf.key_screen));
    g_signal_connect(b_screen, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_screen, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_bw_up = gtk_label_new("Increase bandwidth");
    gtk_misc_set_alignment(GTK_MISC(l_bw_up), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_bw_up, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_bw_up = gtk_button_new_with_label(gdk_keyval_name(conf.key_bw_up));
    g_signal_connect(b_bw_up, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_bw_up, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_bw_down = gtk_label_new("Decrease bandwidth");
    gtk_misc_set_alignment(GTK_MISC(l_bw_down), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_bw_down, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_bw_down = gtk_button_new_with_label(gdk_keyval_name(conf.key_bw_down));
    g_signal_connect(b_bw_down, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_bw_down, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_bw_auto = gtk_label_new("Adaptive bandwidth");
    gtk_misc_set_alignment(GTK_MISC(l_bw_auto), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_bw_auto, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_bw_auto = gtk_button_new_with_label(gdk_keyval_name(conf.key_bw_auto));
    g_signal_connect(b_bw_auto, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_bw_auto, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rotate_cw = gtk_label_new("Rotate CW");
    gtk_misc_set_alignment(GTK_MISC(l_rotate_cw), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_rotate_cw, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_rotate_cw = gtk_button_new_with_label(gdk_keyval_name(conf.key_rotate_cw));
    g_signal_connect(b_rotate_cw, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_rotate_cw, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_rotate_ccw = gtk_label_new("Rotate CCW");
    gtk_misc_set_alignment(GTK_MISC(l_rotate_ccw), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_rotate_ccw, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_rotate_ccw = gtk_button_new_with_label(gdk_keyval_name(conf.key_rotate_ccw));
    g_signal_connect(b_rotate_ccw, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_rotate_ccw, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_switch_ant = gtk_label_new("Switch antenna");
    gtk_misc_set_alignment(GTK_MISC(l_switch_ant), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_switch_ant, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_switch_ant = gtk_button_new_with_label(gdk_keyval_name(conf.key_switch_ant));
    g_signal_connect(b_switch_ant, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_switch_ant, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_key_ps_mode = gtk_label_new("Toggle RDS PS mode");
    gtk_misc_set_alignment(GTK_MISC(l_key_ps_mode), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_key_ps_mode, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_key_ps_mode = gtk_button_new_with_label(gdk_keyval_name(conf.key_ps_mode));
    g_signal_connect(b_key_ps_mode, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_key_ps_mode, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

    row++;
    GtkWidget *l_spectral_toggle = gtk_label_new("Start/stop spectral scan");
    gtk_misc_set_alignment(GTK_MISC(l_spectral_toggle), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table_key), l_spectral_toggle, 0, 1, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
    GtkWidget *b_key_spectral_toggle = gtk_button_new_with_label(gdk_keyval_name(conf.key_spectral_toggle));
    g_signal_connect(b_key_spectral_toggle, "clicked", G_CALLBACK(settings_key), NULL);
    gtk_table_attach(GTK_TABLE(table_key), b_key_spectral_toggle, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0, 0, 0);


    /* Presets page */
    page_presets = gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_width(GTK_CONTAINER(page_presets), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_presets, gtk_label_new("Presets"));

    GtkWidget* presets_wrapper = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page_presets), presets_wrapper, TRUE, TRUE, 0);

    table_presets = gtk_table_new(PRESETS/2, 4, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table_presets), 3);
    gtk_table_set_row_spacings(GTK_TABLE(table_presets), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table_presets), 4);
    gtk_table_set_homogeneous(GTK_TABLE(table_presets), FALSE);
    gtk_box_pack_start(GTK_BOX(presets_wrapper), table_presets, TRUE, FALSE, 0);

    for(i=0; i<PRESETS; i++)
    {
        g_snprintf(tmp, sizeof(tmp), "F%d:", i+1);
        gtk_table_attach(GTK_TABLE(table_presets), gtk_label_new(tmp), ((i>=6)?2:0), ((i>=6)?3:1), i%6, i%6+1, 0, 0, 0, 0);
        s_presets[i] = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.presets[i], TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
        gtk_table_attach(GTK_TABLE(table_presets), s_presets[i], ((i>=6)?3:1), ((i>=6)?4:2), i%6, i%6+1, 0, 0, 0, 0);
    }

    GtkWidget *l_presets_unit = gtk_label_new("All frequencies are in kHz.");
    gtk_label_set_justify(GTK_LABEL(l_presets_unit), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(page_presets), l_presets_unit, TRUE, FALSE, 0);

    GtkWidget *l_presets_info = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(l_presets_info), "Tune to a frequency - press <b>F1</b>...<b>F12</b>.\nStore a frequency - <b>Shift+F1</b>...<b>F12</b>.");
    gtk_label_set_justify(GTK_LABEL(l_presets_info), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(page_presets), l_presets_info, TRUE, FALSE, 0);


    /* Scheduler page */
    page_scheduler = gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_width(GTK_CONTAINER(page_scheduler), 4);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_scheduler, gtk_label_new("Scheduler"));

    scheduler_liststore = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_INT);
    scheduler_treeview = gtk_tree_view_new();
    scheduler_renderer_freq = gtk_cell_renderer_text_new();
    scheduler_renderer_timeout = gtk_cell_renderer_text_new();
    g_object_set(scheduler_renderer_freq, "editable", TRUE, NULL);
    g_object_set(scheduler_renderer_timeout, "editable", TRUE, NULL);
    g_signal_connect(scheduler_renderer_freq, "edited", G_CALLBACK(settings_scheduler_edit), GINT_TO_POINTER(SCHEDULER_LIST_STORE_FREQ));
    g_signal_connect(scheduler_renderer_timeout, "edited", G_CALLBACK(settings_scheduler_edit), GINT_TO_POINTER(SCHEDULER_LIST_STORE_TIMEOUT));
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(scheduler_treeview), -1, "Freq [kHz]", scheduler_renderer_freq, "text", SCHEDULER_LIST_STORE_FREQ, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(scheduler_treeview), -1, "Time [s]", scheduler_renderer_timeout, "text", SCHEDULER_LIST_STORE_TIMEOUT, NULL);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(scheduler_treeview), TRUE);
    g_signal_connect(scheduler_treeview, "key-press-event", G_CALLBACK(settings_scheduler_key), NULL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(scheduler_treeview), GTK_TREE_MODEL(scheduler_liststore));
    g_object_unref(scheduler_liststore);
    scheduler_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scheduler_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scheduler_scroll), scheduler_treeview);
    gtk_box_pack_start(GTK_BOX(page_scheduler), scheduler_scroll, TRUE, TRUE, 0);

    scheduler_add_box = gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(page_scheduler), scheduler_add_box, FALSE, FALSE, 0);

    s_scheduler_freq = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(87500.0, TUNER_FREQ_MIN, TUNER_FREQ_MAX, 100.0, 200.0, 0.0)), 0, 0);
    gtk_widget_set_tooltip_text(s_scheduler_freq, "Frequency [kHz]");
    g_signal_connect(s_scheduler_freq, "key-press-event", G_CALLBACK(settings_scheduler_add_key), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_add_box), s_scheduler_freq, FALSE, FALSE, 0);

    s_scheduler_timeout = gtk_spin_button_new(GTK_ADJUSTMENT(gtk_adjustment_new(conf.scheduler_default_timeout, 1.0, 99999.0, 1.0, 2.0, 0.0)), 0, 0);
    gtk_widget_set_tooltip_text(s_scheduler_timeout, "Time [s]");
    g_signal_connect(s_scheduler_timeout, "key-press-event", G_CALLBACK(settings_scheduler_add_key), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_add_box), s_scheduler_timeout, FALSE, FALSE, 0);

    b_scheduler_add = gtk_button_new();
    gtk_widget_set_tooltip_text(b_scheduler_add, "Add");
    gtk_button_set_image(GTK_BUTTON(b_scheduler_add), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(b_scheduler_add, "clicked", G_CALLBACK(settings_scheduler_add), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_add_box), b_scheduler_add, FALSE, FALSE, 0);

    scheduler_right_align = gtk_alignment_new(1.0, 1.0, 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(scheduler_add_box), scheduler_right_align, TRUE, TRUE, 0);

    scheduler_right_box = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(scheduler_right_align), scheduler_right_box);

    b_scheduler_remove = gtk_button_new();
    gtk_widget_set_tooltip_text(b_scheduler_remove, "Remove");
    gtk_button_set_image(GTK_BUTTON(b_scheduler_remove), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(b_scheduler_remove, "clicked", G_CALLBACK(settings_scheduler_remove), NULL);
    gtk_box_pack_start(GTK_BOX(scheduler_right_box), b_scheduler_remove, FALSE, FALSE, 0);

    b_scheduler_clear = gtk_button_new_with_label("Clear");
    gtk_button_set_image(GTK_BUTTON(b_scheduler_clear), gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON));
    g_signal_connect_swapped(b_scheduler_clear, "clicked", G_CALLBACK(gtk_list_store_clear), scheduler_liststore);
    gtk_box_pack_start(GTK_BOX(scheduler_right_box), b_scheduler_clear, FALSE, FALSE, 0);

    settings_scheduler_load();


    /* About page */
    page_about = gtk_vbox_new(FALSE, 2);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_about, gtk_label_new("About..."));

    GtkWidget *img = gtk_image_new_from_icon_name("xdr-gtk", GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(page_about), img, TRUE, FALSE, 10);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_CENTER);
    gchar* msg = g_markup_printf_escaped("<span size=\"xx-large\"><b>%s</b></span>\n<span size=\"x-large\"><b>%s</b></span>", APP_NAME, APP_VERSION);
    gtk_label_set_markup(GTK_LABEL(title), msg);
    g_free(msg);
    gtk_box_pack_start(GTK_BOX(page_about), title, TRUE, FALSE, 0);

    GtkWidget *info = gtk_label_new(NULL);
    gtk_label_set_justify(GTK_LABEL(info), GTK_JUSTIFY_CENTER);
    gtk_label_set_markup(GTK_LABEL(info), "User interface for the XDR-F1HD Radio\nTuner with <a href=\"http://fmdx.pl/xdr-i2c\">XDR-I2C</a> modification.");
#ifdef G_OS_WIN32
    g_signal_connect(info, "activate-link", G_CALLBACK(win32_uri), NULL);
#endif
    gtk_box_pack_start(GTK_BOX(page_about), info, TRUE, FALSE, 5);

    GtkWidget *info2 = gtk_label_new(NULL);
    gtk_label_set_justify(GTK_LABEL(info2), GTK_JUSTIFY_CENTER);
    gtk_label_set_markup(GTK_LABEL(info2), "Written in C/GTK+ for GNU/Linux\nand Windows operating systems.");
    gtk_box_pack_start(GTK_BOX(page_about), info2, TRUE, FALSE, 5);

    GtkWidget *copyright = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(copyright), "<span size=\"small\">Copyright (C) 2012-2015  Konrad Kosmatka</span>");
    gtk_box_pack_start(GTK_BOX(page_about), copyright, TRUE, FALSE, 5);

    GtkWidget *link = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(link), "<a href=\"http://fmdx.pl/xdr-gtk/\">http://fmdx.pl/xdr-gtk/</a>");
#ifdef G_OS_WIN32
    g_signal_connect(link, "activate-link", G_CALLBACK(win32_uri), NULL);
#endif
    gtk_box_pack_start(GTK_BOX(page_about), link, TRUE, FALSE, 10);

    /* HACK: the same background color in the Keyboard page */
    gtk_widget_realize(notebook);
    gtk_widget_modify_bg(page_key_viewport, GTK_STATE_NORMAL, &gtk_widget_get_style(notebook)->bg[GTK_STATE_NORMAL]);

    gtk_widget_show_all(dialog);
#ifdef G_OS_WIN32
    result = win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    result = gtk_dialog_run(GTK_DIALOG(dialog));
#endif
    if(result != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    /* Interface page */
    conf.init_freq = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_init_freq));
//    conf.signal_offset = gtk_spin_button_get_value(GTK_SPIN_BUTTON(s_signal_offset));
    conf.signal_unit = gtk_combo_box_get_active(GTK_COMBO_BOX(c_unit));
    conf.event_action = gtk_combo_box_get_active(GTK_COMBO_BOX(c_event));
    conf.utc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_utc));
    conf.autoconnect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_autoconnect));
    conf.amstep = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_amstep));
    conf.disconnect_confirm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_disconnect_confirm));
    conf.auto_reconnect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_auto_reconnect));
    conf.hide_decorations = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_hide_decorations));
    conf.hide_status = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_hide_status));
    conf.restore_pos = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_restore_pos));

    /* Graph page */
    conf.signal_display = gtk_combo_box_get_active(GTK_COMBO_BOX(c_display));
    conf.graph_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(c_graph_mode));
    gtk_color_button_get_color(GTK_COLOR_BUTTON(c_mono), &conf.color_mono);
    gtk_color_button_get_color(GTK_COLOR_BUTTON(c_stereo), &conf.color_stereo);
    gtk_color_button_get_color(GTK_COLOR_BUTTON(c_rds), &conf.color_rds);
    conf.graph_height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_height));
    conf.signal_avg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_avg));
    conf.show_grid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_grid));

    /* RDS page */
    conf.rds_pty = gtk_combo_box_get_active(GTK_COMBO_BOX(c_pty));
    conf.rds_reset = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rds_reset));
    conf.rds_reset_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_reset));
    conf.ps_info_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_ps_info_error));
    conf.ps_data_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_ps_data_error));
    conf.rds_ps_progressive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_psprog));
    conf.rt_info_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_rt_info_error));
    conf.rt_data_error = gtk_combo_box_get_active(GTK_COMBO_BOX(c_rt_data_error));

    /* Antenna page */
    conf.alignment = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_alignment));
    conf.swap_rotator = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_swap_rotator));
    conf.ant_count = gtk_combo_box_get_active(GTK_COMBO_BOX(c_ant_count))+1;
    conf.ant_clear_rds = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_ant_clear_rds));
    conf.ant_switching = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_ant));
    for(i=0; i<ANT_COUNT; i++)
    {
        conf.ant_start[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_ant_start[i]));
        conf.ant_stop[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_ant_stop[i]));
    }

    /* Logs page */
    i = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_rdsspy_port));
    if(rdsspy_is_up() && conf.rds_spy_port != i)
    {
        rdsspy_stop();
    }
    conf.rds_spy_port = i;
    conf.rds_spy_auto = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rdsspy_auto));
    conf.rds_spy_run = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rdsspy_run));
    g_free(conf.rds_spy_command);
    conf.rds_spy_command = settings_read_string(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(c_rdsspy_command)));
    conf.stationlist = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_stationlist));
    conf.stationlist_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_stationlist_port));
    stationlist_stop();
    if(conf.stationlist)
    {
        stationlist_init();
    }
    if(conf.rds_logging)
    {
        log_cleanup();
    }
    conf.rds_logging = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_rds_logging));
    conf.replace_spaces = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(x_replace));
    g_free(conf.log_dir);
    conf.log_dir = settings_read_string(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(c_log_dir)));
    g_free(conf.screen_dir);
    conf.screen_dir = settings_read_string(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(c_screen_dir)));

    /* Keyboard page */
    conf.key_tune_up = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_up)));
    conf.key_tune_down = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_down)));
    conf.key_tune_up_5 = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_up_5)));
    conf.key_tune_down_5 = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_down_5)));
    conf.key_tune_up_1000 = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_up_1000)));
    conf.key_tune_down_1000 = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_down_1000)));
    conf.key_tune_back = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_tune_back)));
    conf.key_reset = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_reset)));
    conf.key_screen = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_screen)));
    conf.key_bw_up = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_bw_up)));
    conf.key_bw_down = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_bw_down)));
    conf.key_bw_auto = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_bw_auto)));
    conf.key_rotate_cw = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_rotate_cw)));
    conf.key_rotate_ccw = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_rotate_ccw)));
    conf.key_switch_ant = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_switch_ant)));
    conf.key_ps_mode = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_key_ps_mode)));
    conf.key_spectral_toggle = gdk_keyval_from_name(gtk_button_get_label(GTK_BUTTON(b_key_spectral_toggle)));

    /* Presets page */
    for(i=0; i<PRESETS; i++)
    {
        conf.presets[i] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_presets[i]));
    }

    /* Scheduler page */
    settings_scheduler_store();
    scheduler_stop();
    conf.scheduler_default_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_scheduler_timeout));


    settings_write();
    graph_resize();
    signal_display();
    gui_antenna_showhide();
    gui_rotator_button_swap();

    if(conf.alignment)
    {
        gtk_widget_show(gui.hs_align);
    }
    else
    {
        gtk_widget_hide(gui.hs_align);
    }
    if(conf.hide_status)
    {
        gtk_widget_hide(gui.l_status);
    }
    else
    {
        gtk_widget_show(gui.l_status);
    }

    gtk_window_set_decorated(GTK_WINDOW(gui.window), !conf.hide_decorations);
    gtk_window_set_resizable(GTK_WINDOW(gui.window), FALSE);
    gtk_widget_modify_bg(gui.frame, GTK_STATE_NORMAL, (conf.hide_decorations?&gui.colors.grey:&gui.colors.background));
    gtk_widget_destroy(dialog);
}

void settings_key(GtkWidget *widget, GdkEventButton *event, gpointer nothing)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Key", GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    g_signal_connect(dialog, "key-press-event", G_CALLBACK(settings_key_press), widget);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 7);

    gchar *text = g_markup_printf_escaped("Current key: <b>%s</b>\n\nPress any key to change...\n", gtk_button_get_label(GTK_BUTTON(widget)));
    GtkWidget *desc = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(desc), text);
    gtk_container_add(GTK_CONTAINER(content), desc);
    g_free(text);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
}

gboolean settings_key_press(GtkWidget* widget, GdkEventKey* event, gpointer button)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    gtk_button_set_label(GTK_BUTTON(button), gdk_keyval_name(current));
    gtk_widget_destroy(widget);
    return TRUE;
}

void settings_update_string(gchar** ptr, const gchar* str)
{
    if(*ptr != NULL)
    {
        g_free(*ptr);
    }

    *ptr = g_strdup((str ? str : ""));
}

gchar* settings_read_string(gchar* str)
{
    return str ? str : g_strdup("");
}

void settings_add_host(const gchar* host)
{
    gchar** hosts;
    gint i, j;

    if(host == NULL)
        return;

    hosts = g_new0(char*, HOST_HISTORY_LEN+1);
    hosts[0] = g_strdup(host);

    for(i=0, j=1; (j<HOST_HISTORY_LEN && conf.host[i]); i++)
    {
        if(g_ascii_strcasecmp(host, conf.host[i]))
        {
            hosts[j++] = g_strdup(conf.host[i]);
        }
    }
    g_strfreev(conf.host);
    conf.host = hosts;
}

void settings_scheduler_load()
{
    GtkTreeIter iter;
    int i;
    for(i=0; i<conf.scheduler_n; i++)
    {
        gtk_list_store_append(scheduler_liststore, &iter);
        gtk_list_store_set(scheduler_liststore, &iter, SCHEDULER_LIST_STORE_FREQ, conf.scheduler_freqs[i], SCHEDULER_LIST_STORE_TIMEOUT, conf.scheduler_timeouts[i], -1);
    }
}

void settings_scheduler_store()
{
    gint i;
    g_free(conf.scheduler_freqs);
    g_free(conf.scheduler_timeouts);
    conf.scheduler_n = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(scheduler_liststore), NULL);
    if(!conf.scheduler_n)
    {
        conf.scheduler_freqs = NULL;
        conf.scheduler_timeouts = NULL;
        return;
    }
    conf.scheduler_freqs = g_new(gint, conf.scheduler_n);
    conf.scheduler_timeouts = g_new(gint, conf.scheduler_n);
    i = 0;
    gtk_tree_model_foreach(GTK_TREE_MODEL(scheduler_liststore), (GtkTreeModelForeachFunc)settings_scheduler_store_foreach, &i);
}

gboolean settings_scheduler_store_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer* data)
{
    gint *i = (gint*)data;
    gtk_tree_model_get(model, iter, SCHEDULER_LIST_STORE_FREQ, &conf.scheduler_freqs[*i], SCHEDULER_LIST_STORE_TIMEOUT, &conf.scheduler_timeouts[*i], -1);
    (*i)++;
    return FALSE;
}

void settings_scheduler_add(GtkWidget *widget, gpointer nothing)
{
    GtkTreeIter iter;
    gint freq = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_scheduler_freq));
    gint timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s_scheduler_timeout));
    gtk_list_store_append(scheduler_liststore, &iter);
    gtk_list_store_set(scheduler_liststore, &iter, SCHEDULER_LIST_STORE_FREQ, freq, SCHEDULER_LIST_STORE_TIMEOUT, timeout, -1);
}

void settings_scheduler_edit(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer col)
{
    GtkTreeIter iter;
    gint new_value = atoi(new_text);
    gint column = GPOINTER_TO_INT(col);

    if(column == SCHEDULER_LIST_STORE_FREQ && (new_value < TUNER_FREQ_MIN || new_value > TUNER_FREQ_MAX))
    {
        return;
    }

    if(column == SCHEDULER_LIST_STORE_TIMEOUT && new_value < 1)
    {
        return;
    }

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(scheduler_liststore), &iter, path_string);
    gtk_list_store_set(scheduler_liststore, &iter, col, new_value, -1);
}

void settings_scheduler_remove(GtkWidget* widget, gpointer nothing)
{
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(scheduler_treeview));
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    }
}

gboolean settings_scheduler_key(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
    if(event->keyval == GDK_Delete)
    {
        gtk_button_clicked(GTK_BUTTON(b_scheduler_remove));
        return TRUE;
    }
    return FALSE;
}

gboolean settings_scheduler_add_key(GtkWidget* widget, GdkEventKey* event, gpointer button)
{
    if(event->keyval == GDK_Return)
    {
        /* Let the spin button update its value before adding an entry */
        g_idle_add(settings_scheduler_add_key_idle, b_scheduler_add);
    }
    return FALSE;
}

gboolean settings_scheduler_add_key_idle(gpointer ptr)
{
    gtk_button_clicked(GTK_BUTTON(ptr));
    return FALSE;
}

void settings_scan_mark_add(gint frequency)
{
    gpointer value = GINT_TO_POINTER(frequency);
    if(!g_list_find(conf.scan_freq_list, value))
    {
        conf.scan_freq_list = g_list_prepend(conf.scan_freq_list, value);
    }
}

void settings_scan_mark_toggle(gint frequency)
{
    gpointer value = GINT_TO_POINTER(frequency);
    GList* ptr = g_list_find(conf.scan_freq_list, value);
    if(ptr)
    {
        conf.scan_freq_list = g_list_delete_link(conf.scan_freq_list, ptr);
    }
    else
    {
        conf.scan_freq_list = g_list_prepend(conf.scan_freq_list, value);
    }
}

void settings_scan_mark_remove(gint frequency)
{
    gpointer value = GINT_TO_POINTER(frequency);
    conf.scan_freq_list = g_list_remove(conf.scan_freq_list, value);
}

void settings_scan_mark_clear(gint from, gint to)
{
    GList* ptr = conf.scan_freq_list;
    gint freq;

    while(ptr)
    {
        GList *next = ptr->next;
        freq = GPOINTER_TO_INT(ptr->data);
        if(freq >= from && freq <= to)
        {
            conf.scan_freq_list = g_list_delete_link(conf.scan_freq_list, ptr);
        }
        ptr = next;
    }
}


void settings_scan_mark_clear_all()
{
    g_list_free(conf.scan_freq_list);
    conf.scan_freq_list = NULL;
}
