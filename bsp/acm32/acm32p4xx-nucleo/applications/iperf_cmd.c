/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board MSH wrapper for lwIP official lwiperf (raw TCP API).
 * Same approach as ACM32P4 HAL SDK LwIP_iperf demo.
 *
 *   iperf -s              start TCP server (port 5001)
 *   iperf -c <ip>         start TCP client (default 10s)
 *   iperf --stop          abort session
 *
 * Host (iperf 2.x, not iperf3):
 *   iperf -c 192.168.16.50 -p 5001 -t 10
 */

#include <rtthread.h>
#include <string.h>
#include <stdlib.h>

#ifdef BSP_USING_IPERF

#include <lwip/apps/lwiperf.h>
#include <lwip/tcpip.h>
#include <lwip/ip_addr.h>

#define IPERF_PORT_DEFAULT  LWIPERF_TCP_PORT_DEFAULT

static void *g_session = RT_NULL;
static volatile rt_bool_t g_busy = RT_FALSE;
static char g_host[32];
static u16_t g_port = IPERF_PORT_DEFAULT;
static struct rt_semaphore g_sync_sem;
static volatile rt_bool_t g_sync_ready = RT_FALSE;

static void iperf_sync_init(void)
{
    if (!g_sync_ready)
    {
        rt_sem_init(&g_sync_sem, "iperf", 0, RT_IPC_FLAG_PRIO);
        g_sync_ready = RT_TRUE;
    }
}

static const char *lwiperf_type_str(enum lwiperf_report_type t)
{
    switch (t)
    {
    case LWIPERF_TCP_DONE_SERVER:           return "DONE_SERVER";
    case LWIPERF_TCP_DONE_CLIENT:           return "DONE_CLIENT";
    case LWIPERF_TCP_ABORTED_LOCAL:         return "ABORTED_LOCAL";
    case LWIPERF_TCP_ABORTED_LOCAL_DATAERROR: return "ABORTED_DATAERROR";
    case LWIPERF_TCP_ABORTED_LOCAL_TXERROR: return "ABORTED_TXERROR";
    case LWIPERF_TCP_ABORTED_REMOTE:        return "ABORTED_REMOTE";
    default:                                return "UNKNOWN";
    }
}

static void lwiperf_report(void *arg, enum lwiperf_report_type report_type,
                           const ip_addr_t *local_addr, u16_t local_port,
                           const ip_addr_t *remote_addr, u16_t remote_port,
                           u32_t bytes_transferred, u32_t ms_duration,
                           u32_t bandwidth_kbitpsec)
{
    char local_ip[IP4ADDR_STRLEN_MAX];
    char remote_ip[IP4ADDR_STRLEN_MAX];

    (void)arg;
    ip4addr_ntoa_r(ip_2_ip4(local_addr), local_ip, sizeof(local_ip));
    ip4addr_ntoa_r(ip_2_ip4(remote_addr), remote_ip, sizeof(remote_ip));

    rt_kprintf("[lwiperf] %s %s:%u <-> %s:%u\n",
               lwiperf_type_str(report_type),
               local_ip, (unsigned)local_port,
               remote_ip, (unsigned)remote_port);
    rt_kprintf("[lwiperf] %u bytes in %u ms, %u kbit/s\n",
               (unsigned)bytes_transferred,
               (unsigned)ms_duration,
               (unsigned)bandwidth_kbitpsec);

    if (report_type != LWIPERF_TCP_DONE_SERVER)
    {
        /* Server keeps listening; client/one-shot sessions end here */
        if (report_type == LWIPERF_TCP_DONE_CLIENT ||
            report_type == LWIPERF_TCP_ABORTED_LOCAL ||
            report_type == LWIPERF_TCP_ABORTED_LOCAL_DATAERROR ||
            report_type == LWIPERF_TCP_ABORTED_LOCAL_TXERROR ||
            report_type == LWIPERF_TCP_ABORTED_REMOTE)
        {
            g_session = RT_NULL;
            g_busy = RT_FALSE;
        }
    }
}

struct iperf_start_arg
{
    int is_server;
    ip_addr_t addr;
    u16_t port;
    void *session;
    err_t err;
};

struct iperf_abort_arg
{
    void *session;
};

static void iperf_start_tcpip(void *arg)
{
    struct iperf_start_arg *a = (struct iperf_start_arg *)arg;

    a->err = ERR_OK;
    if (a->is_server)
    {
        a->session = lwiperf_start_tcp_server(&a->addr, a->port,
                                              lwiperf_report, RT_NULL);
    }
    else
    {
        a->session = lwiperf_start_tcp_client(&a->addr, a->port,
                                              LWIPERF_CLIENT,
                                              lwiperf_report, RT_NULL);
    }
    if (a->session == RT_NULL)
        a->err = ERR_MEM;
    rt_sem_release(&g_sync_sem);
}

