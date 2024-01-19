#include <gtk/gtk.h>
#include <string.h>
#ifdef G_OS_WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include "win32.h"
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#endif
#include "rdsspy.h"
#include "ui.h"
#include "conf.h"
#include "tuner-conn.h"

static gint rdsspy_socket = -1;
static gint rdsspy_client = -1;
static gboolean rdsspy_child = 0;
static GPid rdsspy_pid;

static gboolean rdsspy_init(gint);
static gpointer rdsspy_server(gpointer);
static gboolean rdsspy_toggle_button(gpointer);
static void rdsspy_child_watch(GPid, gint, gpointer);


void
rdsspy_toggle()
{
    GError *error = NULL;
    gchar *command[] = { conf.rdsspy_exec, NULL };

    if(rdsspy_is_up())
    {
        if(rdsspy_child)
        {
#ifdef G_OS_WIN32
            TerminateProcess(rdsspy_pid, 0);
#else
            kill(rdsspy_pid, SIGTERM);
#endif
        }
        else
        {
            rdsspy_stop();
        }
    }
    else
    {
        if(rdsspy_init(conf.rdsspy_port) && conf.rdsspy_run && strlen(conf.rdsspy_exec))
        {
            if(!g_spawn_async(NULL, command, NULL, G_SPAWN_DO_NOT_REAP_CHILD, 0, NULL, &rdsspy_pid, &error))
            {
                ui_dialog(ui.window,
                          GTK_MESSAGE_ERROR,
                          "RDS Spy link",
                          "%s",
                          error->message);
                g_error_free(error);
            }
            else
            {
                rdsspy_child = TRUE;
                g_child_watch_add(rdsspy_pid, (GChildWatchFunc)rdsspy_child_watch, NULL);
            }
        }
    }
}

gboolean
rdsspy_is_up()
{
    return (rdsspy_socket >= 0);
}

gboolean
rdsspy_is_connected()
{
    return (rdsspy_socket >= 0 && rdsspy_client >= 0);
}

void
rdsspy_stop()
{
    shutdown(rdsspy_socket, 2);
    closesocket(rdsspy_socket);
    if(rdsspy_client >= 0)
        shutdown(rdsspy_client, 2);
}

static gboolean
rdsspy_init(gint port)
{
    rdsspy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(rdsspy_socket < 0)
    {
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "RDS Spy link",
                  "rdsspy_init: socket");
        return FALSE;
    }

#ifndef G_OS_WIN32
    int on = 1;
    if(setsockopt(rdsspy_socket, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) < 0)
    {
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "RDS Spy link",
                  "rdsspy_init: SO_REUSEADDR");
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
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "RDS Spy link",
                  "Failed to bind to a port: %d.\nIt may be already in use by another application.",
                  port);
        closesocket(rdsspy_socket);
        rdsspy_socket = -1;
        rdsspy_toggle_button(GINT_TO_POINTER(FALSE));
        return FALSE;
    }
    listen(rdsspy_socket, 4);

    rdsspy_toggle_button(GINT_TO_POINTER(TRUE));
    g_thread_unref(g_thread_new("rdsspy", rdsspy_server, NULL));
    return TRUE;
}

static gpointer
rdsspy_server(gpointer nothing)
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

static gboolean
rdsspy_toggle_button(gpointer is_active)
{
    g_signal_handlers_block_by_func(G_OBJECT(ui.b_rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_rdsspy), GPOINTER_TO_INT(is_active));
    g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_rdsspy), GINT_TO_POINTER(rdsspy_toggle), NULL);
    return FALSE;
}

static void
rdsspy_child_watch(GPid     pid,
                   gint     status,
                   gpointer data)
{
    rdsspy_child = FALSE;
    g_spawn_close_pid(pid);
    if(conf.rdsspy_run && rdsspy_is_up())
        rdsspy_stop();
}

void
rdsspy_reset()
{
    if(!rdsspy_is_connected())
        return;

    gchar out[15];
    g_snprintf(out, sizeof(out), "G:\r\nRESET\r\n\r\n");
    send(rdsspy_client, out, strlen(out), 0);
}

void
rdsspy_send(guint *data,
            guint  errors)
{
    if(!rdsspy_is_connected())
        return;

    gchar groups[4][5];
    gchar out[25];

    /* Block A */
    if ((errors & 192) == 0)
    {
        g_snprintf(groups[0], 5, "%04X", data[0]);
    }
    else
    {
        g_snprintf(groups[0], 5, "----");
    }

    /* Block B */
    if((errors & 48) == 0)
    {
        g_snprintf(groups[1], 5, "%04X", data[1]);
    }
    else
    {
        g_snprintf(groups[1], 5, "----");
    }

    /* Block C */
    if((errors & 12) == 0)
    {
        g_snprintf(groups[2], 5, "%04X", data[2]);
    }
    else
    {
        g_snprintf(groups[2], 5, "----");
    }

    /* Block D */
    if((errors & 3) == 0)
    {
        g_snprintf(groups[3], 5, "%04X", data[3]);
    }
    else
    {
        g_snprintf(groups[3], 5, "----");
    }

    g_snprintf(out, sizeof(out), "G:\r\n%s%s%s%s\r\n\r\n", groups[0], groups[1], groups[2], groups[3]);
    send(rdsspy_client, out, strlen(out), 0);
}
