#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#ifdef G_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "win32.h"
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include "stationlist.h"
#include "conf.h"
#include "ui.h"
#include "ui-tuner-set.h"
#include "tuner-conn.h"
#include "tuner.h"
#include "version.h"

#define STATIONLIST_BUFF        1024
#define STATIONLIST_DEBUG       0
#define STATIONLIST_AF_BUFF_LEN 25

static gint stationlist_socket = -1;
static gint stationlist_client = -1;
static struct sockaddr_in stationlist_client_addr;
static gint stationlist_sender;
static GMutex stationlist_mutex;
static GSList *stationlist_buffer;
static guint8 stationlist_af_buffer[STATIONLIST_AF_BUFF_LEN];
static guint8 stationlist_af_buffer_pos;

static gpointer stationlist_server(gpointer);
static void stationlist_cmd(gchar*, gchar*);
static gboolean stationlist_set_freq(gpointer);
static gboolean stationlist_set_bw(gpointer);

static void stationlist_add(gchar*, gchar*);
static void stationlist_clear_rds();
static gboolean stationlist_send(gchar*);
static void stationlist_free(gpointer);


void
stationlist_init()
{
    stationlist_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(stationlist_socket < 0)
    {
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "SRCP",
                  "stationlist_init: socket");
        return;
    }

#ifndef G_OS_WIN32
    gint on = 1;
    if(setsockopt(stationlist_socket, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) < 0)
    {
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "SRCP",
                  "stationlist_init: SO_REUSEADDR");
    }
#endif

    struct sockaddr_in addr;
    memset((char*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(conf.srcp_port);

    if(bind(stationlist_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "SRCP",
                  "Failed to bind to a port: %d.\nIt may be already in use by another application.",
                  conf.srcp_port);
        closesocket(stationlist_socket);
        stationlist_socket = -1;
        return;
    }

    stationlist_client = socket(AF_INET, SOCK_DGRAM, 0);
    if(stationlist_client < 0)
    {
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "SRCP",
                  "stationlist_init: socket (client)");
        return;
    }

    memset((char*)&stationlist_client_addr, 0, sizeof(stationlist_client_addr));
    stationlist_client_addr.sin_family = AF_INET;
    stationlist_client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    stationlist_client_addr.sin_port = htons(conf.srcp_port-1);

    g_thread_unref(g_thread_new("thread_stationlist", stationlist_server, NULL));
    g_mutex_init(&stationlist_mutex);
    stationlist_af_clear();
    stationlist_sender = g_timeout_add(200, (GSourceFunc)stationlist_send, NULL);
}

gboolean
stationlist_is_up()
{
    return (stationlist_socket >= 0 && stationlist_client >= 0);
}

void
stationlist_stop()
{
    if(stationlist_is_up())
    {
        shutdown(stationlist_client, 2);
        closesocket(stationlist_client);
        stationlist_client = -1;
        shutdown(stationlist_socket, 2);
        closesocket(stationlist_socket);
        stationlist_socket = -1;

        g_source_remove(stationlist_sender);
        g_mutex_lock(&stationlist_mutex);
        g_slist_free_full(stationlist_buffer, (GDestroyNotify)stationlist_free);
        stationlist_buffer = NULL;
        g_mutex_unlock(&stationlist_mutex);
        g_mutex_clear(&stationlist_mutex);
    }
}

static gpointer
stationlist_server(gpointer nothing)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    gchar msg[STATIONLIST_BUFF], *ptr, *parse, *param, *value;
    gint n;

    while((n = recvfrom(stationlist_socket, msg, STATIONLIST_BUFF-1, 0, (struct sockaddr*)&addr, &len)) > 0)
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
        stationlist_client_addr.sin_addr.s_addr = addr.sin_addr.s_addr;
        len = sizeof(addr);
    }
    return NULL;
}

static void
stationlist_cmd(gchar *param,
                gchar *value)
{
    if(!g_ascii_strcasecmp(param, "freq"))
    {
        if(!g_ascii_strcasecmp(value, "?"))
            stationlist_freq(tuner_get_freq());
        else
            g_idle_add(stationlist_set_freq, GINT_TO_POINTER(atoi(value)));
    }
    else if(!g_ascii_strcasecmp(param, "bandwidth"))
    {
        if(!g_ascii_strcasecmp(value, "?"))
            stationlist_bw(tuner_filter_bw(tuner.filter));
        else
            g_idle_add(stationlist_set_bw, GINT_TO_POINTER(atoi(value)));
    }
}

static gboolean
stationlist_set_freq(gpointer freq)
{
    tuner_set_frequency(GPOINTER_TO_INT(freq)/1000);
    return FALSE;
}

static gboolean
stationlist_set_bw(gpointer bw)
{
    gint index = tuner_filter_index_from_bw(GPOINTER_TO_INT(bw));
    if(index >= 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(ui.c_bw), index);
    return FALSE;
}

void
stationlist_freq(gint freq)
{
    if(stationlist_is_up())
    {
        stationlist_clear_rds();
        stationlist_add(g_strdup("freq"), g_strdup_printf("%d", freq*1000));
    }
}

void
stationlist_rcvlevel(gint level)
{
    if(stationlist_is_up())
    {
        stationlist_add(g_strdup("RcvLevel"), g_strdup_printf("%d", level));
    }
}

void
stationlist_pi(gint pi)
{
    if(stationlist_is_up())
    {
        stationlist_add(g_strdup("pi"), g_strdup_printf("%04X", pi));
    }
}

void
stationlist_pty(gint pty)
{
    if(stationlist_is_up())
    {
        stationlist_add(g_strdup("pty"), g_strdup_printf("%01X", pty));
    }
}

