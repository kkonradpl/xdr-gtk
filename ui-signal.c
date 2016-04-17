#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include "ui.h"
#include "tuner.h"
#include "conf.h"
#include "ui-signal.h"

#define GRAPH_FONT_SIZE  12

#define GRAPH_OFFSET_LEFT 23
#define GRAPH_OFFSET_TOP  (GRAPH_FONT_SIZE/2)
#define GRAPH_OFFSET_DBM  7

#define GRAPH_SCALE_LINE_LENGTH 5

typedef struct signal_sample
{
    gfloat value;
    gboolean rds;
    gboolean stereo;
    gint freq;
} signal_sample_t;

typedef struct signal_buffer
{
    signal_sample_t *data;
    gint len;
    gint pos;
} signal_buffer_t;

signal_buffer_t s;

static GdkColor graph_color_border;
static GdkColor graph_color_scale;
static GdkColor graph_color_grid;
static GdkColor graph_color_font;

static const double grid_pattern[] = {1.0, 1.0};
static gint grid_pattern_len = sizeof(grid_pattern) / sizeof(grid_pattern[0]);

static gboolean graph_draw(GtkWidget*, GdkEventExpose*, gpointer);
static gboolean graph_click(GtkWidget*, GdkEventButton*, gpointer);

void
signal_init()
{
    gdk_color_parse("#646464", &graph_color_border);
    gdk_color_parse("#AAAAAA", &graph_color_scale);
    gdk_color_parse("#C8C8C8", &graph_color_grid);
    gdk_color_parse("#000000", &graph_color_font);

    s.len = (ui.graph->allocation.width)-GRAPH_OFFSET_LEFT;
    s.data = g_new(signal_sample_t, s.len);

    gtk_widget_add_events(ui.graph, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(ui.graph, "expose-event", G_CALLBACK(graph_draw), NULL);
    g_signal_connect(ui.graph, "button-press-event", G_CALLBACK(graph_click), NULL);

    signal_resize();
    signal_clear();
    signal_display();
}

void
signal_resize()
{
    gtk_widget_set_size_request(ui.graph, -1, conf.signal_height+2*GRAPH_OFFSET_TOP);
}

static gboolean
graph_draw(GtkWidget      *widget,
           GdkEventExpose *event,
           gpointer        nothing)
{
    cairo_t *cr;
    gdouble step;
    gint current_value;
    gdouble position;
    gdouble last_position;
    gint max = G_MININT;
    gint min = G_MAXINT;
    gint i, curr, prev, next;
    gdouble value;
    gchar text[10];
    gint offset_left = GRAPH_OFFSET_LEFT + (conf.signal_unit == UNIT_DBM ? GRAPH_OFFSET_DBM : 0);
    gint draw_count = widget->allocation.width - offset_left;

    if(draw_count > s.len)
        draw_count = s.len;

    curr = s.pos;
    for(i=0; i<draw_count; i++)
    {
        if(isnan(s.data[curr].value))
            continue;
        if(signal_level(s.data[curr].value) > max)
            max = ceil(signal_level(s.data[curr].value));
        if(signal_level(s.data[curr].value) < min)
            min = floor(signal_level(s.data[curr].value));
        curr = ((curr==0) ? (s.len-1) : curr-1);
    }

    cr = gdk_cairo_create(widget->window);
    gdk_cairo_set_source_color(cr, &ui.colors.background);
    cairo_paint(cr);

    if(max == G_MININT)
    {
        cairo_destroy(cr);
        return FALSE;
    }

    if((max - min) == 0)
        max++;
    if((max - min) == 1)
        min--;

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1.0);
    cairo_select_font_face(cr, "DejaVu Sans Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, GRAPH_FONT_SIZE);

    step = conf.signal_height / (gdouble)(max - min);
    current_value = max;
    position = GRAPH_OFFSET_TOP+0.5;
    last_position = GRAPH_FONT_SIZE * -1.0;
    while(position <= GRAPH_OFFSET_TOP+conf.signal_height+1.0)
    {
        if(last_position + GRAPH_FONT_SIZE + 1.0 < position)
        {
            /* Draw a tick */
            gdk_cairo_set_source_color(cr, &graph_color_scale);
            cairo_move_to(cr, offset_left-0.5-GRAPH_SCALE_LINE_LENGTH/2.0, position);
            cairo_line_to(cr, offset_left-0.5+GRAPH_SCALE_LINE_LENGTH/2.0, position);
            cairo_stroke(cr);

            /* Draw a grid */
            if(conf.signal_grid && current_value != min)
            {
                cairo_save(cr);
                gdk_cairo_set_source_color(cr, &graph_color_grid);
                cairo_set_dash(cr, grid_pattern, grid_pattern_len, 0);
                cairo_move_to(cr, offset_left-0.5+GRAPH_SCALE_LINE_LENGTH/2.0, position);
                cairo_line_to(cr, offset_left-0.5+s.len+0.5, position);
                cairo_stroke(cr);
                cairo_restore(cr);
            }

            /* Draw a label */
            gdk_cairo_set_source_color(cr, &graph_color_font);
            g_snprintf(text,
                       sizeof(text),
                       (conf.signal_unit == UNIT_DBM) ? "%4d" : "%3d",
                       current_value);
            cairo_move_to(cr, -1.5, position+GRAPH_FONT_SIZE/2.0-1.0);
            cairo_show_text(cr, text);

            last_position = position;
        }
        current_value--;
        position += step;
    }

    curr = s.pos;
    for(i=draw_count; i >= 1; i--)
    {
        prev = ((curr==0) ? (s.len-1) : curr-1);
        next = ((curr==(s.len-1)) ? 0 : curr+1);
        if(!isnan(s.data[curr].value))
        {
            if(s.data[curr].rds)
                gdk_cairo_set_source_color(cr, &conf.color_rds);
            else if(s.data[curr].stereo)
                gdk_cairo_set_source_color(cr, &conf.color_stereo);
            else
                gdk_cairo_set_source_color(cr, &conf.color_mono);

            if(conf.signal_avg)
            {
                if(i == 1 || isnan(s.data[prev].value) || s.data[curr].freq != s.data[prev].freq)
                {
                    if(isnan(s.data[next].value) || s.data[curr].freq != s.data[next].freq)
                        value = s.data[curr].value;
                    else
                        value = (s.data[curr].value + s.data[next].value) / 2.0;
                }
                else if(i == draw_count || isnan(s.data[next].value) || s.data[curr].freq != s.data[next].freq)
                {
                    if(isnan(s.data[prev].value) || s.data[curr].freq != s.data[prev].freq)
                        value = s.data[curr].value;
                    else
                        value = (s.data[curr].value + s.data[prev].value) / 2.0;
                }
                else
                {
                    value = (s.data[prev].value + s.data[curr].value + s.data[next].value) / 3.0;
                }
            }
            else
            {
                value = s.data[curr].value;
            }
            cairo_move_to(cr, offset_left+i, GRAPH_OFFSET_TOP+conf.signal_height);
            cairo_line_to(cr, offset_left+i, GRAPH_OFFSET_TOP+conf.signal_height-(signal_level(value) - min)*step);
            cairo_stroke(cr);
        }
        curr = prev;
    }

    /* Draw left vertical and bottom horizontal line */
    gdk_cairo_set_source_color(cr, &graph_color_border);
    cairo_move_to(cr, offset_left-0.5, GRAPH_OFFSET_TOP+0.5);
    cairo_line_to(cr, offset_left-0.5, GRAPH_OFFSET_TOP+conf.signal_height+0.5);
    cairo_line_to(cr, offset_left-0.5+s.len+0.5, GRAPH_OFFSET_TOP+conf.signal_height+0.5);
    cairo_stroke(cr);

    cairo_destroy(cr);
    return FALSE;
}

static gboolean
graph_click(GtkWidget      *widget,
            GdkEventButton *event,
            gpointer        nothing)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) /* right click */
        signal_clear();

    return FALSE;
}

