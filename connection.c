#define _WIN32_WINNT 0x0501
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "gui.h"
#include "gui-tuner.h"
#include "gui-update.h"
#include "connection.h"
#include "tuner.h"
#include "graph.h"
#include "settings.h"
#include "scan.h"
#include "pattern.h"
#include "sig.h"
#include "version.h"
#ifdef G_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "win32.h"
#else
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#endif

void connection_dialog()
{
    gint conn_mode;

    gtk_widget_set_sensitive(gui.b_connect, FALSE);
    if(tuner.thread)
    {
        tuner_write("X");
        g_usleep(100000);
        tuner.thread = FALSE;
        return;
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), FALSE);

    if(!(conn_mode = connection()))
    {
        gui_clear_power_off();
        return;
    }

    gtk_window_set_title(GTK_WINDOW(gui.window), gui.window_title);
    signal_clear();

    tuner.ready = FALSE;
    tuner.thread = TRUE;
    g_thread_new("tuner", tuner_read, NULL);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW(gui.window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 2);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_vbox_new(FALSE, 3);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    GtkWidget *spinner = gtk_spinner_new();
    gtk_spinner_start(GTK_SPINNER(spinner));
    gtk_box_pack_start(GTK_BOX(vbox), spinner, TRUE, FALSE, 8);
    gtk_widget_set_size_request(spinner, 100, 100);

    GtkWidget *button = gtk_button_new_with_label("Cancel");
    gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_CANCEL, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(button, "clicked", G_CALLBACK(connection_cancel), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0);

#ifdef G_OS_WIN32
    // keep dialog over main window
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.b_ontop)))
    {
        gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    }
#endif

    gtk_widget_show_all(dialog);

    while(!tuner.ready && tuner.thread)
    {
        while(gtk_events_pending())
        {
            gtk_main_iteration();
        }
        g_usleep(33000);
    }

    gtk_widget_destroy(dialog);

    if(!tuner.thread)
    {
        return;
    }

    if(conn_mode == CONN_SERIAL)
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
}

gboolean connection()
{
    GtkWidget *dialog, *content;
    gint i;

    dialog = gtk_dialog_new_with_buttons("Connect", GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    gtk_box_set_spacing(GTK_BOX(content), 3);

    GtkWidget* local = gtk_radio_button_new_with_label(NULL, "Serial port");
    gtk_box_pack_start(GTK_BOX(content), local, TRUE, TRUE, 2);

    GtkWidget *d_cb = gtk_combo_box_new_text();
#ifdef G_OS_WIN32
    gchar tmp[10];
    for(i=1; i<=20; i++)
    {
        g_snprintf(tmp, sizeof(tmp), "COM%d", i);
        gtk_combo_box_append_text(GTK_COMBO_BOX(d_cb), tmp);
        if(g_ascii_strcasecmp(tmp, conf.serial) == 0)
        {
            gtk_combo_box_set_active(GTK_COMBO_BOX(d_cb), i-1);
        }
    }
#else
    struct dirent *dir;
    DIR *d = opendir("/dev");
    if(d)
    {
        i=0;
        while ((dir = readdir(d)))
        {
#ifdef __APPLE__
            if(!strncmp(dir->d_name, "cu.usbserial", 12))
#else
            if(!strncmp(dir->d_name, "ttyUSB", 6) || !strncmp(dir->d_name, "ttyACM", 6) || !strncmp(dir->d_name, "ttyS", 4))
#endif
            {
                gtk_combo_box_append_text(GTK_COMBO_BOX(d_cb), dir->d_name);
                if(g_ascii_strcasecmp(dir->d_name, conf.serial) == 0)
                {
                    gtk_combo_box_set_active(GTK_COMBO_BOX(d_cb), i);
                }
                i++;
            }
        }
    }
    closedir(d);
#endif
    gtk_box_pack_start(GTK_BOX(content), d_cb, TRUE, TRUE, 0);

    GtkWidget *remote = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(local), "TCP/IP");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote), conf.network);
    gtk_box_pack_start(GTK_BOX(content), remote, TRUE, TRUE, 2);

    GtkWidget *d_boxh2 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), d_boxh2);

    GtkWidget *host = gtk_label_new("Host:");
    gtk_box_pack_start(GTK_BOX(d_boxh2), host, TRUE, FALSE, 1);

    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
    if(conf.host)
    {
        for(i=0; conf.host[i]; i++)
        {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, conf.host[i], -1);
        }
    }
    GtkWidget *e_host = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(store));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(e_host), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(e_host), 0);
    gtk_box_pack_start(GTK_BOX(d_boxh2), e_host, TRUE, FALSE, 1);

    GtkWidget *port = gtk_label_new("Port:");
    gtk_box_pack_start(GTK_BOX(d_boxh2), port, TRUE, FALSE, 1);
    GtkWidget *e_port = gtk_entry_new_with_max_length(5);
    gtk_entry_set_width_chars(GTK_ENTRY(e_port), 5);
    gchar* s_port = g_strdup_printf("%d", conf.port);
    gtk_entry_set_text(GTK_ENTRY(e_port), s_port);
    g_free(s_port);
    gtk_box_pack_start(GTK_BOX(d_boxh2), e_port, TRUE, FALSE, 1);

    GtkWidget *d_boxh3 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), d_boxh3);

    GtkWidget *password = gtk_label_new("Password:");
    gtk_box_pack_start(GTK_BOX(d_boxh3), password, FALSE, FALSE, 1);
    GtkWidget *e_password = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(e_password), 15);
    gtk_entry_set_text(GTK_ENTRY(e_password), conf.password);
    gtk_entry_set_visibility(GTK_ENTRY(e_password), FALSE);
    gtk_box_pack_start(GTK_BOX(d_boxh3), e_password, TRUE, TRUE, 1);

    GtkWidget *c_password = gtk_check_button_new_with_label("Keep");
    if(conf.password && strlen(conf.password))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_password), TRUE);
    }
    gtk_box_pack_start(GTK_BOX(d_boxh3), c_password, FALSE, FALSE, 1);

    gtk_widget_show_all(dialog);

    gint d_result;
