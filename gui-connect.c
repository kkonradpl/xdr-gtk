#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include "gui-connect.h"
#include "gui.h"
#include "gui-update.h"
#include "settings.h"
#include "tuner.h"
#include "version.h"
#include "connection.h"
#include "tuner.h"
#include "gui-tuner.h"
#include "sig.h"

GtkWidget *dialog, *content;
GtkWidget *r_serial, *c_serial;
GtkWidget *r_tcp;
GtkWidget *box_tcp1, *l_host, *e_host, *l_port, *e_port;
GtkWidget *box_tcp2, *l_password, *e_password, *c_password;
GtkWidget *l_status;
GtkWidget *box_button, *b_connect, *b_cancel;
GtkListStore *ls_host;

conn_t* connecting;
gboolean wait_for_tuner;

void connection_toggle()
{
    if(tuner.thread)
    {
        /* The tuner is connected, shutdown it */
        tuner_write("X");
        /* Lock the connection button until the thread ends */
        gtk_widget_set_sensitive(gui.b_connect, FALSE);
        g_usleep(100000);
        tuner.thread = FALSE;
        return;
    }

    /* Drop current rotator state */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), FALSE);

    /* Display connection dialog */
    connection_dialog(FALSE);
}

void connection_dialog(gboolean autoconnect)
{
    gint i;
    connecting = NULL;
    wait_for_tuner = FALSE;

    dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(dialog), "Connect");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gui.window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    g_signal_connect(dialog, "destroy", G_CALLBACK(connection_dialog_destroy), NULL);

    content = gtk_vbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(dialog), content);

    r_serial = gtk_radio_button_new_with_label(NULL, "Serial port");
    gtk_box_pack_start(GTK_BOX(content), r_serial, TRUE, TRUE, 2);
    c_serial = gtk_combo_box_text_new();
#ifdef G_OS_WIN32
    gchar tmp[10];
    for(i=1; i<=20; i++)
    {
        g_snprintf(tmp, sizeof(tmp), "COM%d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_serial), tmp);
        if(g_ascii_strcasecmp(tmp, conf.serial) == 0)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(c_serial), i-1);
        }
    }
#else
    struct dirent *dir;
    DIR *d = opendir("/dev");
    i=0;
    if(d)
    {
        while((dir = readdir(d)))
        {
#ifdef __APPLE__
            if(!strncmp(dir->d_name, "cu.usbserial", 12))
#else
            if(!strncmp(dir->d_name, "ttyUSB", 6) || !strncmp(dir->d_name, "ttyACM", 6) || !strncmp(dir->d_name, "ttyS", 4))
#endif
            {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(c_serial), dir->d_name);
                if(g_ascii_strcasecmp(dir->d_name, conf.serial) == 0)
                {
                    gtk_combo_box_set_active(GTK_COMBO_BOX(c_serial), i);
                }
                i++;
            }
        }
        closedir(d);
    }
