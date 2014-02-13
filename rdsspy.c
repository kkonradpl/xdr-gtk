#include <gtk/gtk.h>
#include <string.h>
#ifdef G_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include "rdsspy.h"
#include "gui.h"
#include "menu.h"
#include "settings.h"
#include "pattern.h"
#include "connection.h"
#include "graph.h"

gint rdsspy_socket = -1;
gint rdsspy_client = -1;

void rdsspy_toggle()
{
    if(rdsspy_socket < 0)
    {
        rdsspy_init(conf.rds_spy_port);
    }
    else
    {
        rdsspy_stop();
    }
}

gboolean rdsspy_is_up()
{
    if(rdsspy_socket > 0)
    {
        return TRUE;
    }
    return FALSE;
}

gboolean rdsspy_is_connected()
{
    if(rdsspy_socket > 0 && rdsspy_client > 0)
    {
        return TRUE;
    }
    return FALSE;
}

void rdsspy_stop()
{
    shutdown(rdsspy_socket, 2);
#ifdef G_OS_WIN32
    closesocket(rdsspy_socket);
#else
    close(rdsspy_socket);
#endif
    if(rdsspy_client > 0)
    {
        shutdown(rdsspy_client, 2);
    }
}

void rdsspy_init(int port)
{
    rdsspy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(rdsspy_socket < 0)
    {
        dialog_error("rdsspy_init: socket");
        return;
    }

#ifndef G_OS_WIN32
    int on = 1;
    if(setsockopt(rdsspy_socket, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) < 0)
    {
        dialog_error("rdsspy_init: SO_REUSEADDR");
    }
#endif

    struct sockaddr_in addr;
    memset((char*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if(bind(rdsspy_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        dialog_error("RDS Spy link:\nFailed to bind to a port.\nIs it already in use by another app instance?");
#ifdef G_OS_WIN32
        closesocket(rdsspy_socket);
#else
        close(rdsspy_socket);
#endif
        rdsspy_socket = -1;
        g_signal_handlers_block_by_func(G_OBJECT(gui.menu_items.rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gui.menu_items.rdsspy), FALSE);
        g_signal_handlers_unblock_by_func(G_OBJECT(gui.menu_items.rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);
        return;
    }
    listen(rdsspy_socket, 4);

    g_signal_handlers_block_by_func(G_OBJECT(gui.menu_items.rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gui.menu_items.rdsspy), TRUE);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.menu_items.rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);

    g_thread_new("thread_rdsspy", rdsspy_server, NULL);
}

gpointer rdsspy_server(gpointer nothing)
{
    char c;

    while((rdsspy_client = accept(rdsspy_socket, (struct sockaddr*)NULL, NULL)) >= 0)
    {
        fd_set input;
        FD_ZERO(&input);
        FD_SET(rdsspy_client, &input);
        while(select(rdsspy_client+1, &input, NULL, NULL, NULL) > 0)
        {
            if(recv(rdsspy_client, &c, 1, 0) <= 0)
            {
                break;
            }
        }
#ifdef G_OS_WIN32
        closesocket(rdsspy_client);
#else
        close(rdsspy_client);
#endif
        rdsspy_client = -1;
    }
    rdsspy_socket = -1;
    g_idle_add_full(G_PRIORITY_DEFAULT, rdsspy_checkbox_disable, NULL, NULL);
    return NULL;
}

gboolean rdsspy_checkbox_disable(gpointer nothing)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.menu_items.rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gui.menu_items.rdsspy), FALSE);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.menu_items.rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);
    return FALSE;
}

void rdsspy_reset()
{
    if(!rdsspy_is_connected())
    {
        return;
    }

    gchar out[15];
    g_snprintf(out, sizeof(out), "G:\r\nRESET\r\n\r\n");
    send(rdsspy_client, out, strlen(out), 0);
}

void rdsspy_send(gint pi, gchar *msg, guint errors)
{
    if(!rdsspy_is_connected())
    {
        return;
    }

    gchar groups[4][5];

    // 1st block (PI code)
    if(rssi[rssi_pos].rds)
    {
        g_snprintf(groups[0], 5, "%04X", pi);
    }
    else
    {
        g_snprintf(groups[0], 5, "----");
    }

    // 2nd block
    if((errors&3) == 0)
    {
        strncpy(groups[1], msg, 4);
        groups[1][4] = 0;
    }
    else
    {
        g_snprintf(groups[1], 5, "----");
    }

    // 3rd block
    if((errors&12) == 0) //
    {
        strncpy(groups[2], msg+4, 4);
        groups[2][4] = 0;
    }
    else
    {
        g_snprintf(groups[2], 5, "----");
    }

    // 4th block
    if((errors&48) == 0)
    {
        strncpy(groups[3], msg+8, 4);
        groups[3][4] = 0;
    }
    else
    {
        g_snprintf(groups[3], 5, "----");
    }

    gchar out[25];
    g_snprintf(out, sizeof(out), "G:\r\n%s%s%s%s\r\n\r\n", groups[0], groups[1], groups[2], groups[3]);
    send(rdsspy_client, out, strlen(out), 0);
}
