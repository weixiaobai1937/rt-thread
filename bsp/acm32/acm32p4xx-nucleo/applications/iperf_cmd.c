/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal iperf2-compatible TCP tool (BSP applications only).
 * Does not modify third-party/lwIP sources.
 *
 * board server:  iperf -s
 * host client:   iperf -c 192.168.16.50 -p 5001 -t 10
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include <string.h>

#ifdef BSP_USING_IPERF

#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <lwip/netdb.h>

#ifndef IPERF_PORT_DEFAULT
#define IPERF_PORT_DEFAULT  5001
#endif

#ifndef IPERF_BUF_SIZE
#define IPERF_BUF_SIZE      1460
#endif

#ifndef IPERF_CLIENT_SEC
#define IPERF_CLIENT_SEC    10
#endif

static volatile rt_bool_t g_running = RT_FALSE;
static volatile rt_bool_t g_stop = RT_FALSE;
static rt_thread_t g_tid = RT_NULL;
static int g_mode = 0; /* 0=server, 1=client */
static int g_port = IPERF_PORT_DEFAULT;
static char g_host[32];

static void iperf_print_rate(rt_uint64_t bytes, rt_tick_t ticks)
{
    rt_uint32_t ms;
    rt_uint32_t kbps;

    if (ticks == 0)
        ticks = 1;
    ms = (rt_uint32_t)(ticks * 1000U / RT_TICK_PER_SECOND);
    if (ms == 0)
        ms = 1;
    kbps = (rt_uint32_t)((bytes * 8ULL * 1000ULL) / (rt_uint64_t)ms / 1000ULL);
    rt_kprintf("[iperf] %u bytes in %u ms, %u kbit/s\n",
               (unsigned)bytes, (unsigned)ms, (unsigned)kbps);
}

static void iperf_server_entry(void *param)
{
    int listen_fd = -1, conn_fd = -1;
    struct sockaddr_in addr, caddr;
    socklen_t clen;
    char *buf;
    rt_uint64_t total;
    rt_tick_t t0, t1;
    int n;
    int on = 1;

    (void)param;
    buf = (char *)rt_malloc(IPERF_BUF_SIZE);
    if (buf == RT_NULL)
    {
        rt_kprintf("iperf: no mem\n");
        goto out;
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        rt_kprintf("iperf: socket failed\n");
        goto out;
    }

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    rt_memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u16_t)g_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        rt_kprintf("iperf: bind %d failed\n", g_port);
        goto out;
    }
    if (listen(listen_fd, 1) < 0)
    {
        rt_kprintf("iperf: listen failed\n");
        goto out;
    }

    rt_kprintf("iperf TCP server listen on %d\n", g_port);

    while (!g_stop)
    {
        struct timeval tv;
        fd_set rfds;

        FD_ZERO(&rfds);
        FD_SET(listen_fd, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        n = select(listen_fd + 1, &rfds, RT_NULL, RT_NULL, &tv);
        if (n <= 0)
            continue;

        clen = sizeof(caddr);
        conn_fd = accept(listen_fd, (struct sockaddr *)&caddr, &clen);
        if (conn_fd < 0)
            continue;

        rt_kprintf("iperf: client %s:%u connected\n",
                   inet_ntoa(caddr.sin_addr),
                   (unsigned)ntohs(caddr.sin_port));

        total = 0;
        t0 = rt_tick_get();
        while (!g_stop)
        {
            n = recv(conn_fd, buf, IPERF_BUF_SIZE, 0);
            if (n <= 0)
                break;
            total += (rt_uint64_t)n;
        }
        t1 = rt_tick_get();
        closesocket(conn_fd);
        conn_fd = -1;
        iperf_print_rate(total, t1 - t0);
    }

out:
    if (conn_fd >= 0)
        closesocket(conn_fd);
    if (listen_fd >= 0)
        closesocket(listen_fd);
    if (buf)
        rt_free(buf);
    g_running = RT_FALSE;
    g_tid = RT_NULL;
    rt_kprintf("iperf server stopped\n");
}

