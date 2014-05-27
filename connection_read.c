#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "gui.h"
#include "connection.h"
#include "connection_read.h"
#include "graph.h"
#include "settings.h"
#include "scan.h"
#include "pattern.h"
#include "rdsspy.h"
#ifdef G_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/ioctl.h>
#endif

gpointer read_thread(gpointer nothing)
{
    gchar c, buffer[SERIAL_BUFFER];
    gint n, pos = 0;
    fd_set input;
    struct timeval timeout;

#ifdef G_OS_WIN32
    DWORD len_in = 0;
    BOOL fWaitingOnRead = FALSE;
    DWORD state;
    OVERLAPPED osReader = {0};
    if (tuner.socket == INVALID_SOCKET)
    {
        osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (osReader.hEvent == NULL)
        {
            thread = FALSE;
        }
    }
#endif
    if(thread)
    {
        g_usleep(1750000); // arduino may restart during port opening
        xdr_write("x");
        g_usleep(50000);
    }

    while(thread)
    {
#ifdef G_OS_WIN32
        if (tuner.socket != INVALID_SOCKET)
        {
            FD_ZERO(&input);
            FD_SET(tuner.socket, &input);
            timeout.tv_sec  = 0;
            timeout.tv_usec = 50000;
            if(! (n = select(tuner.socket+1, &input, NULL, NULL, &timeout)))
            {
                continue;
            }
            if(n < 0 || recv(tuner.socket, &c, 1, 0) <= 0)
            {
                break;
            }
        }
        else
        {
            if (!fWaitingOnRead)
            {
                if (!ReadFile(tuner.serial, &c, 1, &len_in, &osReader))
                {
                    if (GetLastError() != ERROR_IO_PENDING)
                    {
                        CloseHandle(osReader.hEvent);
                        break;
                    }
                    else
                    {
                        fWaitingOnRead = TRUE;
                    }
                }
            }

            if (fWaitingOnRead)
            {
                state = WaitForSingleObject(osReader.hEvent, 200);
                if(state == WAIT_TIMEOUT)
                {
                    continue;
                }
                if(state != WAIT_OBJECT_0)
                {
                    CloseHandle(osReader.hEvent);
                    break;
                }

                if (!GetOverlappedResult(tuner.serial, &osReader, &len_in, FALSE))
                {
                    CloseHandle(osReader.hEvent);
                    break;
                }

                fWaitingOnRead = FALSE;
            }
            if(len_in != 1)
            {
                continue;
            }
        }
#else
        FD_ZERO(&input);
        FD_SET(tuner.serial, &input);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 50000;
        if(!(n = select(tuner.serial+1, &input, NULL, NULL, &timeout)))
        {
            continue;
        }
        if(n < 0 || read(tuner.serial, &c, 1) <= 0)
        {
            break;
        }
#endif
        if(c != '\n' && pos != SERIAL_BUFFER-1)
        {
            buffer[pos++] = c;
            continue;
        }
        buffer[pos] = 0;
        pos = 0;

#if DEBUG_READ
        g_print("read: %s\n", buffer);
#endif

        if(buffer[0] == 'O') // OK
        {
            ready = TRUE;
        }
        else if(buffer[0] == 'X')
        {
            break;
        }
        else if(buffer[0] == 'a') // socket auth
        {
            gint auth = atoi(buffer+1);
            g_idle_add(gui_auth, GINT_TO_POINTER(auth));
            if(!auth)
            {
                break;
            }
        }
        else
        {
            read_parse(buffer[0], buffer+1);
        }
    }

// restart Arduino using RTS & DTR lines
#ifdef G_OS_WIN32
    if(tuner.socket != INVALID_SOCKET)
    {
        closesocket(tuner.socket);
    }
    else
    {
        EscapeCommFunction(tuner.serial, CLRDTR);
        EscapeCommFunction(tuner.serial, CLRRTS);
        g_usleep(10000);
        EscapeCommFunction(tuner.serial, SETDTR);
        EscapeCommFunction(tuner.serial, SETRTS);
        CloseHandle(tuner.serial);
    }
#else
    // is this really a serial port?
    if(ioctl(tuner.serial, TIOCMGET, &n) != -1)
    {
        n &= ~(TIOCM_DTR | TIOCM_RTS);
        ioctl(tuner.serial, TIOCMSET, &n);
        g_usleep(10000);
        n |=  (TIOCM_DTR | TIOCM_RTS);
        ioctl(tuner.serial, TIOCMSET, &n);
    }

    close(tuner.serial);
#endif
    tuner.online = 0;
    thread = FALSE;
    g_idle_add(gui_clear_power_off, NULL);
    return NULL;
}

