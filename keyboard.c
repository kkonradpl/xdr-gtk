#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"
#include "connection.h"
#include "settings.h"

gboolean keyboard(GtkWidget* widget, GdkEventKey* event, gpointer nothing)
{
    guint current = gdk_keyval_to_upper(event->keyval);

    // tuning
    if(current == conf.key_tune_down)
    {
        if(tuner.freq > 65750 && tuner.freq <= 74000)
        {
            if(((tuner.freq-65750) % 30) == 0)
                tune(tuner.freq-30);
            else
                tune(65750+((tuner.freq-65750)/30*30));
        }
        else
        {
            if(tuner.freq%100<50 && tuner.freq%100!=0)
                tune_r(tuner.freq);
            else
                tune_r(tuner.freq-100);
        }
        return TRUE;
    }

    if(current == conf.key_tune_up)
    {
        if(tuner.freq >= 65750 && tuner.freq < 74000)
        {
            if(((tuner.freq-65750) % 30) == 0)
                tune(tuner.freq+30);
            else
                tune(65750+((tuner.freq-65750)/30*30)+30);
        }
        else
        {
            if(tuner.freq%100>=50)
                tune_r(tuner.freq);
            else
                tune_r(tuner.freq+100);
        }
        return TRUE;
    }

    if(current == conf.key_tune_up_5)
    {
        tune(tuner.freq+5);
        return TRUE;
    }

    if(current == conf.key_tune_down_5)
    {
        tune(tuner.freq-5);
        return TRUE;
    }

    if(current == conf.key_tune_down_1000)
    {
        tune_r(tuner.freq-1000);
        return TRUE;
    }

    if(current == conf.key_tune_up_1000)
    {
        tune_r(tuner.freq+1000);
        return TRUE;
    }

    if(current == conf.key_tune_back)
    {
        tune(tuner.prevfreq);
        return TRUE;
    }

    if(current == conf.key_reset)
    {
        tune_r(tuner.freq);
        return TRUE;
    }

    if(current == conf.key_screen)
    {
        gchar t[20], filename[50];
        time_t tt = time(NULL);
        if(conf.utc)
        {
            strftime(t, sizeof(t), "%Y%m%d-%H%M%S", gmtime(&tt));
        }
        else
        {
            strftime(t, sizeof(t), "%Y%m%d-%H%M%S", localtime(&tt));
        }
        if(tuner.pi!=-1)
        {
            g_snprintf(filename, sizeof(filename), "./screenshots/%s-%d-%04X.png", t, tuner.freq, tuner.pi);
        }
        else
        {
            g_snprintf(filename, sizeof(filename), "./screenshots/%s-%d.png", t, tuner.freq);
        }
        GdkPixmap *pixmap = gtk_widget_get_snapshot(gui.window, NULL);
        GdkPixbuf *pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 0, 0, 0, 0, -1, -1);
        g_mkdir("./screenshots/", 0755);
        if(!gdk_pixbuf_save(pixbuf, filename, "png", NULL, NULL))
        {
            dialog_error("Unable to save a screenshot...");
        }
        g_object_unref(G_OBJECT(pixmap));
        g_object_unref(G_OBJECT(pixbuf));
        return TRUE;
    }

    // presets
    if(event->keyval >= GDK_F1 && event->keyval <= GDK_F12)
    {
        gint id = event->keyval-GDK_F1;
        if(event->state & GDK_SHIFT_MASK)
        {
            gchar buff[50];
            conf.presets[id] = tuner.freq;
            g_source_remove(gui.status_timeout);
            g_snprintf(buff, sizeof(buff), "Preset F%d saved: %d kHz", id+1, tuner.freq);
            gtk_label_set_text(GTK_LABEL(gui.l_status), buff);
            gui.status_timeout = g_timeout_add(1000, (GSourceFunc)gui_update_clock, (gpointer)gui.l_status);
            settings_write();
        }
        else
        {
            tune(conf.presets[id]);
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
            tune(atoi(buff2));

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