static void iperf_client_entry(void *param)
{
    int fd = -1;
    struct sockaddr_in addr;
    char *buf;
    rt_uint64_t total = 0;
    rt_tick_t t0, deadline, now;
    int n, i;
    int sec = IPERF_CLIENT_SEC;

    (void)param;
    buf = (char *)rt_malloc(IPERF_BUF_SIZE);
    if (buf == RT_NULL)
    {
        rt_kprintf("iperf: no mem\n");
        goto out;
    }
    for (i = 0; i < IPERF_BUF_SIZE; i++)
        buf[i] = (char)('0' + (i % 10));

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        rt_kprintf("iperf: socket failed\n");
        goto out;
    }

    rt_memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u16_t)g_port);
    if (inet_aton(g_host, &addr.sin_addr) == 0)
    {
        rt_kprintf("iperf: bad ip %s\n", g_host);
        goto out;
    }

    rt_kprintf("iperf client connect %s:%d ...\n", g_host, g_port);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        rt_kprintf("iperf: connect failed\n");
        goto out;
    }

    t0 = rt_tick_get();
    deadline = t0 + sec * RT_TICK_PER_SECOND;
    while (!g_stop)
    {
        now = rt_tick_get();
        if ((rt_tick_t)(now - deadline) < (rt_tick_t)(1U << 31) && now >= deadline)
            break;
        n = send(fd, buf, IPERF_BUF_SIZE, 0);
        if (n <= 0)
            break;
        total += (rt_uint64_t)n;
    }
    iperf_print_rate(total, rt_tick_get() - t0);

out:
    if (fd >= 0)
        closesocket(fd);
    if (buf)
        rt_free(buf);
    g_running = RT_FALSE;
    g_tid = RT_NULL;
    rt_kprintf("iperf client done\n");
}

static void iperf_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  iperf -s [port]         TCP server (default 5001)\n");
    rt_kprintf("  iperf -c <ip> [port]    TCP client, send %ds\n", IPERF_CLIENT_SEC);
    rt_kprintf("  iperf --stop            stop running session\n");
}

static int iperf(int argc, char **argv)
{
    if (argc < 2)
    {
        iperf_usage();
        return -1;
    }

    if (!strcmp(argv[1], "--stop") || !strcmp(argv[1], "stop"))
    {
        if (!g_running)
        {
            rt_kprintf("no active iperf\n");
            return 0;
        }
        g_stop = RT_TRUE;
        rt_kprintf("stopping iperf...\n");
        return 0;
    }

    if (g_running)
    {
        rt_kprintf("iperf busy, use: iperf --stop\n");
        return -1;
    }

    g_port = IPERF_PORT_DEFAULT;
    g_stop = RT_FALSE;

    if (!strcmp(argv[1], "-s") || !strcmp(argv[1], "s"))
    {
        if (argc >= 3)
            g_port = atoi(argv[2]);
        g_mode = 0;
        g_running = RT_TRUE;
        g_tid = rt_thread_create("iperf", iperf_server_entry, RT_NULL,
                                 2048, 20, 10);
        if (g_tid == RT_NULL)
        {
            g_running = RT_FALSE;
            rt_kprintf("iperf: create thread failed\n");
            return -1;
        }
        rt_thread_startup(g_tid);
        return 0;
    }

    if (!strcmp(argv[1], "-c") || !strcmp(argv[1], "c"))
    {
        if (argc < 3)
        {
            iperf_usage();
            return -1;
        }
        rt_strncpy(g_host, argv[2], sizeof(g_host) - 1);
        g_host[sizeof(g_host) - 1] = '\0';
        if (argc >= 4)
            g_port = atoi(argv[3]);
        g_mode = 1;
        g_running = RT_TRUE;
        g_tid = rt_thread_create("iperf", iperf_client_entry, RT_NULL,
                                 2048, 20, 10);
        if (g_tid == RT_NULL)
        {
            g_running = RT_FALSE;
            rt_kprintf("iperf: create thread failed\n");
            return -1;
        }
        rt_thread_startup(g_tid);
        return 0;
    }

    iperf_usage();
    return -1;
}
MSH_CMD_EXPORT(iperf, TCP iperf2 server/client);

#endif /* BSP_USING_IPERF */