void
stationlist_ecc(guchar ecc)
{
    if(stationlist_is_up())
    {
        stationlist_add(g_strdup("ecc"), g_strdup_printf("%02X", ecc));
    }
}

void
stationlist_ps(gchar *ps)
{
    if(stationlist_is_up())
    {
        stationlist_add(g_strdup("ps"), g_strdup_printf("%2X%2X%2X%2X%2X%2X%2X%2X", ps[0], ps[1], ps[2], ps[3], ps[4], ps[5], ps[6], ps[7]));
    }
}

void
stationlist_rt(gint   n,
               gchar *rt)
{
    GString *msg;
    gchar *msg_full;

    if(stationlist_is_up())
    {
        msg = g_string_new("");
        while(*rt)
        {
            g_string_append_printf(msg, "%02X", *rt++);
        }
        msg_full = g_string_free(msg, FALSE);
        stationlist_add(g_strdup_printf("rt%d", n+1), msg_full);
    }
}

void
stationlist_bw(gint bw)
{
    if(stationlist_is_up())
    {
        stationlist_add(g_strdup("bandwidth"), g_strdup_printf("%d", bw));
    }
}

void
stationlist_af(gint af)
{
    if(stationlist_is_up())
    {
        stationlist_af_buffer[stationlist_af_buffer_pos] = af;
        stationlist_af_buffer_pos = (stationlist_af_buffer_pos + 1) % STATIONLIST_AF_BUFF_LEN;

        stationlist_add(g_strdup("af"),
                        g_strdup_printf("%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                                        stationlist_af_buffer[0],
                                        stationlist_af_buffer[1],
                                        stationlist_af_buffer[2],
                                        stationlist_af_buffer[3],
                                        stationlist_af_buffer[4],
                                        stationlist_af_buffer[5],
                                        stationlist_af_buffer[6],
                                        stationlist_af_buffer[7],
                                        stationlist_af_buffer[8],
                                        stationlist_af_buffer[9],
                                        stationlist_af_buffer[10],
                                        stationlist_af_buffer[11],
                                        stationlist_af_buffer[12],
                                        stationlist_af_buffer[13],
                                        stationlist_af_buffer[14],
                                        stationlist_af_buffer[15],
                                        stationlist_af_buffer[16],
                                        stationlist_af_buffer[17],
                                        stationlist_af_buffer[18],
                                        stationlist_af_buffer[19],
                                        stationlist_af_buffer[20],
                                        stationlist_af_buffer[21],
                                        stationlist_af_buffer[22],
                                        stationlist_af_buffer[23],
                                        stationlist_af_buffer[24])
                       );
    }
}

void
stationlist_af_clear()
{
    gint i;
    for(i=0; i<STATIONLIST_AF_BUFF_LEN; i++)
    {
        stationlist_af_buffer[i] = 0;
    }
    stationlist_af_buffer_pos = 0;
}

static void
stationlist_add(gchar *param,
                gchar *value)
{
    GSList *l;
    sl_data_t *d;

    g_mutex_lock(&stationlist_mutex);

    // update existing data
    for(l=stationlist_buffer; l; l=g_slist_next(l))
    {
        d = l->data;
        if(!strcmp(param, d->param))
        {
            g_free(param);
            g_free(d->value);
            d->value = value;
            g_mutex_unlock(&stationlist_mutex);
            return;
        }
    }

    // or create new element
    d = g_new(sl_data_t, 1);
    d->param = param;
    d->value = value;
    stationlist_buffer = g_slist_append(stationlist_buffer, d);

    g_mutex_unlock(&stationlist_mutex);
}

static void
stationlist_clear_rds()
{
    GSList *l;
    sl_data_t *d;

    g_mutex_lock(&stationlist_mutex);

    l = stationlist_buffer;
    while(l)
    {
        d = l->data;
        l = g_slist_next(l);
        if(!strcmp("pi", d->param) || !strcmp("pty", d->param) || !strcmp("ecc", d->param) || !strcmp("ps", d->param) || !strcmp("rt", d->param) || !strcmp("af", d->param))
        {
            stationlist_buffer = g_slist_remove(stationlist_buffer, d);
            stationlist_free(d);
        }
    }

    stationlist_af_clear();
    g_mutex_unlock(&stationlist_mutex);
}

static gboolean
stationlist_send(gchar *data)
{
    GString *msg;
    GSList *l;
    gchar *msg_full;

    g_mutex_lock(&stationlist_mutex);
    if(!stationlist_buffer)
    {
        g_mutex_unlock(&stationlist_mutex);
        return TRUE;
    }

    msg = g_string_new("");
    g_string_append_printf(msg, "from=%s", APP_NAME);
    for(l=stationlist_buffer; l; l=g_slist_next(l))
    {
        g_string_append_printf(msg, ";%s=%s", ((sl_data_t*)l->data)->param, ((sl_data_t*)l->data)->value);
    }
    g_slist_free_full(stationlist_buffer, (GDestroyNotify)stationlist_free);
    stationlist_buffer = NULL;
    g_mutex_unlock(&stationlist_mutex);

    msg_full = g_string_free(msg, FALSE);
#if STATIONLIST_DEBUG
    printf("SL <- %s\n", msg_full);
#endif
    sendto(stationlist_client, msg_full, strlen(msg_full), 0, (struct sockaddr *)&stationlist_client_addr, sizeof(stationlist_client_addr));
    g_free(msg_full);
    return TRUE;
}

static void
stationlist_free(gpointer data)
{
    sl_data_t *d = data;
    g_free(d->param);
    g_free(d->value);
    g_free(data);
}
