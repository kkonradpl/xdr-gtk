#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "scan.h"
#include "ui-tuner-set.h"
#include "tuner.h"
#include "conf.h"
#include "log.h"
#include "ui-input.h"

gboolean rotation_shift_pressed;

gboolean
keyboard_press(GtkWidget   *widget,
               GdkEventKey *event,
               gpointer     disable_frequency_entry)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    gboolean shift_pressed = (event->state & GDK_SHIFT_MASK);
    gboolean ctrl_pressed = (event->state & GDK_CONTROL_MASK);

    // tuning
    if(current == conf.key_tune_down)
    {
        tuner_modify_frequency(TUNER_FREQ_MODIFY_DOWN);
        return TRUE;
    }

    if(current == conf.key_tune_up)
    {
        tuner_modify_frequency(TUNER_FREQ_MODIFY_UP);
        return TRUE;
    }

    if(current == conf.key_tune_fine_up)
    {
        if(tuner_get_freq() < 1900)
            tuner_set_frequency(tuner_get_freq()+1);
        else
            tuner_set_frequency(tuner_get_freq()+(conf.fm_10k_steps ? 10 : 5));
        return TRUE;
    }

    if(current == conf.key_tune_fine_down)
    {
        if(tuner_get_freq() <= 1900)
            tuner_set_frequency(tuner_get_freq()-1);
        else
            tuner_set_frequency(tuner_get_freq()-(conf.fm_10k_steps ? 10 : 5));
        return TRUE;
    }

    if(current == conf.key_tune_jump_down)
    {
        tuner_set_frequency(tuner_get_freq()-1000);
        return TRUE;
    }

    if(current == conf.key_tune_jump_up)
    {
        tuner_set_frequency(tuner_get_freq()+1000);
        return TRUE;
    }

    if(current == conf.key_tune_back)
    {
        gtk_button_clicked(GTK_BUTTON(ui.b_tune_back));
        return TRUE;
    }

    if(current == conf.key_tune_reset)
    {
        gtk_button_clicked(GTK_BUTTON(ui.b_tune_reset));
        return TRUE;
    }

    if(current == conf.key_screenshot)
    {
        ui_screenshot();
        return TRUE;
    }

    if(current == conf.key_rotate_cw)
    {
        rotation_shift_pressed = shift_pressed;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_cw), TRUE);
        return TRUE;
    }

    if(current == conf.key_rotate_ccw)
    {
        rotation_shift_pressed = shift_pressed;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_ccw), TRUE);
        return TRUE;
    }

    if(current == conf.key_switch_antenna)
    {
        gint current = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_ant));
        gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_ant), ((current < conf.ant_count-1) ? current+1 : 0));
        return TRUE;
    }

    if(current == conf.key_rds_ps_mode)
    {
        ui_toggle_ps_mode();
        return TRUE;
    }

    if(current == conf.key_scan_toggle)
    {
        scan_try_toggle(shift_pressed);
        return TRUE;
    }

    if(current == conf.key_scan_prev)
    {
        scan_try_prev();
        return TRUE;
    }

    if(current == conf.key_scan_next)
    {
        scan_try_next();
        return TRUE;
    }

    // presets
    if(event->keyval >= GDK_KEY_F1 && event->keyval <= GDK_KEY_F12)
    {
        gint id = event->keyval-GDK_KEY_F1;
        if(shift_pressed)
        {
            conf.presets[id] = tuner_get_freq();
            ui_status(1500, "Preset <b>F%d</b> has been stored.", id+1);
        }
        else
        {
            tuner_set_frequency(conf.presets[id]);
        }
        return TRUE;
    }

    // decrease filter bandwidth
    if(current == conf.key_bw_down)
    {
        gint current = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_bw));
        gint offset = (gint)(tuner.mode == MODE_AM); /* don't set the 'default' option in AM mode */
        if(current != gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(ui.c_bw))), NULL)-1-offset)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_bw), current+1);
        }
        return TRUE;
    }

    // increase filter bandwidth
    if(current == conf.key_bw_up)
    {
        gint current = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_bw));
        if(current != 0)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_bw), current-1);
        }
        return TRUE;
    }

    // adaptive filter bandwidth
    if(current == conf.key_bw_auto)
    {
        if(tuner.mode == MODE_FM)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_bw),
                                     gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(ui.c_bw))), NULL) - 1);
        }
        return TRUE;
    }

    if(current == conf.key_stereo_toggle)
    {
        tuner_set_forced_mono(!tuner.forced_mono);
        return TRUE;
    }

    if(current == conf.key_mode_toggle)
    {
        tuner_set_mode((tuner.mode != MODE_FM) ? MODE_FM : MODE_AM);
        return TRUE;
    }

    if(ctrl_pressed)
    {
        if(current == GDK_KEY_1)
            conf.title_tuner_mode = 0;
        else if(current == GDK_KEY_2)
            conf.title_tuner_mode = 1;
        else if(current == GDK_KEY_3)
            conf.title_tuner_mode = 2;
        else if(current == GDK_KEY_4)
            conf.title_tuner_mode = 3;
        else if(current == GDK_KEY_5)
            conf.title_tuner_mode = 4;
        else if(current == GDK_KEY_6)
            conf.title_tuner_mode = 5;

        g_source_remove(ui.title_timeout);
        ui_update_title(NULL);

        return TRUE;
    }

    if(disable_frequency_entry)
    {
        return FALSE;
    }

    // freq entry
    gtk_widget_grab_focus(ui.e_freq);
    gtk_editable_set_position(GTK_EDITABLE(ui.e_freq), -1);

    gchar buff[10], buff2[10];
    gint i = 0, j = 0, k = 0;
    gboolean flag = FALSE;
    g_snprintf(buff, 10, "%s", gtk_entry_get_text(GTK_ENTRY(ui.e_freq)));
    if (event->keyval == GDK_KEY_Return ||
        event->keyval == GDK_KEY_KP_Enter)
    {
        for(i=0; i<strlen(buff); i++)
            if(buff[i]=='.')
                flag = TRUE;
        if(flag)
        {
            k=strlen(buff);
            for(i=0; i<k; i++)
            {
                if(buff[i] != '.')
                    buff2[j++] = buff[i];
                else
                {
                    buff2[j] = '0';
                    buff2[j+1] = '0';
                    buff2[j+2] = '0';
                    buff2[j+3] = 0x00;
                    if(k>(i+4))
                        k=i+4;
                }
            }
        }
        else
            g_snprintf(buff2, 10, "%s000", buff);

        gint i = atoi(buff2);
        if(i>=100)
            tuner_set_frequency(atoi(buff2));

        gtk_entry_set_text(GTK_ENTRY(ui.e_freq), "");
        gtk_editable_set_position(GTK_EDITABLE(ui.e_freq), 2);
        return TRUE;
    }
    else if(event->state & GDK_CONTROL_MASK)
    {
        return FALSE;
    }
    else if((event->keyval < GDK_KEY_0 || event->keyval > GDK_KEY_9) && (event->keyval < GDK_KEY_KP_0 || event->keyval > GDK_KEY_KP_9) && event->keyval != GDK_KEY_BackSpace && event->keyval != '.')
    {
        return TRUE;
    }
    else
    {
        for(i=0; i<strlen(buff); i++)
            if(buff[i]=='.')
                flag = TRUE;
        if(flag)
        {
            if(event->keyval == '.')
                return TRUE;
        }
        else
        {
            i=atoi(buff);
            if(i>=20 && i<=200 && event->keyval != GDK_KEY_BackSpace && event->keyval != '.')
            {
                switch(event->keyval)
                {
                case GDK_KEY_KP_0:
                    sprintf(buff2, ".0");
                    break;
                case GDK_KEY_KP_1:
                    sprintf(buff2, ".1");
                    break;
                case GDK_KEY_KP_2:
                    sprintf(buff2, ".2");
                    break;
                case GDK_KEY_KP_3:
                    sprintf(buff2, ".3");
                    break;
                case GDK_KEY_KP_4:
                    sprintf(buff2, ".4");
                    break;
                case GDK_KEY_KP_5:
                    sprintf(buff2, ".5");
                    break;
                case GDK_KEY_KP_6:
                    sprintf(buff2, ".6");
                    break;
                case GDK_KEY_KP_7:
                    sprintf(buff2, ".7");
                    break;
                case GDK_KEY_KP_8:
                    sprintf(buff2, ".8");
                    break;
                case GDK_KEY_KP_9:
                    sprintf(buff2, ".9");
                    break;
                default:
                    sprintf(buff2, ".%c", event->keyval);
                    break;
                }

                gchar* content = gtk_editable_get_chars(GTK_EDITABLE(ui.e_freq), 0, -1);
                gint position = strlen(content);
                gtk_editable_insert_text(GTK_EDITABLE(ui.e_freq), buff2, sizeof(buff2), &position);
                gtk_editable_set_position(GTK_EDITABLE(ui.e_freq), -1);
                g_free(content);
                return TRUE;
            }
        }
    }
    return FALSE;
}