#endif
    g_signal_connect(c_serial, "changed", G_CALLBACK(connection_dialog_select), r_serial);
    gtk_box_pack_start(GTK_BOX(content), c_serial, TRUE, TRUE, 0);

    r_tcp = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(r_serial), "TCP/IP");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r_tcp), conf.network);
    gtk_box_pack_start(GTK_BOX(content), r_tcp, TRUE, TRUE, 2);
    box_tcp1 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), box_tcp1);
    l_host = gtk_label_new("Host:");
    gtk_box_pack_start(GTK_BOX(box_tcp1), l_host, TRUE, FALSE, 1);
    ls_host = gtk_list_store_new(1, G_TYPE_STRING);
    if(conf.host)
    {
        for(i=0; conf.host[i]; i++)
        {
            GtkTreeIter iter;
            gtk_list_store_append(ls_host, &iter);
            gtk_list_store_set(ls_host, &iter, 0, conf.host[i], -1);
        }
    }
    e_host = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(ls_host));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(e_host), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(e_host), 0);
    g_signal_connect(e_host, "changed", G_CALLBACK(connection_dialog_select), r_tcp);
    gtk_box_pack_start(GTK_BOX(box_tcp1), e_host, TRUE, FALSE, 1);
    l_port = gtk_label_new("Port:");
    gtk_box_pack_start(GTK_BOX(box_tcp1), l_port, TRUE, FALSE, 1);
    e_port = gtk_entry_new_with_max_length(5);
    gtk_entry_set_width_chars(GTK_ENTRY(e_port), 5);
    gchar* s_port = g_strdup_printf("%d", conf.port);
    gtk_entry_set_text(GTK_ENTRY(e_port), s_port);
    g_free(s_port);
    g_signal_connect(e_port, "changed", G_CALLBACK(connection_dialog_select), r_tcp);
    gtk_box_pack_start(GTK_BOX(box_tcp1), e_port, TRUE, FALSE, 1);

    box_tcp2 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), box_tcp2);
    l_password = gtk_label_new("Password:");
    gtk_box_pack_start(GTK_BOX(box_tcp2), l_password, FALSE, FALSE, 1);
    e_password = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(e_password), 20);
    gtk_entry_set_text(GTK_ENTRY(e_password), conf.password);
    gtk_entry_set_visibility(GTK_ENTRY(e_password), FALSE);
    g_signal_connect(e_password, "changed", G_CALLBACK(connection_dialog_select), r_tcp);
    gtk_box_pack_start(GTK_BOX(box_tcp2), e_password, TRUE, TRUE, 1);
    c_password = gtk_check_button_new_with_label("Keep");
    if(conf.password && strlen(conf.password))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_password), TRUE);
    }
    g_signal_connect(c_password, "toggled", G_CALLBACK(connection_dialog_select), r_tcp);
    gtk_box_pack_start(GTK_BOX(box_tcp2), c_password, FALSE, FALSE, 1);

    l_status = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(content), l_status, FALSE, FALSE, 1);

    box_button = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(box_button), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(box_button), 5);
    gtk_box_pack_start(GTK_BOX(content), box_button, FALSE, FALSE, 5);
    b_connect = gtk_button_new_from_stock(GTK_STOCK_OK);
    g_signal_connect(dialog, "key-press-event", G_CALLBACK(connection_dialog_key), b_connect);
    g_signal_connect(b_connect, "clicked", G_CALLBACK(connection_dialog_connect), NULL);
    gtk_container_add(GTK_CONTAINER(box_button), b_connect);
    b_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect_swapped(b_cancel, "clicked", G_CALLBACK(gtk_widget_destroy), dialog);
    gtk_container_add(GTK_CONTAINER(box_button), b_cancel);

#ifdef G_OS_WIN32
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.b_ontop)))
    {
        gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    }
#endif

    gtk_widget_show_all(dialog);
    gtk_widget_hide(l_status);
    connect_button(FALSE);
    if(autoconnect)
    {
        gtk_button_clicked(GTK_BUTTON(b_connect));
    }
}

void connection_dialog_destroy(GtkWidget *widget, gpointer data)
{
    if(connecting)
    {
        /* Waiting for socket connection, force its thread to end */
        connecting->canceled = TRUE;
        connecting = NULL;
    }
    if(wait_for_tuner)
    {
        /* Waiting for tuner, force its thread to end */
        tuner.thread = FALSE;
        wait_for_tuner = FALSE;
    }
}

void connection_dialog_select(GtkWidget *widget, gpointer data)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
}

void connection_dialog_connect(GtkWidget *widget, gpointer data)
{
    const gchar* hostname = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(e_host))));
    const gchar* port = gtk_entry_get_text(GTK_ENTRY(e_port));
    const gchar* password = gtk_entry_get_text(GTK_ENTRY(e_password));

    connection_dialog_unlock(FALSE);
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(r_serial)))
    {
        /* Serial port */
        gchar* serial = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(c_serial));
        if(serial)
        {
            g_snprintf(gui.window_title, 100, "%s / %s", APP_NAME, serial);
            settings_update_string(&conf.serial, serial);
            conf.network = FALSE;
            settings_write();

            connection_serial_state(open_serial(serial));
            g_free(serial);
        }
    }
    else if(strlen(hostname) && atoi(port) > 0)
    {
        /* Network */
        connecting = conn_new(hostname, port, password);
        g_snprintf(gui.window_title, 100, "%s / %s", APP_NAME, hostname);

        settings_add_host(hostname);
        conf.port = atoi(port);
        conf.network = TRUE;
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_password)))
        {
            settings_update_string(&conf.password, password);
        }
        settings_write();

        g_thread_unref(g_thread_new("open_socket", open_socket, connecting));
    }
}

void connection_dialog_status(const gchar* string)
{
    gchar *markup;
    gtk_widget_show(l_status);
    markup = g_markup_printf_escaped("<b>%s</b>", string);
    gtk_label_set_markup(GTK_LABEL(l_status), markup);
    g_free(markup);
}

void connection_dialog_unlock(gboolean value)
{
    gtk_widget_set_sensitive(r_serial, value);
    gtk_widget_set_sensitive(c_serial, value);
    gtk_widget_set_sensitive(r_tcp, value);
    gtk_widget_set_sensitive(e_host, value);
    gtk_widget_set_sensitive(e_port, value);
    gtk_widget_set_sensitive(e_password, value);
    gtk_widget_set_sensitive(c_password, value);
    gtk_widget_set_sensitive(b_connect, value);
}

