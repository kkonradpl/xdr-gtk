#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"
#include "connection.h"
#include "graph.h"
#include "settings.h"
#include "scan.h"
#include "menu.h"
#include "pattern.h"
#ifdef G_OS_WIN32
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
SOCKET serial_socket = INVALID_SOCKET;
#else
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#endif


gint freq = 87500, prevfreq;
gchar daa;
gint prevpi, pi = -1;
gchar ps_data[9], rt_data[2][65];
gboolean ps_available;
short prevpty, prevtp, prevta, prevms;
gint64 rds_timer;
gfloat max_signal;
gint online = -1;

volatile gboolean thread = FALSE;
volatile gboolean ready = FALSE;

short filters[] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 29, 2, 28, 1, 26, 0, 24, 23, 22, 21, 20, 19, 18, 17, 16, 31, -1};
short filters_n = sizeof(filters)/sizeof(short);

void connection_dialog()
{
    gtk_widget_set_sensitive(gui.menu_items.connect, FALSE);
    if(thread)
    {
        xdr_write("X");
        g_usleep(100000);
        thread = FALSE;
        return;
    }

    online = -1;
    gint conn_mode = connection();
    if(!conn_mode)
    {
        gui_clear_power_off(NULL);
        return;
    }

    gtk_window_set_title(GTK_WINDOW(gui.window), gui.window_title);
    graph_clear();

    ready = FALSE;

    g_thread_new("thread", read_thread, NULL);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW(gui.window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

    GtkWidget *hbox = gtk_hbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    gtk_container_add(GTK_CONTAINER(content), hbox);

    GtkWidget *spinner = gtk_spinner_new();
    gtk_spinner_start(GTK_SPINNER(spinner));
    gtk_box_pack_start(GTK_BOX(hbox), spinner, TRUE, FALSE, 8);
    gtk_widget_set_size_request(spinner, 80, 80);
    gtk_widget_show_all(dialog);
    gtk_widget_show(dialog);

    gint64 timer = g_get_monotonic_time();
    while(!ready)
    {
        if((g_get_monotonic_time() - timer) >= 10000000)
        {
            gtk_widget_destroy(dialog);
            dialog_error("Unable to start the tuner.");
            thread = FALSE;
            return;
        }
        while(gtk_events_pending())
        {
            gtk_main_iteration();
        }
        g_usleep(33000);
    }

    gtk_widget_destroy(dialog);

    if(conn_mode == 1)
    {
        tune(freq);
        if(conf.mode == MODE_FM)
        {
            xdr_write("M0");
        }
        if(conf.mode == MODE_AM)
        {
            xdr_write("M1");
        }
        tty_change_agc();
        tty_change_bandwidth();
        tty_change_deemphasis();
        tty_change_gain();
        tty_change_ant();
    }
    gtk_menu_item_set_label(GTK_MENU_ITEM(gui.menu_items.connect), "Disconnect");
    gtk_widget_set_sensitive(gui.menu_items.connect, TRUE);
}

gboolean connection()
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("XDR-GTK", GTK_WINDOW(gui.window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
    GtkWidget *d_boxh = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), d_boxh);

    GtkWidget *d_box = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(d_boxh), d_box);

    GtkWidget* local = gtk_radio_button_new_with_label(NULL, "Local connection");
    gtk_box_pack_start(GTK_BOX(d_box), local, TRUE, TRUE, 2);

    GtkWidget *d_cb = gtk_combo_box_new_text();
    gint i;
#ifdef G_OS_WIN32
    gchar tmp[10];
    for(i=1; i<=20; i++)
    {
        g_sprintf(tmp, "COM%d", i);
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
    gtk_box_pack_start(GTK_BOX(d_box), d_cb, TRUE, TRUE, 0);

    GtkWidget *remote = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(local), "Network connection");
    if(conf.network)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote), TRUE);
    }
    gtk_box_pack_start(GTK_BOX(d_box), remote, TRUE, TRUE, 2);

    GtkWidget *d_boxh2 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(d_box), d_boxh2);

    GtkWidget *e_host = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(e_host), conf.host);
    gtk_box_pack_start(GTK_BOX(d_boxh2), e_host, TRUE, FALSE, 7);
    GtkWidget *e_port = gtk_entry_new_with_max_length(5);
    gtk_entry_set_width_chars(GTK_ENTRY(e_port), 5);
    gchar* port = g_strdup_printf("%d", conf.port);
    gtk_entry_set_text(GTK_ENTRY(e_port), port);
    g_free(port);
    gtk_box_pack_start(GTK_BOX(d_boxh2), e_port, TRUE, FALSE, 7);

    gtk_widget_show_all(dialog);

    gint d_result;
#ifdef G_OS_WIN32
    // workaround for windows to keep dialog over main window
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gui.menu_items.alwaysontop)))
    {
        gtk_window_set_keep_above(GTK_WINDOW(gui.window), FALSE);
    }
    d_result = gtk_dialog_run(GTK_DIALOG(dialog));
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(gui.menu_items.alwaysontop)))
    {
        gtk_window_set_keep_above(GTK_WINDOW(gui.window), TRUE);
    }
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
            g_snprintf(gui.window_title, 100, "XDR-GTK / %s", serial);

            if(conf.serial)
            {
                g_free(conf.serial);
            }
            conf.serial = g_strdup(serial);
            conf.network = 0;
            settings_write();

            gchar serial_fn[32];