gboolean
keyboard_release(GtkWidget   *widget,
                 GdkEventKey *event,
                 gpointer     nothing)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    rotation_shift_pressed |= (event->state & GDK_SHIFT_MASK);

    if(current == conf.key_rotate_cw)
    {
        if(!rotation_shift_pressed)
        {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_cw), FALSE);
            return TRUE;
        }
    }
    else if(current == conf.key_rotate_ccw)
    {
        if(!rotation_shift_pressed)
        {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_ccw), FALSE);
            return TRUE;
        }
    }

    return FALSE;
}

gboolean
mouse_window(GtkWidget      *widget,
             GdkEventButton *event,
             GtkWindow      *window)
{
    static gboolean compact_mode = FALSE;

    if(event->type == GDK_BUTTON_PRESS && event->button == 1 && event->state & GDK_SHIFT_MASK)
    {
        gtk_window_begin_move_drag(window, event->button, event->x_root, event->y_root, event->time);
        return TRUE;
    }
    else if(event->type == GDK_3BUTTON_PRESS && event->button == 1)
    {
        if(compact_mode)
        {
            gtk_widget_hide(ui.box_left_settings1);
            gtk_widget_hide(ui.box_left_settings2);
        }
        else
        {
            gtk_widget_show(ui.box_left_settings1);
            gtk_widget_show(ui.box_left_settings2);
        }
        compact_mode = !compact_mode;
    }
    return FALSE;
}

