#ifndef XDR_SETTINGS_H_
#define XDR_SETTINGS_H_
#include <gtk/gtk.h>

#define CONF_FILE "xdr-gtk.conf"
#define PRESETS 12
#define ANTENNAS 4

enum Mode {MODE_FM, MODE_AM};
enum Signal {SIGNAL_NONE, SIGNAL_GRAPH, SIGNAL_BAR};
enum Unit {UNIT_DBF, UNIT_DBM, UNIT_DBUV, UNIT_S};

typedef struct
{
    gint network; // 0=serial, 1=net
    gchar* serial;
    gchar* host;
    unsigned short port;

    enum Mode mode;
    gboolean rfgain;
    gboolean ifgain;
    gint agc;
    gint deemphasis;

    gint presets[PRESETS];
    enum Signal signal_display;
    enum Unit signal_unit;
    gint graph_height;
    GdkColor color_mono;
    GdkColor color_stereo;
    GdkColor color_rds;
    gboolean signal_avg;
    gboolean show_grid;
    gboolean utc;
    gboolean replace_spaces;

    gint rds_timeout;
    gboolean rds_discard;
    gint rds_pty; // 0=europe, 1=usa
    gint rds_info_error;
    gint rds_data_error;
    // 0=no errors
    // 1=max 2 bit err. corr.
    // 2=max 5 bit err. corr.

    gint scan_width;
    gint scan_height;
    gint scan_start;
    gint scan_end;
    gint scan_step;
    gint scan_bw;
    gboolean scan_relative;

    gboolean ant_switching;
    gint ant_start[ANTENNAS];
    gint ant_stop[ANTENNAS];

    gint pattern_size;
    gboolean pattern_fill;
    gboolean pattern_avg;
} settings;

settings conf;

void settings_read();
void settings_write();
void settings_dialog();

#endif
