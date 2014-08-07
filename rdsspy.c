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
#include "settings.h"
#include "pattern.h"
#include "connection.h"
#include "sig.h"

gint rdsspy_socket = -1;
gint rdsspy_client = -1;

void rdsspy_toggle()
{
    GError* error = NULL;
    gchar* command[] = { conf.rds_spy_command, NULL };

    if(rdsspy_is_up())
    {
        rdsspy_stop();
    }
    else
    {
        if(rdsspy_init(conf.rds_spy_port) && conf.rds_spy_run && strlen(conf.rds_spy_command))
        {
            if(!g_spawn_async(NULL, command, NULL, 0, 0, NULL, NULL, &error))
            {
                dialog_error("Unable to start RDS Spy:\n%s", error->message);
                g_error_free(error);
            }
        }
    }
}

gboolean rdsspy_is_up()
{
    return (rdsspy_socket >= 0);
}

gboolean rdsspy_is_connected()
{
    return (rdsspy_socket >= 0 && rdsspy_client >= 0);
}

void rdsspy_stop()
{
    shutdown(rdsspy_socket, 2);
    closesocket(rdsspy_socket);
    if(rdsspy_client >= 0)
    {
        shutdown(rdsspy_client, 2);
    }
}

gboolean rdsspy_init(gint port)
{
    rdsspy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(rdsspy_socket < 0)
    {
        dialog_error("rdsspy_init: socket");
        return FALSE;
    }

#ifndef G_OS_WIN32
    int on = 1;
    if(setsockopt(rdsspy_socket, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) < 0)
    {
        dialog_error("rdsspy_init: SO_REUSEADDR");
        return FALSE;
    }
#endif

    struct sockaddr_in addr;
    memset((char*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if(bind(rdsspy_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        dialog_error("RDS Spy link:\nFailed to bind to a port: %d.\nIt may be already in use by another application.", port);
        closesocket(rdsspy_socket);
        rdsspy_socket = -1;
        rdsspy_toggle_button(GINT_TO_POINTER(FALSE));
        return FALSE;
    }
    listen(rdsspy_socket, 4);

    rdsspy_toggle_button(GINT_TO_POINTER(TRUE));
    g_thread_new("rdsspy", rdsspy_server, NULL);
    return TRUE;
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
        closesocket(rdsspy_client);
        rdsspy_client = -1;
    }
    rdsspy_socket = -1;
    g_idle_add(rdsspy_toggle_button, GINT_TO_POINTER(FALSE));
    return NULL;
}

gboolean rdsspy_toggle_button(gpointer is_active)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.b_rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_rdsspy), GPOINTER_TO_INT(is_active));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.b_rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);
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
    gchar out[25];

    // 1st block (PI code)
    if(s.data[s.pos].rds)
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
    if((errors&12) == 0)
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

    g_snprintf(out, sizeof(out), "G:\r\n%s%s%s%s\r\n\r\n", groups[0], groups[1], groups[2], groups[3]);
    send(rdsspy_client, out, strlen(out), 0);
}