gboolean connection_dialog_key(GtkWidget* widget, GdkEventKey* event, gpointer button)
{
    guint current = gdk_keyval_to_upper(event->keyval);
    if(current == GDK_Escape)
    {
        gtk_widget_destroy(widget);
        return TRUE;
    }
    else if(current == GDK_Return)
    {
        gtk_button_clicked(GTK_BUTTON(button));
        return TRUE;
    }
    return FALSE;
}

void connection_serial_state(gint result)
{
    switch(result)
    {
    case CONN_SUCCESS:
        connection_dialog_connected(MODE_SERIAL);
        return;

    case CONN_SERIAL_FAIL_OPEN:
        connection_dialog_status("Unable to open the serial port.");
        break;

    case CONN_SERIAL_FAIL_PARM_R:
        connection_dialog_status("Unable to read serial port parameters.");
        break;

    case CONN_SERIAL_FAIL_PARM_W:
        connection_dialog_status("Unable to set serial port parameters.");
        break;

    case CONN_SERIAL_FAIL_SPEED:
        connection_dialog_status("Unable to set serial port speed.");
        break;
    }
    connection_dialog_unlock(TRUE);
}

void connection_dialog_connected(gint mode)
{
    gtk_window_set_title(GTK_WINDOW(gui.window), gui.window_title);
    signal_clear();
    connection_dialog_status("Waiting for tuner...");
    gtk_widget_set_sensitive(gui.b_connect, FALSE);

    tuner.ready = FALSE;
    tuner.thread = TRUE;
    wait_for_tuner = TRUE;
    g_thread_unref(g_thread_new("tuner", tuner_read, NULL));

    while(!tuner.ready && tuner.thread)
    {
        while(gtk_events_pending())
        {
            gtk_main_iteration();
        }
        g_usleep(33000);
    }

    if(!tuner.thread)
    {
        if(wait_for_tuner)
        {
            connection_dialog_unlock(TRUE);
            connection_dialog_status("Connection has been unexpectedly closed.");
        }
        return;
    }

    if(mode == MODE_SERIAL)
    {
        tuner_set_volume();
        tuner_set_squelch();
        tuner_set_frequency(tuner.freq);
        tuner_set_mode();
        tuner_set_agc();
        tuner_set_bandwidth();
        tuner_set_deemphasis();
        tuner_set_gain();
        tuner_set_antenna();
    }

    connect_button(TRUE);
    gtk_widget_set_sensitive(gui.b_connect, TRUE);
    wait_for_tuner = FALSE;
    gtk_widget_destroy(dialog);
}

gboolean connection_socket_callback(gpointer ptr)
{
    /* Gets final state of conn_t ptr and frees up the memory */
    conn_t* data = (conn_t*)ptr;
    if(data->canceled && data->state == CONN_SUCCESS)
    {
        closesocket(data->socketfd);
    }
    else if(!data->canceled)
    {
        connecting = NULL;
        switch(data->state)
        {
        case CONN_SUCCESS:
#ifdef G_OS_WIN32
            tuner.socket = data->socketfd;
#else
            tuner.serial = data->socketfd;
#endif
            connection_dialog_connected(MODE_SOCKET);
            break;

        case CONN_SOCKET_FAIL_RESOLV:
            connection_dialog_status("Unable to resolve the hostname.");
            connection_dialog_unlock(TRUE);
            break;

        case CONN_SOCKET_FAIL_CONN:
            connection_dialog_status("Unable to connect to a server.");
            connection_dialog_unlock(TRUE);
            break;

        case CONN_SOCKET_FAIL_AUTH:
            connection_dialog_status("Authentication error.");
            connection_dialog_unlock(TRUE);
            break;

        case CONN_SOCKET_FAIL_WRITE:
            connection_dialog_status("Socket write error.");
            connection_dialog_unlock(TRUE);
            break;
        }
    }

    conn_free(data);
    return FALSE;
}

gboolean connection_socket_callback_info(gpointer ptr)
{
    /* Like connection_socket_callback but does not free up the conn_t pointer */
    conn_t* data = (conn_t*)ptr;
    if(!data->canceled)
    {
        switch(data->state)
        {
        case CONN_SOCKET_STATE_RESOLV:
            connection_dialog_status("Resolving hostname...");
            break;

        case CONN_SOCKET_STATE_CONN:
            connection_dialog_status("Connecting...");
            break;

        case CONN_SOCKET_STATE_AUTH:
            connection_dialog_status("Authenticating...");
            break;
        }
    }
    return FALSE;
}

gboolean connection_socket_auth_fail(gpointer ptr)
{
    if(wait_for_tuner)
    {
        connection_dialog_status("Incorrect password.");
        connection_dialog_unlock(TRUE);
    }
    return FALSE;
}