static void iperf_abort_tcpip(void *arg)
{
    struct iperf_abort_arg *a = (struct iperf_abort_arg *)arg;

    if (a->session != RT_NULL)
        lwiperf_abort(a->session);
    rt_sem_release(&g_sync_sem);
}

static void iperf_usage(void)
{
    rt_kprintf("Usage (lwIP lwiperf, iperf2-compatible):\n");
    rt_kprintf("  iperf -s [port]         TCP server (default %d)\n", IPERF_PORT_DEFAULT);
    rt_kprintf("  iperf -c <ip> [port]    TCP client\n");
    rt_kprintf("  iperf --stop            abort session\n");
    rt_kprintf("Host: iperf -c 192.168.16.50 -p 5001 -t 10\n");
}

static int iperf(int argc, char **argv)
{
    struct iperf_start_arg sa;
    ip4_addr_t ip4;

    rt_kprintf("iperf cmd (lwiperf)\n");
    iperf_sync_init();

    if (argc < 2)
    {
        iperf_usage();
        return 0;
    }

    if (!strcmp(argv[1], "--stop") || !strcmp(argv[1], "stop"))
    {
        struct iperf_abort_arg aa;

        if (g_session == RT_NULL)
        {
            rt_kprintf("iperf: no session\n");
            g_busy = RT_FALSE;
            return 0;
        }
        aa.session = g_session;
        while (rt_sem_trytake(&g_sync_sem) == RT_EOK)
            ;
        if (tcpip_callback(iperf_abort_tcpip, &aa) != ERR_OK)
        {
            rt_kprintf("iperf: tcpip_callback failed\n");
            return -1;
        }
        rt_sem_take(&g_sync_sem, RT_WAITING_FOREVER);
        g_session = RT_NULL;
        g_busy = RT_FALSE;
        rt_kprintf("iperf stopped\n");
        return 0;
    }

    if (g_busy || g_session != RT_NULL)
    {
        rt_kprintf("iperf busy, stop first: iperf --stop\n");
        return -1;
    }

    g_port = IPERF_PORT_DEFAULT;
    rt_memset(&sa, 0, sizeof(sa));

    if (!strcmp(argv[1], "-s") || !strcmp(argv[1], "s"))
    {
        if (argc >= 3)
            g_port = (u16_t)atoi(argv[2]);
        sa.is_server = 1;
        IP_ADDR4(&sa.addr, 0, 0, 0, 0); /* any */
        sa.port = g_port;
        g_busy = RT_TRUE;
        while (rt_sem_trytake(&g_sync_sem) == RT_EOK)
            ;
        if (tcpip_callback(iperf_start_tcpip, &sa) != ERR_OK)
        {
            g_busy = RT_FALSE;
            rt_kprintf("iperf: tcpip_callback failed\n");
            return -1;
        }
        rt_sem_take(&g_sync_sem, RT_WAITING_FOREVER);
        if (sa.session == RT_NULL)
        {
            g_busy = RT_FALSE;
            rt_kprintf("iperf: start server failed\n");
            return -1;
        }
        g_session = sa.session;
        rt_kprintf("iperf server listening on %u (lwiperf)\n", (unsigned)g_port);
        return 0;
    }

    if (!strcmp(argv[1], "-c") || !strcmp(argv[1], "c"))
    {
        if (argc < 3)
        {
            iperf_usage();
            return 0;
        }
        rt_strncpy(g_host, argv[2], sizeof(g_host) - 1);
        g_host[sizeof(g_host) - 1] = '\0';
        if (argc >= 4)
            g_port = (u16_t)atoi(argv[3]);
        if (!ip4addr_aton(g_host, &ip4))
        {
            rt_kprintf("iperf: bad ip %s\n", g_host);
            return -1;
        }
        sa.is_server = 0;
        ip_addr_copy_from_ip4(sa.addr, ip4);
        sa.port = g_port;
        g_busy = RT_TRUE;
        while (rt_sem_trytake(&g_sync_sem) == RT_EOK)
            ;
        if (tcpip_callback(iperf_start_tcpip, &sa) != ERR_OK)
        {
            g_busy = RT_FALSE;
            rt_kprintf("iperf: tcpip_callback failed\n");
            return -1;
        }
        rt_sem_take(&g_sync_sem, RT_WAITING_FOREVER);
        if (sa.session == RT_NULL)
        {
            g_busy = RT_FALSE;
            rt_kprintf("iperf: start client failed\n");
            return -1;
        }
        g_session = sa.session;
        rt_kprintf("iperf client -> %s:%u (lwiperf)\n", g_host, (unsigned)g_port);
        return 0;
    }

    iperf_usage();
    return 0;
}
MSH_CMD_EXPORT(iperf, lwIP lwiperf TCP server/client);

#endif /* BSP_USING_IPERF */
