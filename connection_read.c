#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "gui.h"
#include "connection.h"
#include "graph.h"
#include "settings.h"
#include "scan.h"
#include "pattern.h"
#ifdef G_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

gpointer read_thread(gpointer nothing)
{
    gchar c, buffer[SERIAL_BUFFER];
    gint pos = 0;
#ifdef G_OS_WIN32
    DWORD len_in = 0;
#else
    gint n;
    fd_set input;
    struct timeval timeout;
#endif

    thread = TRUE;
    g_usleep(1750000); // arduino may restart during port opening
    xdr_write("x");
    g_usleep(50000);
    while(thread)
    {
#ifdef G_OS_WIN32
        if (serial_socket != INVALID_SOCKET)
        {
            if (recv(serial_socket, &c, 1, 0) <= 0)
            {
                break;
            }
        }
        else
        {
            DWORD state;
            BOOL fWaitingOnRead = FALSE;
            OVERLAPPED osReader = {0};
            osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (osReader.hEvent == NULL)
            {
                break;
            }

            if (!fWaitingOnRead)
            {
                if (!ReadFile(serial, &c, 1, &len_in, &osReader))
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
                if(state == WAIT_FAILED)
                {
                    CloseHandle(osReader.hEvent);
                    break;
                }
                else if(state != WAIT_OBJECT_0)
                {
                    CloseHandle(osReader.hEvent);
                    continue;
                }

                if (!GetOverlappedResult(serial, &osReader, &len_in, FALSE))
                {
                    CloseHandle(osReader.hEvent);
                    break;
                }
            }
            CloseHandle(osReader.hEvent);

            if(len_in != 1)
            {
                continue;
            }
        }
#else
        FD_ZERO(&input);
        FD_SET(serial, &input);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 50000;
        n = select(serial+1, &input, NULL, NULL, &timeout);

        if(!n)
            continue;

        if(n < 0 || read(serial, &c, 1) <= 0)
            break;
#endif

        if(c != '\n' && pos != SERIAL_BUFFER-1)
        {
            buffer[pos++] = c;
            continue;
        }
        buffer[pos] = 0x00;
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
        else
        {
            read_parse(buffer[0], buffer+1);
        }
    }

#ifdef G_OS_WIN32
    if(serial_socket != INVALID_SOCKET)
    {
        closesocket(serial_socket);
        WSACleanup();
    }
    else
    {
        CloseHandle(serial);
    }
#else
    close(serial);
#endif
    online = -1;
    thread = FALSE;
    g_idle_add_full(G_PRIORITY_DEFAULT, gui_clear_power_off, NULL, NULL);
    return NULL;
}

