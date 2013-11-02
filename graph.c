#include <gtk/gtk.h>
#include <math.h>
#include "settings.h"
#include "connection_read.h"
#include "gui.h"
#include "graph.h"

void graph_init()
{
    gdk_color_parse("#646464", &graph_color_border);
    gdk_color_parse("#AAAAAA", &graph_color_scale);
    gdk_color_parse("#C8C8C8", &graph_color_grid);
    gdk_color_parse("#000000", &graph_color_font);

    graph_resize();
    rssi_len = (gui.graph->allocation.width)-GRAPH_OFFSET;
    rssi = g_new(rssi_struct, rssi_len);
    graph_clear();
    signal_display();
    gtk_widget_add_events(gui.graph, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(gui.graph, "expose-event", G_CALLBACK(graph_draw), NULL);
    g_signal_connect(gui.graph, "button-press-event", G_CALLBACK(graph_click), NULL);
}

gboolean graph_draw(GtkWidget *widget, GdkEventExpose *event, gpointer nothing)
{
    gint _max, _min, range, i, j;
    gfloat step, h, value;
    gchar text[10];
    cairo_t *cr;

    _max = G_MININT;
    _min = G_MAXINT;
    for(i=0; i<rssi_len; i++)
    {
        if(signal_level(rssi[i].value) > _max && rssi[i].value != -1)
        {
            _max = ceil((signal_level(rssi[i].value)));
        }
        if(signal_level(rssi[i].value) < _min && rssi[i].value != -1)
        {
            _min = floor(signal_level(rssi[i].value));
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

    if(_min && (_max - _min) == 1)
    {
        _min--;
    }

    range = _max - _min;
    step = conf.graph_height/(float)((!range?1:range));

    cairo_set_line_width(cr, 1);
    gdk_cairo_set_source_color(cr, &graph_color_border);
    cairo_move_to(cr, GRAPH_OFFSET, GRAPH_HALF_FONT);
    cairo_line_to(cr, GRAPH_OFFSET, GRAPH_HALF_FONT+conf.graph_height+1); // |
    cairo_line_to(cr, GRAPH_OFFSET+rssi_len, GRAPH_HALF_FONT+conf.graph_height+1); // _
    cairo_stroke(cr);

    cairo_select_font_face(cr, "Bitstream Vera Sans Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, GRAPH_HALF_FONT*2);


    gint current = _max;
    gint prevpos = -1.75*GRAPH_HALF_FONT;

    const double pattern[] = {1.0, 1.0};
    gint pattern_len = sizeof(pattern) / sizeof(pattern[0]);

    for(h=GRAPH_HALF_FONT; h<=(conf.graph_height+GRAPH_HALF_FONT); h+=step)
    {
        if((prevpos + 1.75*GRAPH_HALF_FONT < h))
        {
            gdk_cairo_set_source_color(cr, &graph_color_scale);
            cairo_move_to(cr, GRAPH_OFFSET-3, h);
            cairo_line_to(cr, GRAPH_OFFSET+2, h);
            cairo_stroke(cr);

            if(conf.show_grid)
            {
                gdk_cairo_set_source_color(cr, &graph_color_grid);
                cairo_set_dash(cr, pattern, pattern_len, 0);
                cairo_move_to(cr, GRAPH_OFFSET+2, h);
                cairo_line_to(cr, GRAPH_OFFSET+rssi_len, h);
                cairo_stroke(cr);
                cairo_set_dash(cr, NULL, 0, 0);
            }

            gdk_cairo_set_source_color(cr, &graph_color_font);
            if(current<=-100)
            {
                g_snprintf(text, sizeof(text), "%3d", abs(current));
            }
            else
            {
                g_snprintf(text, sizeof(text), "%3d", current);
            }
            cairo_move_to(cr, -1.5, h+3);
            cairo_show_text(cr, text);
            prevpos = h;
        }
        current--;
    }

    i = ((rssi_pos == (rssi_len-1)) ? 0 : rssi_pos+1);
    for(j=1; j<=rssi_len; j++)
    {
        if(rssi[i].value != -1)
        {
            if(rssi[i].rds)
            {
                gdk_cairo_set_source_color(cr, &conf.color_rds);
            }
            else if(rssi[i].stereo)
            {
                gdk_cairo_set_source_color(cr, &conf.color_stereo);
            }
            else
            {
                gdk_cairo_set_source_color(cr, &conf.color_mono);
            }

            if(conf.signal_avg)
            {
                if(j==1 || rssi[((i==0)?rssi_len-1:i-1)].value == -1)
                {
                    value = (rssi[i].value + rssi[((i==(rssi_len-1)) ? 0 : i+1)].value) / 2.0;
                }
                else if (j==rssi_len)
                {
                    value = (rssi[((i==0) ? rssi_len-1 : i-1)].value + rssi[i].value) / 2.0;
                }
                else
                {
                    value = (rssi[((i==0) ? rssi_len-1 : i-1)].value + rssi[i].value + rssi[((i==(rssi_len-1)) ? 0 : i+1)].value) / 3.0;
                }
            }
            else
            {
                value = rssi[i].value;
            }

            value = signal_level(value);

            cairo_move_to(cr, GRAPH_OFFSET+j, GRAPH_HALF_FONT+conf.graph_height);
            cairo_line_to(cr, GRAPH_OFFSET+j, GRAPH_HALF_FONT+conf.graph_height-(value - _min)*step);
            cairo_stroke(cr);
        }

        i = ((i==(rssi_len-1)) ? 0 : i+1);
    }

    cairo_destroy(cr);
    return FALSE;
}

inline void graph_resize()
{
    gtk_widget_set_size_request(gui.graph, -1, conf.graph_height+2*GRAPH_HALF_FONT+1);
}

void graph_click(GtkWidget *widget, GdkEventButton *event, gpointer nothing)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click
    {
        graph_clear();
    }
}

void graph_clear()
{
    gint i;
    for(i=0; i<rssi_len; i++)
    {
        rssi[i].value = -1;
        rssi[i].rds = FALSE;
        rssi[i].stereo = FALSE;
    }
    rssi_pos = -1;
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