void read_parse(gchar c, gchar msg[])
{
    if(c == 'S') // signal level + stereo/mono
    {
        guint smeter = atoi(msg+1);
        gfloat sig = (smeter >> 16) + (smeter&0xFFFF)/65536.0;

        if(tuner.mode == MODE_FM)
        {
            // calibrated signal level meter (simple linear interpolation)
            sig = sig*0.797 + 3.5;
        }

        if(pattern.active)
        {
            pattern_push(sig);
            g_idle_add(pattern_update, NULL);
        }

        rssi_pos = (rssi_pos + 1)%rssi_len;
        rssi[rssi_pos].value = sig;
        rssi[rssi_pos].rds = (tuner.rds_timer?TRUE:FALSE);
        rssi[rssi_pos].stereo = (msg[0]=='s'?TRUE:FALSE);

        tuner.rds_timer = (tuner.rds_timer ? (tuner.rds_timer-1) : 0);

        if(sig > tuner.max_signal)
        {
            tuner.max_signal = sig;
        }

        g_idle_add(gui_update_status, NULL);
    }
    else if(c == 'T') // tuned frequency
    {
        gint newfreq = atoi(msg);
        if(newfreq != tuner.freq)
        {
            tuner.prevfreq = tuner.freq;
        }
        tuner.freq = newfreq;
        rdsspy_reset();

        tuner.max_signal = 0;
        g_idle_add(gui_clear, GINT_TO_POINTER(newfreq));
    }
    else if(c == 'V') // alignment data
    {
        tuner.daa = atoi(msg);
    }
    else if(c == 'P') // PI code
    {
        guint pi;
        sscanf(msg, "%x", &pi);
        tuner.pi = pi;

        tuner.rds_timer = conf.rds_timeout;
        if(conf.rds_reset)
        {
            tuner.rds_reset_timer = g_get_real_time();
        }

        if(strlen((msg)) == 4) // double checked
        {
            if(tuner.prevpi != tuner.pi)
                g_idle_add(gui_update_pi, GINT_TO_POINTER(TRUE));
            tuner.prevpi = tuner.pi;
        }
        else
        {
            g_idle_add(gui_update_pi, GINT_TO_POINTER(FALSE));
        }
    }
    else if(c == 'R') // RDS data
    {
        guint data[3], errors, i;

        for(i=0; i<3; i++)
        {
            gchar hex[5];
            strncpy(hex, msg+i*4, 4);
            hex[4] = 0;
            sscanf(hex, "%x", &data[i]);
        }
        sscanf(msg+12, "%x", &errors);

        if(tuner.pi >= 0)
        {
            rdsspy_send(tuner.pi, msg, errors);
        }

        if((!conf.rds_discard && tuner.pi>=0) || rssi[rssi_pos].rds)
        {
            guchar group = (data[BLOCK_B] & 0xF000) >> 12;
            gboolean flag = (data[BLOCK_B] & 0x0800) >> 11;
            guchar err[] = { (errors&3), ((errors&12)>>2), ((errors&48)>>4) };

            // PTY, TP, TA, MS, AF: error-free blocks
            if(!err[BLOCK_B])
            {
                gchar pty = (data[BLOCK_B] & 0x03E0) >> 5;
                gchar tp = (data[BLOCK_B] & 0x400) >> 10;
                if(tuner.prevpty != pty || tuner.prevtp != tp)
                {
                    tuner.prevtp = tp;
                    tuner.prevpty = pty;
                    g_idle_add(gui_update_ptytp, NULL);
                }

                if(group == 0)
                {
                    gchar ta = (data[BLOCK_B] & 0x10) >> 4;
                    gchar ms = (data[BLOCK_B] & 0x8) >> 3;
                    if(tuner.prevta != ta || tuner.prevms != ms)
                    {
                        tuner.prevta = ta;
                        tuner.prevms = ms;
                        g_idle_add(gui_update_tams, NULL);
                    }

                    // AF
                    if(!err[BLOCK_C] && !flag)
                    {
                        guchar af[] = { data[BLOCK_C] >> 8, data[BLOCK_C] & 0xFF };
                        for(i=0; i<2; i++)
                        {
                            if(af[i]>0 && af[i]<205)
                            {
                                gchar* af_new_freq = g_strdup_printf("%.1f", ((87500+af[i]*100)/1000.0));
                                g_idle_add(gui_update_af, af_new_freq);
                            }
                        }
                    }
                }
            }

            // PS: user-defined error correction
            if(conf.rds_ps_progressive || err[BLOCK_B] <= conf.rds_info_error)
            {
                // PS
                if(err[BLOCK_B] < 3 && err[BLOCK_D] < 3 && group == 0 && (conf.rds_ps_progressive || (err[BLOCK_D] <= conf.rds_data_error)))
                {
                    gchar pos = data[BLOCK_B] & 3;
                    gchar ps[] = { data[BLOCK_D] >> 8, data[BLOCK_D] & 0xFF };
                    gboolean changed = FALSE;
                    guchar e = 2*err[BLOCK_B] + 3*err[BLOCK_D];

                    for(i=0; i<2; i++)
                    {
                        // only ASCII printable characters
                        if(ps[i] >= 32 && ps[i] < 127)
                        {
                            gint p = pos*2+i;
                            if(!conf.rds_ps_progressive || tuner.ps_err[p] >= e)
                            {
                                if(tuner.ps[p] != ps[i] || tuner.ps_err[p] > e)
                                {
                                    tuner.ps[p] = ps[i];
                                    tuner.ps_err[p] = e;
                                    changed = TRUE;
                                }
                            }
                        }
                    }

                    if(changed)
                    {
                        tuner.ps_avail = TRUE;
                        g_idle_add(gui_update_ps, NULL);
                    }
                }
            }

            // RT: user-defined error correction
            if(err[BLOCK_B] <= conf.rds_info_error)
            {
                if(group == 2)
                {
                    short pos  = (data[BLOCK_B] & 15);
                    gboolean flag = (data[BLOCK_B] & 16) >> 4;
                    gchar rt[] = { data[BLOCK_C]>>8, data[BLOCK_C]&0xFF, data[BLOCK_D]>>8, data[BLOCK_D]&0xFF };
                    gboolean changed = FALSE;

                    for(i=0; i<4; i++)
                    {
                        if(rt[i] == 0x0D) // end of the RadioText message
                        {
                            if ((i <= 1 && (errors&15) == 0) || (i >= 2 && (errors&51) == 0))
                            {
                                tuner.rt[flag][pos*4+i] = 0;
                                changed = TRUE;
                            }
                        }
                        // only ASCII printable characters
                        else if(rt[i] >= 32 && rt[i] < 127 && tuner.rt[flag][pos*4+i] != rt[i])
                        {
                            if( (i <= 1 && ((errors&12)>>2) <= conf.rds_data_error) || (i >= 2 && ((errors&48)>>4) <= conf.rds_data_error) )
                            {
                                tuner.rt[flag][pos*4+i] = rt[i];
                                changed = TRUE;
                            }
                        }
                    }

                    if(changed)
                    {
                        g_idle_add(gui_update_rt, GINT_TO_POINTER(flag));
                    }
                }
            }
        }
    }
    else if(c == 'U') // Spectral Scan
    {
        gchar tmp[10];
        gint i, j = 0, k = 0, n = 0;
        for(i=0; i<strlen(msg); i++)
            if(msg[i] == ',')
                n++;
        scan_data* data = g_new(scan_data, 1);
        data->len = n;
        data->min = 100;
        data->max = 0;
        data->signals = g_new(scan_node, data->len);
        for(i=0; i<strlen(msg); i++)
        {
            if(msg[i] == '=')
            {
                tmp[j] = 0;
                data->signals[k].freq = atoi(tmp);
                j = 0;
            }
            else if(msg[i] == ',')
            {
                tmp[j] = 0;
                data->signals[k].signal = atoi(tmp);
                j = 0;
                if(data->signals[k].signal > data->max)
                    data->max = data->signals[k].signal;
                if(data->signals[k].signal < data->min)
                    data->min = data->signals[k].signal;
                k++;
            }
            else
                tmp[j++] = msg[i];
        }
        g_idle_add(scan_update, (gpointer)data);
    }
    else if(c == 'Y')  // Sound volume control (network)
    {
        g_idle_add(gui_update_volume, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'A') // RF AGC threshold (network)
    {
        g_idle_add(gui_update_agc, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'D') // De-emphasis (network)
    {
        g_idle_add(gui_update_deemphasis, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'Z') // Antenna switch (network)
    {
        g_idle_add(gui_update_ant, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'G') // RF/IF Gain (network)
    {
        g_idle_add(gui_update_gain, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'M') // FM/AM mode (network)
    {
        g_idle_add(((msg[0] == '0') ? gui_mode_FM : gui_mode_AM), NULL);
    }
    else if(c == 'F') // Filter (network)
    {
        g_idle_add(gui_update_filter, GINT_TO_POINTER(atoi(msg)));
    }
    else if(c == 'o') // On-line users (network)
    {
        tuner.online = atoi(msg);
    }
}

gfloat signal_level(gfloat value)
{
    if(tuner.mode == MODE_FM)
    {
        switch(conf.signal_unit)
        {
        case UNIT_DBM:
            return value - 120;

        case UNIT_DBUV:
            return value - 11.25;

        default:
        case UNIT_DBF:
            return value;
        }
    }
    return value;
}