#ifdef G_OS_WIN32
            g_snprintf((gchar*)serial_fn, sizeof(serial_fn), "\\\\.\\%s", serial);
#else
            g_snprintf((gchar*)serial_fn, sizeof(serial_fn), "/dev/%s", serial);
#endif
            result = open_serial(serial_fn);
        }
    }
    else
    {
        const gchar* destination = gtk_entry_get_text(GTK_ENTRY(e_host));
        gint port = atoi(gtk_entry_get_text(GTK_ENTRY(e_port)));

        g_snprintf(gui.window_title, 100, "XDR-GTK / %s", destination);

        if(conf.host)
        {
            g_free(conf.host);
        }
        conf.host = g_strdup(destination);
        conf.port = port;
        conf.network = 1;
        settings_write();

        result = open_socket(destination, port);
    }

    if(!result)
    {
        gui_clear_power_off(NULL);
    }
    gtk_widget_destroy(dialog);
    return result;

}

gint open_serial(gchar* tty_name)
{
#ifdef G_OS_WIN32
    serial = CreateFile(tty_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if(serial == INVALID_HANDLE_VALUE)
    {
        dialog_error("Unable to open the serial port.");
        return 0;
    }
    DCB dcbSerialParams = {0};
    if(!GetCommState(serial, &dcbSerialParams))
    {
        dialog_error("Unable to read serial port parameters.");
        return 0;
    }
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if(!SetCommState(serial, &dcbSerialParams))
    {
        dialog_error("Unable to set serial port parameters.");
        return 0;
    }
#else
    serial = open(tty_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if(serial < 0)
    {
        dialog_error("Unable to open the serial port.");
        return 0;
    }
    fcntl(serial, F_SETFL, 0);
    tcflush(serial, TCIOFLUSH);

    struct termios options;
    if(tcgetattr(serial, &options))
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
    if(tcsetattr(serial, TCSANOW, &options))
    {
        dialog_error("Unable to set serial port parameters.");
        return 0;
    }
#endif
    return 1;
}

gint open_socket(const gchar* destination, guint port)
{
#ifdef G_OS_WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData))
    {
        dialog_error("Unable to initialize Winsock");
        return 0;
    }
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    gchar cport[6];
    g_snprintf(cport, sizeof(cport), "%d", port);
    if(getaddrinfo(destination, cport, &hints, &result))
    {
        dialog_error("Unable to resolve the hostname");
        WSACleanup();
        return 0;
    }

    for(ptr=result; ptr != NULL; ptr=ptr->ai_next)
    {
        serial_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(serial_socket == INVALID_SOCKET)
        {
            WSACleanup();
            return 0;
        }

        if(connect(serial_socket, ptr->ai_addr, (gint)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(serial_socket);
            serial_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if(serial_socket == INVALID_SOCKET)
    {
        dialog_error("Unable to connect to server.");
        WSACleanup();
        return 0;
    }
#else
    struct sockaddr_in serv_addr;
    struct hostent *server;
    serial = socket(AF_INET, SOCK_STREAM, 0);
    if(!(server = gethostbyname(destination)))
    {
        dialog_error("Unable to resolve hostname.");
        return 0;
    }
    bzero((gchar *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((gchar *)server->h_addr, (gchar *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    if(connect(serial, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        dialog_error("Unable to connect to server.");
        return 0;
    }
#endif
    return 2;
}

void xdr_write(gchar* command)
{
    if(thread)
    {
        gchar *tmp = g_strdup(command);
        gint len = strlen(command);
        tmp[len++] = '\n';
#if DEBUG_WRITE
        g_print("write: %s\n", command);
#endif
#ifdef G_OS_WIN32
        if(serial_socket != INVALID_SOCKET)
        {
            // network connection
            send(serial_socket, tmp, len, 0);
        }
        else
        {
            // serial connection
            OVERLAPPED osWrite = {0};
            DWORD dwWritten;

            osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if(osWrite.hEvent == NULL)
            {
                g_free(tmp);
                return;
            }

            if (!WriteFile(serial, tmp, len, &dwWritten, &osWrite))
            {
                if (GetLastError() == ERROR_IO_PENDING)
                {
                    if(WaitForSingleObject(osWrite.hEvent, INFINITE) == WAIT_OBJECT_0)
                    {
                        GetOverlappedResult(serial, &osWrite, &dwWritten, FALSE);
                    }
                }
            }
            CloseHandle(osWrite.hEvent);
        }
#else
        write(serial, tmp, len);
#endif
        g_free(tmp);
    }
}

void tune(gint newfreq)
{
    gchar freq_text[10];
    gint i;

    if(conf.ant_switching)
    {
        for(i=0; i<ANTENNAS; i++)
        {
            if(newfreq >= conf.ant_start[i] && newfreq <= conf.ant_stop[i])
            {
                if(gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_ant)) == i)
                {
                    break;
                }
                gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_ant), i);
                break;
            }
        }
    }
    g_snprintf(freq_text, sizeof(freq_text), "T%d", newfreq);
    xdr_write(freq_text);
}
