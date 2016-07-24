#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include "log.h"
#include "tuner.h"
#include "conf.h"
#include "ui.h"

static FILE *logfp = NULL;
static gchar ps_buff[9];
static gchar rt_buff[2][65];
static gboolean ps_buff_error;
static gchar *default_log_path = "." PATH_SEP "logs";

static gboolean log_prepare();
static void log_timestamp();

static gboolean
log_prepare()
{
    gchar t[16], t2[16], path[100];
    gchar *directory;
    time_t tt;

    if(!conf.rds_logging)
    {
        log_cleanup();
        return FALSE;
    }

    if(logfp)
    {
        return TRUE;
    }

    directory = ((conf.log_dir && strlen(conf.log_dir)) ? conf.log_dir : default_log_path);

    g_sprintf(ps_buff, "%8s", "");
    g_sprintf(rt_buff[0], "%64s", "");
    g_sprintf(rt_buff[1], "%64s", "");
    ps_buff_error = TRUE;

    g_snprintf(path, sizeof(path), "%s" PATH_SEP, directory);
    g_mkdir(path, 0755);

    tt = time(NULL);
    strftime(t, sizeof(t), "%Y-%m-%d", (conf.utc)?gmtime(&tt):localtime(&tt));
    strftime(t2, sizeof(t2), "%H%M%S", (conf.utc)?gmtime(&tt):localtime(&tt));
    g_snprintf(path, sizeof(path), "%s" PATH_SEP "%s" PATH_SEP, directory, t);
    g_mkdir(path, 0755);

    g_snprintf(path, sizeof(path), "%s" PATH_SEP "%s" PATH_SEP "%d-%s.txt", directory, t, tuner.freq, t2);
    logfp = g_fopen(path, "w");

    if(!logfp)
        ui_status(2000, "<b>Failed to create a log. Check logging directory in settings.</b>");

    return (logfp != NULL);
}

static void
log_timestamp()
{
    gchar t[32];
    time_t tt;

    if(logfp)
    {
        tt = time(NULL);
        strftime(t, sizeof(t), "%Y-%m-%d %H:%M:%S", (conf.utc)?gmtime(&tt):localtime(&tt));
        fprintf(logfp, "%s\t", t);
    }
}

void
log_cleanup()
{
    if(logfp)
    {
        fclose(logfp);
        logfp = NULL;
    }
}

void
log_pi(gint pi,
       gint err_level)
{
    if(!log_prepare())
        return;

    log_timestamp();
    fprintf(logfp, "PI\t%04X", pi);
    if(err_level)
        fprintf(logfp, "\t");
    while(err_level--)
        fprintf(logfp, "?");
    fprintf(logfp, "%s", LOG_NL);
}

void
log_af(const gchar *af)
{
    if(!log_prepare())
        return;

    log_timestamp();
    fprintf(logfp, "AF\t%s%s", af, LOG_NL);
}

void
log_ps(const gchar  *ps,
       const guchar *err)
{
    gchar *tmp;
    gboolean error;

    if(!log_prepare())
        return;

    error = err[0] || err[1] || err[2] || err[3] || err[4] || err[5] || err[6] || err[7];

    /* Check whether the PS string is different from a last saved one */
    if(!strcmp(ps, ps_buff) && error == ps_buff_error)
        return;

    strcpy(ps_buff, ps);
    ps_buff_error = error;

    log_timestamp();

    if(conf.replace_spaces)
    {
        tmp = replace_spaces(ps);
        fprintf(logfp, "PS\t%s", tmp);
        g_free(tmp);
    }
    else
    {
        fprintf(logfp, "PS\t%s", ps);
    }

    if(error)
        fprintf(logfp, "\t?%s", LOG_NL);
    else
        fprintf(logfp, "%s", LOG_NL);
}

void
log_rt(guint8       i,
       const gchar *rt)
{
    gchar *tmp;

    if(!log_prepare())
        return;

    /* Check whether the RT string is different from a last saved one */
    if(!strcmp(rt, rt_buff[i]))
        return;

    strcpy(rt_buff[i], rt);

    log_timestamp();

    if(conf.replace_spaces)
    {
        tmp = replace_spaces(rt);
        fprintf(logfp, "RT%d\t%s%s", i+1, tmp, LOG_NL);
        g_free(tmp);
    }
    else
    {
        fprintf(logfp, "RT%d\t%s%s", i+1, rt, LOG_NL);
    }
}

void
log_pty(const gchar *pty)
{
    if(!log_prepare())
        return;

    log_timestamp();
    fprintf(logfp, "PTY\t%s%s", pty, LOG_NL);
}

void
log_ecc(const gchar *ecc,
        guint        ecc_raw)
{
    if(!log_prepare())
        return;

    log_timestamp();
    if(!strcmp(ecc, "??"))
        fprintf(logfp, "ECC\t?? (%02X)%s", ecc_raw, LOG_NL);
    else
        fprintf(logfp, "ECC\t%s%s", ecc, LOG_NL);
}

gchar*
replace_spaces(const gchar *str)
{
    gchar *new_str = g_strdup(str);
    size_t i;
    for(i=0; i<strlen(new_str); i++)
        if(new_str[i] == ' ')
            new_str[i] = '_';
    return new_str;
}
