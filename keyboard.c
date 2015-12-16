#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include "keyboard.h"
#include "gui.h"
#include "scan.h"
#include "gui-tuner.h"
#include "tuner.h"
#include "settings.h"

gboolean rotation_shift_pressed;

gboolean keyboard_press(GtkWidget* widget, GdkEventKey* event, gpointer disable_frequency_entry)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    gboolean shift_pressed = (event->state & GDK_SHIFT_MASK);

    // tuning
    if(current == conf.key_tune_down)
    {
        tuner_modify_frequency(FREQ_MODIFY_DOWN);
        return TRUE;
    }

    if(current == conf.key_tune_up)
    {
        tuner_modify_frequency(FREQ_MODIFY_UP);
        return TRUE;
    }

    if(current == conf.key_tune_up_5)
    {
        if(tuner.freq < 1900)
        {
            tuner_set_frequency(tuner.freq+1);
        }
        else
        {
            tuner_set_frequency(tuner.freq+5);
        }
        return TRUE;
    }

    if(current == conf.key_tune_down_5)
    {
        if(tuner.freq <= 1900)
        {
            tuner_set_frequency(tuner.freq-1);
        }
        else
        {
            tuner_set_frequency(tuner.freq-5);
        }
        return TRUE;
    }

    if(current == conf.key_tune_down_1000)
    {
        tuner_set_frequency(tuner.freq-1000);
        return TRUE;
    }

    if(current == conf.key_tune_up_1000)
    {
        tuner_set_frequency(tuner.freq+1000);
        return TRUE;
    }

    if(current == conf.key_tune_back)
    {
        gtk_button_clicked(GTK_BUTTON(gui.b_tune_back));
        return TRUE;
    }

    if(current == conf.key_reset)
    {
        gtk_button_clicked(GTK_BUTTON(gui.b_tune_reset));
        return TRUE;
    }

    if(current == conf.key_screen)
    {
        gui_screenshot();
        return TRUE;
    }

    if(current == conf.key_rotate_cw)
    {
        rotation_shift_pressed = shift_pressed;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), TRUE);
        return TRUE;
    }

    if(current == conf.key_rotate_ccw)
    {
        rotation_shift_pressed = shift_pressed;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), TRUE);
        return TRUE;
    }

    if(current == conf.key_switch_ant)
    {
        gint current = gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_ant));
        gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_ant), ((current < conf.ant_count-1) ? current+1 : 0));
        return TRUE;
    }

    if(current == conf.key_ps_mode)
    {
        gui_toggle_ps_mode();
        return TRUE;
    }

    if(current == conf.key_spectral_toggle && scan.window)
    {
        gtk_button_clicked(GTK_BUTTON(scan.b_start));
        return TRUE;
    }

    // presets
    if(event->keyval >= GDK_F1 && event->keyval <= GDK_F12)
    {
        gint id = event->keyval-GDK_F1;
        if(shift_pressed)
        {
            conf.presets[id] = tuner.freq;
            gui_status(1500, "Preset <b>F%d</b> has been stored.", id+1);
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
        gint current = gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_bw));
        if(current != gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(gui.c_bw))), NULL)-1)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), current+1);
        }
        return TRUE;
    }

    // increase filter bandwidth
    if(current == conf.key_bw_up)
    {
        gint current = gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_bw));
        if(current != 0)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), current-1);
        }
        return TRUE;
    }

    // adaptive filter bandwidth
    if(current == conf.key_bw_auto)
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(gui.c_bw))), NULL) - 1);
        return TRUE;
    }

    if(disable_frequency_entry)
    {
        return FALSE;
    }

    // freq entry
    gtk_widget_grab_focus(gui.e_freq);
    gtk_editable_set_position(GTK_EDITABLE(gui.e_freq), -1);

    gchar buff[10], buff2[10];
    gint i = 0, j = 0, k = 0;
    gboolean flag = FALSE;
    g_snprintf(buff, 10, "%s", gtk_entry_get_text(GTK_ENTRY(gui.e_freq)));
    if(event->keyval == GDK_Return)
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

        gtk_entry_set_text(GTK_ENTRY(gui.e_freq), "");
        gtk_editable_set_position(GTK_EDITABLE(gui.e_freq), 2);
        return TRUE;
    }
    else if(event->state & GDK_CONTROL_MASK)
    {
        return FALSE;
    }
    else if((event->keyval < GDK_KEY_0 || event->keyval > GDK_KEY_9) && (event->keyval < GDK_KEY_KP_0 || event->keyval > GDK_KEY_KP_9) && event->keyval != GDK_BackSpace && event->keyval != '.')
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
            if(i>=20 && i<=200 && event->keyval != GDK_BackSpace && event->keyval != '.')
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

                gtk_entry_append_text(GTK_ENTRY(gui.e_freq), buff2);
                gtk_editable_set_position(GTK_EDITABLE(gui.e_freq), -1);
                return TRUE;
            }
        }
    }
    return FALSE;
}

gboolean keyboard_release(GtkWidget* widget, GdkEventKey* event, gpointer nothing)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    rotation_shift_pressed |= (event->state & GDK_SHIFT_MASK);

    if(current == conf.key_rotate_cw)
    {
        if(!rotation_shift_pressed)
        {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), FALSE);
            return TRUE;
        }
    }
    else if(current == conf.key_rotate_ccw)
    {
        if(!rotation_shift_pressed)
        {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), FALSE);
            return TRUE;
        }
    }

    return FALSE;
}

gboolean window_click(GtkWidget *widget, GdkEventButton *event, GtkWindow* window)
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
            gtk_widget_hide(gui.box_left_settings1);
            gtk_widget_hide(gui.box_left_settings2);
        }
        else
        {
            gtk_widget_show(gui.box_left_settings1);
            gtk_widget_show(gui.box_left_settings2);
        }
        compact_mode = !compact_mode;
    }
    return FALSE;
}
