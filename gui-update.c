#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include "gui-update.h"
#include "gui.h"
#include "gui-tuner.h"
#include "tuner.h"
#include "pattern.h"
#include "settings.h"
#include "sig.h"
#include "stationlist.h"
#include "version.h"

static const gchar* const pty_list[][32] =
{
    { "None", "News", "Affairs", "Info", "Sport", "Educate", "Drama", "Culture", "Science", "Varied", "Pop M", "Rock M", "Easy M", "Light M", "Classics", "Other M", "Weather", "Finance", "Children", "Social", "Religion", "Phone In", "Travel", "Leisure", "Jazz", "Country", "Nation M", "Oldies", "Folk M", "Document", "TEST", "Alarm !" },
    { "None", "News", "Inform", "Sports", "Talk", "Rock", "Cls Rock", "Adlt Hit", "Soft Rck", "Top 40", "Country", "Oldies", "Soft", "Nostalga", "Jazz", "Classicl", "R & B", "Soft R&B", "Language", "Rel Musc", "Rel Talk", "Persnlty", "Public", "College", "N/A", "N/A", "N/A", "N/A", "N/A", "Weather", "Test", "ALERT!" }
};

gboolean gui_update_freq(gpointer data)
{
    gchar buffer[8];
    g_snprintf(buffer, sizeof(buffer), "%7.3f", GPOINTER_TO_INT(data)/1000.0);
    gtk_label_set_text(GTK_LABEL(gui.l_freq), buffer);
    stationlist_freq(GPOINTER_TO_INT(data));
    s.max = 0;
    gui_clear_rds();
    return FALSE;
}

gboolean gui_update_signal(gpointer data)
{
    s_data_t* signal = (s_data_t*)data;

    if(signal->value > s.max)
    {
        s.max = signal->value;
    }

    if(s.stereo != signal->stereo)
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_st), GTK_STATE_NORMAL, signal->stereo?&gui.colors.stereo:&gui.colors.grey);
        s.stereo = !s.stereo;
    }

    if(s.rds != signal->rds)
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_rds), GTK_STATE_NORMAL, signal->rds?&conf.color_rds:&gui.colors.grey);
        s.rds = !s.rds;
    }

    if(signal->value != lround(signal_get()->value))
    {
        stationlist_rcvlevel(lround(signal->value));
    }

    if((s.pos % 3) == 0) // slower signal level label refresh (~5 fps)
    {
        gchar *str, *s_m, *s_m2;
        if(tuner.mode == MODE_FM)
        {
            switch(conf.signal_unit)
            {
            case UNIT_DBM:
                str = g_markup_printf_escaped("<span color=\"#777777\">%4.0f↑</span>%4.0fdBm", s.max-120, signal->value-120);
                break;

            case UNIT_DBUV:
                str = g_markup_printf_escaped("<span color=\"#777777\">%3.0f↑</span>%3.0f dBµV", s.max-11.25, signal->value-11.25);
                break;

            case UNIT_S:
                s_m = s_meter(s.max);
                s_m2 = s_meter(signal->value);
                str = g_markup_printf_escaped("<span color=\"#777777\"> %5s↑</span>%5s", s_m, s_m2);
                g_free(s_m);
                g_free(s_m2);
                break;

            case UNIT_DBF:
            default:
                str = g_markup_printf_escaped("<span color=\"#777777\"> %3.0f↑</span>%3.0f dBf", s.max, signal->value);
                break;
            }
        }
        else
        {
            str = g_markup_printf_escaped("<span color=\"#777777\">    %3.0f↑</span> %3.0f", s.max, signal->value);
        }
        gtk_label_set_markup(GTK_LABEL(gui.l_sig), str);
        g_free(str);
    }

    signal_push(signal->value, signal->stereo, signal->rds);

    if(conf.signal_display == SIGNAL_GRAPH)
    {
        gtk_widget_queue_draw(gui.graph);
    }
    else if(conf.signal_display == SIGNAL_BAR)
    {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gui.p_signal), ((signal->value >= 80)? 1.0 : signal->value/80.0));
    }
    if(pattern.active)
    {
        pattern_push(signal->value);
    }
    g_free(data);
    return FALSE;
}