void read_parse(gchar c, gchar msg[])
{
    if(c == 'S') // signal level + stereo/mono
    {
        guint smeter = atoi(msg+1);

        float sig = (smeter >> 16) + (smeter&0xFFFF)/65536.0;
        if(conf.mode == MODE_FM)
        {
            sig = sig*0.797 + 3.5;
        }

        if(sig > max_signal)
        {
            max_signal = sig;
        }

        if(pattern.active)
        {
            pattern_push(sig);
            g_idle_add_full(G_PRIORITY_DEFAULT, pattern_update, NULL, NULL);
        }

        rssi_pos = (rssi_pos + 1)%rssi_len;
        rssi[rssi_pos].value = sig;

        if((g_get_monotonic_time() - rds_timer) <= (conf.rds_timeout*1000))
        {
            rssi[rssi_pos].rds = TRUE;
        }
        else
        {
            rssi[rssi_pos].rds = FALSE;
        }

        rssi[rssi_pos].stereo = (msg[0]=='s'?TRUE:FALSE);

        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_status, NULL, NULL);
    }
    else if(c == 'T') // tuned frequency
    {
        gint newfreq = atoi(msg);

        if(newfreq != freq)
        {
            prevfreq = freq;
        }

        freq = newfreq;

        rds_timer = max_signal = 0;
        pi = prevpi = prevpty = prevtp = prevta = prevms = -1;
        ps_available = FALSE;

        gchar *freq_text = g_new(gchar, 10);
        g_sprintf(freq_text, "%7.3f", newfreq/1000.0);
        g_idle_add_full(G_PRIORITY_DEFAULT, gui_clear, freq_text, NULL);
    }
    else if(c == 'V') // alignment data
    {
        daa = atoi(msg);
    }
    else if(c == 'P') // PI code
    {
        guint _pi;
        sscanf(msg, "%x", &_pi);
        pi = _pi;
        rds_timer = g_get_monotonic_time();
        gint isOK = 0;
        if(strlen((msg)) == 4) // double checked
        {
            isOK = TRUE;
            if(prevpi != pi)
                g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_pi, GINT_TO_POINTER(isOK), NULL);
            prevpi = pi;
        }
        else
        {
            isOK = FALSE;
            g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_pi, GINT_TO_POINTER(isOK), NULL);
        }
    }
    else if(c == 'R') // RDS data
    {
        if((!conf.rds_discard && pi>=0) || rssi[rssi_pos].rds)
        {
            enum { BLOCK_B, BLOCK_C, BLOCK_D };
            guint i, data[3], errors;
            gchar hexbuffer[5];
            for (i=0; i<3; i++)
            {
                strncpy(hexbuffer, msg+i*4, 4);
                hexbuffer[4]=0x00;
                sscanf(hexbuffer, "%x", &data[i]);
            }
            sscanf(msg+12, "%x", &errors);

            short group = (data[BLOCK_B] & 0xF000) >> 12;
            gboolean flag = (data[BLOCK_B] & 0x0800) >> 11;

            // PTY, TP, TA, MS: error-free blocks
            if((errors&3) == 0)
            {
                short pty = (data[BLOCK_B] & 0x03E0) >> 5;
                short tp = (data[BLOCK_B] & 0x400) >> 10;
                if(prevpty != pty || prevtp != tp)
                {
                    prevtp = tp;
                    prevpty = pty;
                    g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_ptytp, NULL, NULL);
                }

                if(group == 0)
                {
                    short ta = (data[BLOCK_B] & 0x10) >> 4;
                    short ms = (data[BLOCK_B] & 0x8) >> 3;
                    if(prevta != ta || prevms != ms)
                    {
                        prevta = ta;
                        prevms = ms;
                        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_tams, NULL, NULL);
                    }
                }
            }

            // AF: error-free blocks
            if((errors&15) == 0 && group == 0 && !flag)
            {
                guchar af[] = { data[BLOCK_C] >> 8, data[BLOCK_C] & 0xFF };
                for(i=0; i<2; i++)
                {
                    if(af[i]>0 && af[i]<205)
                    {
                        gchar* af_new_freq = g_new(gchar, 6);
                        g_snprintf(af_new_freq, 6, "%.1f", ((87500+af[i]*100)/1000.0));
                        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_af, af_new_freq, NULL);
                    }
                }
            }

            // PS, RT: user-defined error correction
            if((errors&3) <= conf.rds_info_error)
            {
                // PS
                if(group == 0 && ((errors&48)>>4) <= conf.rds_data_error)
                {
                    short pos = data[BLOCK_B] & 3;
                    gchar ps[] = { data[BLOCK_D] >> 8, data[BLOCK_D] & 0xFF };
                    gboolean changed = FALSE;

                    for(i=0; i<2; i++)
                    {
                        // only ASCII printable characters
                        if(ps[i] >= 32 && ps[i] < 127 && ps_data[pos*2+i] != ps[i])
                        {
                            ps_data[pos*2+i] = ps[i];
                            changed = TRUE;
                        }
                    }
                    if(changed)
                    {
                        ps_available = TRUE;
                        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_ps, NULL, NULL);
                    }
                }

                // RT
                if(group == 2)
                {
                    short pos  = (data[BLOCK_B] & 15);
                    gboolean flag = (data[BLOCK_B] & 16) >> 4;
                    gchar rt[] = { data[BLOCK_C]>>8, data[BLOCK_C]&0xFF, data[BLOCK_D]>>8, data[BLOCK_D]&0xFF };
                    gboolean changed = FALSE;

                    for(i=0; i<4; i++)
                    {
                        // only ASCII printable characters
                        if(rt[i] >= 32 && rt[i] < 127 && rt_data[flag][pos*4+i] != rt[i])
                        {
                            if( (i <= 1 && ((errors&12)>>2) <= conf.rds_data_error) ||  (i >= 2 && ((errors&48)>>4) <= conf.rds_data_error) )
                            {
                                rt_data[flag][pos*4+i] = rt[i];
                                changed = TRUE;
                            }
                        }
                    }

                    if(changed)
                    {
                        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_rt, GINT_TO_POINTER(flag), NULL);
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
        g_idle_add_full(G_PRIORITY_DEFAULT, scan_update, (gpointer)data, NULL);
    }
    else if(c == 'Y')  // Sound volume control  (network)
    {
        gint *val = g_new(gint, 1);
        *val = atoi(msg);

        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_volume, (gpointer)val, NULL);
    }
    else if(c == 'A') // RF AGC threshold  (network)
    {
        gint *id = g_new(gint, 1);
        *id = atoi(msg);

        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_agc, (gpointer)id, NULL);
    }
    else if(c == 'D') // De-emphasis  (network)
    {
        gint *id = g_new(gint, 1);
        *id = atoi(msg);

        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_deemphasis, (gpointer)id, NULL);
    }
    else if(c == 'Z') // Antenna swtich (network)
    {
        gint *id = g_new(gint, 1);
        *id = atoi(msg);

        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_ant, (gpointer)id, NULL);
    }
    else if(c == 'G') // RF/IF Gain  (network)
    {
        gchar *data = g_new(gchar, 2);
        data[0] = msg[0];
        data[1] = msg[1];

        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_gain, (gpointer)data, NULL);
    }
    else if(c == 'M') // FM/AM mode (network)
    {
        if(msg[0] == '0')
        {
            g_idle_add_full(G_PRIORITY_DEFAULT, gui_mode_FM, NULL, NULL);
        }
        else
        {
           g_idle_add_full(G_PRIORITY_DEFAULT, gui_mode_AM, NULL, NULL);
        }
    }
    else if(c == 'F') // Filter (network)
    {
        gint *id = g_new(gint, 1);
        *id = atoi(msg);

        g_idle_add_full(G_PRIORITY_DEFAULT, gui_update_filter, (gpointer)id, NULL);
    }
    else if(c == 'o') // On-line users (network)
    {
        online = atoi(msg);
    }
}

gfloat signal_level(gfloat value)
{
    if(conf.mode == MODE_FM)
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