gboolean
mouse_freq(GtkWidget *widget,
           GdkEvent  *event,
           gpointer   nothing)
{
    const gchar *freq_text = gtk_label_get_text(GTK_LABEL(ui.l_freq));
    const gchar *pi_text = gtk_label_get_text(GTK_LABEL(ui.l_pi));
    gchar buff[30];
    gchar *ps;

    if (librds_get_ps_available(tuner.rds))
    {
        if(conf.replace_spaces)
        {
            ps = replace_spaces(librds_get_ps(tuner.rds));
            g_snprintf(buff, sizeof(buff), "%s %s %s", freq_text, pi_text, ps);
            g_free(ps);
        }
        else
        {
            g_snprintf(buff, sizeof(buff), "%s %s %s", freq_text, pi_text, librds_get_ps(tuner.rds));
        }
    }
    else if(tuner.rds_pi >= 0)
    {
        g_snprintf(buff, sizeof(buff), "%s %s", freq_text, pi_text);
    }
    else
    {
        g_snprintf(buff, sizeof(buff), "%s", freq_text);
    }

    gtk_clipboard_set_text(gtk_widget_get_clipboard(ui.window, GDK_SELECTION_CLIPBOARD),
                           buff,
                           -1);
    return TRUE;
}

gboolean
mouse_scroll(GtkWidget      *widget,
             GdkEventScroll *event,
             gpointer        user_data)
{
    if (!conf.signal_scroll)
        return TRUE;

    if (event->direction == GDK_SCROLL_DOWN)
        tuner_modify_frequency(TUNER_FREQ_MODIFY_DOWN);
    else if (event->direction == GDK_SCROLL_UP)
        tuner_modify_frequency(TUNER_FREQ_MODIFY_UP);

    return TRUE;
}

gboolean
mouse_pi(GtkWidget *widget,
         GdkEvent  *event,
         gpointer   nothing)
{
    if(tuner.rds_pi < 0)
        return FALSE;

    gtk_clipboard_set_text(gtk_widget_get_clipboard(ui.window, GDK_SELECTION_CLIPBOARD),
                           gtk_label_get_text(GTK_LABEL(ui.l_pi)),
                           -1);
    return TRUE;
}

gboolean
mouse_ps(GtkWidget      *widget,
         GdkEventButton *event,
         gpointer        data)
{
    if (event->type == GDK_BUTTON_PRESS &&
        event->button == 3) // right click
    {
        ui_toggle_ps_mode();
        return FALSE;
    }

    if (!librds_get_ps_available(tuner.rds))
    {
        return FALSE;
    }

    if (conf.replace_spaces)
    {
        gchar *ps = replace_spaces(librds_get_ps(tuner.rds));
        gtk_clipboard_set_text(gtk_widget_get_clipboard(ui.window, GDK_SELECTION_CLIPBOARD),
                               ps,
                               -1);
        g_free(ps);
    }
    else
    {
        gtk_clipboard_set_text(gtk_widget_get_clipboard(ui.window, GDK_SELECTION_CLIPBOARD),
                               librds_get_ps(tuner.rds),
                               -1);
    }
    return TRUE;
}

gboolean
mouse_rt(GtkWidget      *widget,
         GdkEventButton *event,
         gpointer        data)
{
    librds_rt_flag_t flag = (librds_rt_flag_t)data;
    gchar *str;

    if (event->type == GDK_BUTTON_PRESS &&
        event->button == 3) // right click
    {
        ui_toggle_rt_mode();
        return FALSE;
    }

    if(conf.replace_spaces)
    {
        str = replace_spaces(librds_get_rt(tuner.rds, flag));
        gtk_clipboard_set_text(gtk_widget_get_clipboard(ui.window, GDK_SELECTION_CLIPBOARD),
                               str,
                               -1);
        g_free(str);
    }
    else
    {
        gtk_clipboard_set_text(gtk_widget_get_clipboard(ui.window, GDK_SELECTION_CLIPBOARD),
                               librds_get_rt(tuner.rds, flag),
                               -1);
    }

    return FALSE;
}