gboolean gui_update_pi(gpointer data)
{
    pi_t* pi = (pi_t*)data;
    gchar buffer[6];
    g_snprintf(buffer, sizeof(buffer), "%04X%c", pi->pi, (pi->checked?' ':'?'));
    gtk_label_set_text(GTK_LABEL(gui.l_pi), buffer);
    g_free(data);
    return FALSE;
}

gboolean gui_update_af(gpointer data)
{
    GtkTreeIter iter;
    // if new frequency is found on the AF list, gui_af_check() will set the pointer to NULL
    gtk_tree_model_foreach(GTK_TREE_MODEL(gui.af), (GtkTreeModelForeachFunc)gui_update_af_check, &data);
    if(data)
    {
        gtk_list_store_append(gui.af, &iter);
        gchar* af_new_freq = g_strdup_printf("%.1f", ((87500+GPOINTER_TO_INT(data)*100)/1000.0));
        gtk_list_store_set(gui.af, &iter, 0, GPOINTER_TO_INT(data), 1, af_new_freq, -1);
        stationlist_af(GPOINTER_TO_INT(data));
        g_free(af_new_freq);
    }
    return FALSE;
}

gboolean gui_update_af_check(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer* newfreq)
{
    gint cfreq;
    gtk_tree_model_get(model, iter, 0, &cfreq, -1);
    if(cfreq == GPOINTER_TO_INT(*newfreq))
    {
        *newfreq = NULL;
        return TRUE; // frequency is already on the list, stop searching.
    }
    return FALSE;
}

gboolean gui_update_ps(gpointer nothing)
{
    guchar c[8];
    gint i;
    gchar *markup;
    for(i=0; i<8; i++)
    {
        c[i] = (tuner.ps_err[i] ? 110+(tuner.ps_err[i] * 12) : 0);
    }
    markup = g_markup_printf_escaped(
                 "<span color=\"#C8C8C8\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#%02X%02X%02X\">%c</span><span color=\"#C8C8C8\">%c</span>",
                 (conf.rds_ps_progressive?'(':'['),
                 c[0], c[0], c[0], tuner.ps[0],
                 c[1], c[1], c[1], tuner.ps[1],
                 c[2], c[2], c[2], tuner.ps[2],
                 c[3], c[3], c[3], tuner.ps[3],
                 c[4], c[4], c[4], tuner.ps[4],
                 c[5], c[5], c[5], tuner.ps[5],
                 c[6], c[6], c[6], tuner.ps[6],
                 c[7], c[7], c[7], tuner.ps[7],
                 (conf.rds_ps_progressive?')':']')
             );
    gtk_label_set_markup(GTK_LABEL(gui.l_ps), markup);
    g_free(markup);
    return FALSE;
}

gboolean gui_update_rt(gpointer flag)
{
    gchar *markup = g_markup_printf_escaped("<span color=\"#C8C8C8\">[</span>%s<span color=\"#C8C8C8\">]</span>", tuner.rt[GPOINTER_TO_INT(flag)]);
    gtk_label_set_markup(GTK_LABEL(gui.l_rt[GPOINTER_TO_INT(flag)]), markup);
    g_free(markup);
    return FALSE;
}

gboolean gui_update_tp(gpointer data)
{
    gtk_label_set_text(GTK_LABEL(gui.l_tp), "TP");
    if(GPOINTER_TO_INT(data))
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_tp), GTK_STATE_NORMAL, &gui.colors.black);
    }
    else
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_tp), GTK_STATE_NORMAL, &gui.colors.grey);
    }
    return FALSE;
}

gboolean gui_update_ta(gpointer data)
{
    gtk_label_set_text(GTK_LABEL(gui.l_ta), "TA");
    if(GPOINTER_TO_INT(data))
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_ta), GTK_STATE_NORMAL, &gui.colors.black);
    }
    else
    {
        gtk_widget_modify_fg(GTK_WIDGET(gui.l_ta), GTK_STATE_NORMAL, &gui.colors.grey);
    }
    return FALSE;
}