void
signal_push(gfloat   value,
            gboolean stereo,
            gboolean rds,
            gboolean freq)
{
    s.pos = (s.pos + 1)%s.len;
    s.data[s.pos].value = value;
    s.data[s.pos].stereo = stereo;
    s.data[s.pos].rds = rds;
    s.data[s.pos].freq = freq;
}

void
signal_clear()
{
    gint i;
    s.pos = 0;
    for(i=0; i<s.len; i++)
        s.data[i].value = NAN;
}

gfloat
signal_level(gfloat value)
{
    if(tuner.mode == MODE_FM)
    {
        switch(conf.signal_unit)
        {
        case UNIT_DBM:
            return value - 120 + conf.signal_offset;

        case UNIT_DBUV:
            return value - 11.25 + conf.signal_offset;

        default:
        case UNIT_DBF:
            return value + conf.signal_offset;
        }
    }
    return value;
}

const gchar*
signal_unit()
{
    static const gchar dBuV[] = "dBµV";
    static const gchar dBf[] = "dBf";
    static const gchar dBm[] = "dBm";
    static const gchar unknown[] = "";

    if(tuner.mode == MODE_FM)
    {
        switch(conf.signal_unit)
        {
        case UNIT_DBM:
            return dBm;

        case UNIT_DBUV:
            return dBuV;

        default:
        case UNIT_DBF:
            return dBf;
        }
    }
    return unknown;
}

void
signal_display()
{
    switch(conf.signal_display)
    {
    case SIGNAL_GRAPH:
        gtk_widget_hide(ui.p_signal);
        gtk_widget_show(ui.graph);
        break;

    case SIGNAL_BAR:
        gtk_widget_show(ui.p_signal);
        gtk_widget_hide(ui.graph);
        break;

    default:
        gtk_widget_hide(ui.p_signal);
        gtk_widget_hide(ui.graph);
        break;
    }
}
