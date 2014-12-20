#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include "settings.h"
#include "gui.h"
#include "graph.h"
#include "tuner.h"
#include "sig.h"

GdkColor graph_color_border;
GdkColor graph_color_scale;
GdkColor graph_color_grid;
GdkColor graph_color_font;

const double grid_pattern[] = {1.0, 1.0};
gint grid_pattern_len = sizeof(grid_pattern) / sizeof(grid_pattern[0]);

void graph_init()
{
    gdk_color_parse("#646464", &graph_color_border);
    gdk_color_parse("#AAAAAA", &graph_color_scale);
    gdk_color_parse("#C8C8C8", &graph_color_grid);
    gdk_color_parse("#000000", &graph_color_font);

    graph_resize();
    s.len = (gui.graph->allocation.width)-GRAPH_OFFSET;
    s.data = g_new(s_data_t, s.len);
    signal_clear();
    signal_display();
    gtk_widget_add_events(gui.graph, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(gui.graph, "expose-event", G_CALLBACK(graph_draw), NULL);
    g_signal_connect(gui.graph, "button-press-event", G_CALLBACK(graph_click), NULL);
}

gboolean graph_draw(GtkWidget *widget, GdkEventExpose *event, gpointer nothing)
{
    gint _max, _min, range, i, j;
    gdouble step, h, value;
    gchar text[10];
    cairo_t *cr;
    cairo_text_extents_t extents;

    _max = G_MININT;
    _min = G_MAXINT;
    for(i=0; i<s.len; i++)
    {
        if(signal_level(signal_get_i(i)->value) > _max && signal_get_i(i)->value != -1)
        {
            _max = ceil((signal_level(signal_get_i(i)->value)));
        }
        if(signal_level(signal_get_i(i)->value) < _min && signal_get_i(i)->value != -1)
        {
            _min = floor(signal_level(signal_get_i(i)->value));
        }
    }

    cr = gdk_cairo_create(widget->window);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    gdk_cairo_set_source_color(cr, &gui.colors.background);
    cairo_paint(cr);

    if(_max == G_MININT)
    {
        cairo_destroy(cr);
        return FALSE;
    }

    if((_max - _min) == 1)
    {
        _min--;
    }

    range = _max - _min;
    step = conf.graph_height/(gfloat)((!range?1:range));

    cairo_set_line_width(cr, 1);
    gdk_cairo_set_source_color(cr, &graph_color_border);
    cairo_move_to(cr, GRAPH_OFFSET, GRAPH_OFFSET_V);
    cairo_line_to(cr, GRAPH_OFFSET, GRAPH_OFFSET_V+conf.graph_height+1); // |
    cairo_line_to(cr, GRAPH_OFFSET+s.len, GRAPH_OFFSET_V+conf.graph_height+1); // _
    cairo_stroke(cr);

    cairo_select_font_face(cr, "Bitstream Vera Sans Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, GRAPH_FONT_SIZE);
    cairo_text_extents(cr, "0", &extents);

    gint current_value = _max;
    gdouble prevpos = -100.0;
    h = GRAPH_OFFSET_V;
    while(h <= (conf.graph_height+GRAPH_OFFSET_V+1.0))
    {
        if(prevpos + (1+extents.height) < h)
        {
            gdk_cairo_set_source_color(cr, &graph_color_scale);
            cairo_move_to(cr, GRAPH_OFFSET-3, h);
            cairo_line_to(cr, GRAPH_OFFSET+2, h);
            cairo_stroke(cr);

            if(conf.show_grid)
            {
                gdk_cairo_set_source_color(cr, &graph_color_grid);
                cairo_set_dash(cr, grid_pattern, grid_pattern_len, 0);
                cairo_move_to(cr, GRAPH_OFFSET+2, h);
                cairo_line_to(cr, GRAPH_OFFSET+s.len, h);
                cairo_stroke(cr);
                cairo_set_dash(cr, NULL, 0, 0);
            }

            gdk_cairo_set_source_color(cr, &graph_color_font);

            g_snprintf(text, sizeof(text), "%3d", (current_value<=-100 ? abs(current_value) : current_value));
            cairo_move_to(cr, -1.5, h+3);
            cairo_show_text(cr, text);

            prevpos = h;
        }
        current_value--;
        h+=step;
    }

    i = ((s.pos == (s.len-1)) ? 0 : s.pos+1);
    for(j=1; j<=s.len; j++)
    {
        if(signal_get_i(i)->value != -1)
        {
            if(signal_get_i(i)->rds)
            {
                gdk_cairo_set_source_color(cr, &conf.color_rds);
            }
            else if(signal_get_i(i)->stereo)
            {
                gdk_cairo_set_source_color(cr, &conf.color_stereo);
            }
            else
            {
                gdk_cairo_set_source_color(cr, &conf.color_mono);
            }

            if(conf.signal_avg)
            {
                if(j==1 || signal_get_prev_i(i)->value == -1)
                {
                    value = (signal_get_i(i)->value + signal_get_next_i(i)->value) / 2.0;
                }
                else if (j==s.len)
                {
                    value = (signal_get_prev_i(i)->value + signal_get_i(i)->value) / 2.0;
                }
                else
                {
                    value = (signal_get_prev_i(i)->value + signal_get_i(i)->value + signal_get_next_i(i)->value) / 3.0;
                }
            }
            else
            {
                value = signal_get_i(i)->value;
            }

            value = signal_level(value);

            cairo_move_to(cr, GRAPH_OFFSET+j, GRAPH_OFFSET_V+conf.graph_height);
            cairo_line_to(cr, GRAPH_OFFSET+j, GRAPH_OFFSET_V+conf.graph_height-(value - _min)*step);
            cairo_stroke(cr);
        }

        i = ((i==(s.len-1)) ? 0 : i+1);
    }

    cairo_destroy(cr);
    return FALSE;
}

void graph_resize()
{
    gtk_widget_set_size_request(gui.graph, -1, conf.graph_height+2*GRAPH_OFFSET_V+1);
}

void graph_click(GtkWidget *widget, GdkEventButton *event, gpointer nothing)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click
    {
        signal_clear();
    }
}

void signal_display()
{
    switch(conf.signal_display)
    {
    case SIGNAL_GRAPH:
        gtk_widget_hide(gui.p_signal);
        gtk_widget_show(gui.graph);
        break;

    case SIGNAL_BAR:
        gtk_widget_show(gui.p_signal);
        gtk_widget_hide(gui.graph);
        break;

    default:
        gtk_widget_hide(gui.p_signal);
        gtk_widget_hide(gui.graph);
        break;
    }
}