gboolean gui_update_ms(gpointer data)
{
    gchar *markup;
    if(GPOINTER_TO_INT(data))
    {
        markup = g_markup_printf_escaped("M<span color=\"#DDDDDD\">S</span>");
    }
    else
    {
        markup = g_markup_printf_escaped("<span color=\"#DDDDDD\">M</span>S");
    }
    gtk_label_set_markup(GTK_LABEL(gui.l_ms), markup);
    g_free(markup);
    return FALSE;
}

gboolean gui_update_pty(gpointer data)
{
    gchar buffer[9];
    g_snprintf(buffer, sizeof(buffer), "%-8s", pty_list[conf.rds_pty][GPOINTER_TO_INT(data)]);
    gtk_label_set_text(GTK_LABEL(gui.l_pty), buffer);
    return FALSE;
}

gboolean gui_update_volume(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.volume), GINT_TO_POINTER(tuner_set_volume), NULL);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(gui.volume), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.volume), GINT_TO_POINTER(tuner_set_volume), NULL);
    return FALSE;
}

gboolean gui_update_squelch(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.squelch), GINT_TO_POINTER(tuner_set_squelch), NULL);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(gui.squelch), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.squelch), GINT_TO_POINTER(tuner_set_squelch), NULL);
    return FALSE;
}

gboolean gui_update_agc(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_agc), GINT_TO_POINTER(tuner_set_agc), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_agc), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_agc), GINT_TO_POINTER(tuner_set_agc), NULL);
    return FALSE;
}

gboolean gui_update_deemphasis(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_deemph), GINT_TO_POINTER(tuner_set_deemphasis), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_deemph), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_deemph), GINT_TO_POINTER(tuner_set_deemphasis), NULL);
    return FALSE;
}

gboolean gui_update_ant(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_ant), GINT_TO_POINTER(tuner_set_antenna), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_ant), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_ant), GINT_TO_POINTER(tuner_set_antenna), NULL);
    return FALSE;
}

gboolean gui_update_gain(gpointer data)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_rf), (GPOINTER_TO_INT(data) == 10 || GPOINTER_TO_INT(data) == 11));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_if), (GPOINTER_TO_INT(data) == 01 || GPOINTER_TO_INT(data) == 11));
    return FALSE;
}

gboolean gui_update_filter(gpointer data)
{
    gint i;

    for(i=0; i<FILTERS_N; i++)
    {
        if(filters[i] == GPOINTER_TO_INT(data))
        {
            g_signal_handlers_block_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tuner_set_bandwidth), NULL);
            gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), i);
            g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tuner_set_bandwidth), NULL);
            tuner.filter = i;
            stationlist_bw(i);
            break;
        }
    }

    return FALSE;
}

gboolean gui_update_rotator(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.b_cw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(1));
    g_signal_handlers_block_by_func(G_OBJECT(gui.b_ccw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(2));
    switch(GPOINTER_TO_INT(data))
    {
        case 1:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), TRUE);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), FALSE);
            break;

        case 2:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), FALSE);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), TRUE);
            break;

        default:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), FALSE);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), FALSE);
            break;
    }
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.b_cw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(1));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.b_ccw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(2));
    return FALSE;
}

gboolean gui_update_alignment(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.adj_align), GINT_TO_POINTER(tuner_set_alignment), NULL);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(gui.adj_align), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.adj_align), GINT_TO_POINTER(tuner_set_alignment), NULL);
    return FALSE;
}

gboolean gui_clear_power_off()
{
    gui_clear();
    signal_clear();
    if(conf.signal_display == SIGNAL_GRAPH)
    {
        gtk_widget_queue_draw(gui.graph);
    }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gui.p_signal), 0);

    gtk_widget_set_sensitive(gui.b_connect, TRUE);
    connect_button(FALSE);
    gtk_window_set_title(GTK_WINDOW(gui.window), APP_NAME);
    return FALSE;
}
