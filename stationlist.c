#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#ifdef G_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "strsep.h"
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include "stationlist.h"
#include "settings.h"
#include "gui.h"
#include "connection.h"

gint stationlist_socket = -1;
gint stationlist_client = -1;
struct sockaddr_in stationlist_client_addr;

void stationlist_init()
{
    stationlist_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(stationlist_socket < 0)
    {
        dialog_error("stationlist_init: socket");
        return;
    }

#ifndef G_OS_WIN32
    gint on = 1;
    if(setsockopt(stationlist_socket, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) < 0)
    {
        dialog_error("stationlist_init: SO_REUSEADDR");
    }
#endif

    struct sockaddr_in addr;
    memset((char*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(conf.stationlist_server);

    if(bind(stationlist_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        dialog_error("StationList link:\nFailed to bind to a port.\nIt may be already in use by another application.");
        closesocket(stationlist_socket);
        stationlist_socket = -1;
        return;
    }

    stationlist_client = socket(AF_INET, SOCK_DGRAM, 0);
    if(stationlist_client < 0)
    {
        dialog_error("stationlist_init: socket (client)");
        return;
    }

    memset((char*)&stationlist_client_addr, 0, sizeof(stationlist_client_addr));
    stationlist_client_addr.sin_family = AF_INET;
    stationlist_client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    stationlist_client_addr.sin_port = htons(conf.stationlist_client);

    g_thread_new("thread_stationlist", stationlist_server, NULL);
}

gpointer stationlist_server(gpointer nothing)
{
    gchar msg[STATIONLIST_BUFF], *ptr, *parse, *param, *value;
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    gint n;
    while((n = recvfrom(stationlist_socket, msg, STATIONLIST_BUFF-1, 0, (struct sockaddr *)&client, &len)) > 0)
    {
        msg[n] = 0;
#if STATIONLIST_DEBUG
        printf("SL -> %s\n", msg);
#endif
        parse = msg;
        while((ptr = strsep(&parse, ";")))
        {
            param = strsep(&ptr, "=");
            value = strsep(&ptr, "=");
            if(param && value)
            {
                stationlist_cmd(param, value);
            }
        }
        len = sizeof(client);
    }

    return NULL;
}

void stationlist_cmd(gchar* param, gchar* value)
{
    if(!strcmp(param, "freq"))
    {
        if(!strcmp(value, "?"))
        {
            stationlist_freq(tuner.freq);
        }
        else
        {
            tune(atoi(value)/1000);
        }
    }
}

void stationlist_freq(gint freq)
{
    gchar* data;
    if(stationlist_client >= 0)
    {
        data = g_strdup_printf("Sender=XDR-GTK;freq=%d", freq*1000);
#if STATIONLIST_DEBUG
        printf("SL <- %s\n", data);
#endif
        sendto(stationlist_client, data, strlen(data), 0, (struct sockaddr *)&stationlist_client_addr, sizeof(stationlist_client_addr));
        g_free(data);
    }
}

void stationlist_stop()
{
    if(stationlist_client >= 0)
    {
        shutdown(stationlist_client, 2);
        closesocket(stationlist_client);
        stationlist_client = -1;
    }

    if(stationlist_socket >= 0)
    {
        shutdown(stationlist_socket, 2);
        closesocket(stationlist_socket);
        stationlist_socket = -1;
    }
}