#ifdef G_OS_WIN32
    d_result = win32_dialog_workaround(GTK_DIALOG(dialog));
#else
    d_result = gtk_dialog_run(GTK_DIALOG(dialog));
#endif

    if(d_result != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return FALSE;
    }

    gint result = 0;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(local)))
    {
        const gchar* serial = gtk_combo_box_get_active_text(GTK_COMBO_BOX(d_cb));
        if(serial)
        {
            g_snprintf(gui.window_title, 100, "%s / %s", APP_NAME, serial);
            settings_update_string(&conf.serial, serial);
            conf.network = 0;
            settings_write();

            result = open_serial(serial);
        }
    }
    else
    {
        const gchar* host = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(e_host))));
        gint port = atoi(gtk_entry_get_text(GTK_ENTRY(e_port)));
        const gchar* password = gtk_entry_get_text(GTK_ENTRY(e_password));

        if(strlen(host))
        {
            g_snprintf(gui.window_title, 100, "%s / %s", APP_NAME, host);

            settings_add_host(host);
            conf.port = port;
            conf.network = 1;

            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(c_password)))
            {
                settings_update_string(&conf.password, password);
            }
            settings_write();

            result = open_socket(host, port, password);
        }

    }

    if(!result)
    {
        gui_clear_power_off(NULL);
    }
    gtk_widget_destroy(dialog);
    return result;

}

void connection_cancel(GtkWidget *widget, gpointer data)
{
    tuner.thread = FALSE;
}

gint open_serial(const gchar* serial_port)
{
    gchar path[32];
#ifdef G_OS_WIN32
    DCB dcbSerialParams = {0};

    g_snprintf(path, sizeof(path), "\\\\.\\%s", serial_port);
    tuner.socket = INVALID_SOCKET;
    tuner.serial = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if(tuner.serial == INVALID_HANDLE_VALUE)
    {
        dialog_error("Unable to open the serial port:\n%s", path);
        return 0;
    }
    if(!GetCommState(tuner.serial, &dcbSerialParams))
    {
        dialog_error("Unable to read serial port parameters.");
        return 0;
    }
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if(!SetCommState(tuner.serial, &dcbSerialParams))
    {
        dialog_error("Unable to set serial port parameters.");
        return 0;
    }
#else
    struct termios options;

    g_snprintf(path, sizeof(path), "/dev/%s", serial_port);
    if((tuner.serial = open(path, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
    {
        dialog_error("Unable to open the serial port:\n%s", path);
        return 0;
    }
    fcntl(tuner.serial, F_SETFL, 0);
    tcflush(tuner.serial, TCIOFLUSH);

    if(tcgetattr(tuner.serial, &options))
    {
        dialog_error("Unable to read serial port parameters.");
        return 0;
    }
    if(cfsetispeed(&options, B115200) || cfsetospeed(&options, B115200))
    {
        dialog_error("Unable to set serial port speed.");
        return 0;
    }
    options.c_iflag &= ~(BRKINT | ICRNL | IXON | IMAXBEL);
    options.c_iflag |= IGNBRK;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN | ECHOK | ECHOCTL | ECHOKE);
    options.c_oflag &= ~(OPOST | ONLCR);
    options.c_oflag |= NOFLSH;
    options.c_cflag |= CS8;
    options.c_cflag &= ~(CRTSCTS);
    if(tcsetattr(tuner.serial, TCSANOW, &options))
    {
        dialog_error("Unable to set serial port parameters.");
        return 0;
    }
#endif
    return CONN_SERIAL;
}

gint open_socket(const gchar* destination, guint port, const gchar* password)
{
    gint s;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    gchar salt[SOCKET_SALT+1];
    GChecksum *sha1;
    gchar* msg;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if(!(server = gethostbyname(destination)))
    {
        dialog_error("Unable to resolve the hostname:\n%s", destination);
        return 0;
    }

    memset((gchar *)&serv_addr, 0, sizeof(serv_addr));
    memmove((gchar *)&serv_addr.sin_addr.s_addr, (gchar *)server->h_addr, server->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(connect(s, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        dialog_error("Unable to connect the server:\n%s:%d", destination, port);
        return 0;
    }

    if(recv(s, salt, SOCKET_SALT+1, 0) != SOCKET_SALT+1) // SOCKET_SALT + \n char
    {
        dialog_error("Auth error.");
        return 0;
    }

    sha1 = g_checksum_new(G_CHECKSUM_SHA1);
    g_checksum_update(sha1, (guchar*)salt, SOCKET_SALT);
    if(strlen(password))
    {
        g_checksum_update(sha1, (guchar*)password, strlen(password));
    }
    msg = g_strdup_printf("%s\n", g_checksum_get_string(sha1));
    g_checksum_free(sha1);

    send(s, msg, strlen(msg), 0);
    g_free(msg);

#ifdef G_OS_WIN32
    tuner.socket = s;
#else
    tuner.serial = s;
#endif

    return CONN_SOCKET;
}
