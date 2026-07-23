/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ethernet smoke / functional tests for acm32p4xx-nucleo (lwIP sockets).
 *
 * MSH:
 *   eth_test                 auto suite: link + IP + ping gateway
 *   eth_test info            show netif / MAC / flags
 *   eth_test ping [ip] [n]   ICMP ping (default: gateway, 4 times)
 *   eth_test tcp_echo [port] start TCP echo server (default 7)
 *   eth_test tcp_echo stop   stop TCP echo server
 *   eth_test udp_echo [port] one-shot UDP echo (default 7, wait 30s)
 *   eth_test iperf -s [port] TCP iperf server (lwiperf, need BSP_USING_IPERF)
 *   eth_test iperf -c <ip> [port]
 *   eth_test iperf stop
 *
 * Host side examples:
 *   ping 192.168.16.50
 *   nc 192.168.16.50 7
 *   iperf -c 192.168.16.50 -p 5001 -t 10   (iperf 2.x, not iperf3)
 *
 * Also on board: eth_ifconfig / dping / arp
 */

#include <rtthread.h>
#include <string.h>
#include <stdlib.h>

#ifdef BSP_USING_ETH

#include <lwip/netif.h>
#include <lwip/ip4_addr.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <lwip/icmp.h>
#include <lwip/ip.h>
#include <lwip/inet_chksum.h>
#include <lwip/netifapi.h>

#ifdef BSP_USING_IPERF
#include <lwip/apps/lwiperf.h>
#include <lwip/tcpip.h>
#include <lwip/ip_addr.h>
#endif

#define ETH_TEST_TCP_PORT_DEFAULT   7
#define ETH_TEST_UDP_PORT_DEFAULT   7
#define ETH_TEST_PING_DEFAULT_N     4
#define ETH_TEST_PING_PAYLOAD       32
#define ETH_TEST_PING_TIMEOUT_MS    2000
#define ETH_TEST_TCP_STACK          2048
#define ETH_TEST_TCP_PRIO           20
#define ETH_TEST_TCP_BACKLOG        2
#define ETH_TEST_BUF_SZ             512

static volatile rt_bool_t g_tcp_echo_run = RT_FALSE;
static rt_thread_t g_tcp_echo_tid = RT_NULL;
static int g_tcp_echo_listen_fd = -1;
static int g_tcp_echo_port = ETH_TEST_TCP_PORT_DEFAULT;

static struct netif *eth_test_netif(void)
{
    struct netif *netif;

    for (netif = netif_list; netif != RT_NULL; netif = netif->next)
    {
        if (netif->name[0] == 'e' && netif->name[1] == '0')
            return netif;
    }
    return netif_default ? netif_default : netif_list;
}

static void eth_test_print_info(struct netif *netif)
{
    if (netif == RT_NULL)
    {
        rt_kprintf("[eth_test] no netif\n");
        return;
    }

    rt_kprintf("netif: %c%c%s\n",
               netif->name[0], netif->name[1],
               (netif == netif_default) ? " (Default)" : "");
    rt_kprintf("  IP : %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
    rt_kprintf("  GW : %s\n", ip4addr_ntoa(netif_ip4_gw(netif)));
    rt_kprintf("  NM : %s\n", ip4addr_ntoa(netif_ip4_netmask(netif)));
    rt_kprintf("  MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
               netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);
    rt_kprintf("  FLAGS: %s%s%s\n",
               (netif->flags & NETIF_FLAG_UP) ? "UP " : "DOWN ",
               (netif->flags & NETIF_FLAG_LINK_UP) ? "LINK_UP " : "LINK_DOWN ",
               (netif->flags & NETIF_FLAG_ETHARP) ? "ETHARP" : "");
}

/* ---- ICMP ping (raw socket, same idea as dping) ---- */

static int eth_test_ping_once(const ip4_addr_t *dst, u16_t seq, int *rtt_ms)
{
    int s, ret, matched = 0;
    int ping_size = (int)(sizeof(struct icmp_echo_hdr) + ETH_TEST_PING_PAYLOAD);
    char sbuf[sizeof(struct icmp_echo_hdr) + ETH_TEST_PING_PAYLOAD];
    char rbuf[64];
    struct sockaddr_in to, from;
    socklen_t fromlen = sizeof(from);
    struct timeval tv;
    struct icmp_echo_hdr *eh;
    rt_tick_t t0, t1, t_start, t_max;
    u16_t id = 0xE7E7;

    *rtt_ms = -1;

    s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
    if (s < 0)
        return -1;

    tv.tv_sec = ETH_TEST_PING_TIMEOUT_MS / 1000;
    tv.tv_usec = (ETH_TEST_PING_TIMEOUT_MS % 1000) * 1000;
    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    eh = (struct icmp_echo_hdr *)sbuf;
    ICMPH_TYPE_SET(eh, ICMP_ECHO);
    ICMPH_CODE_SET(eh, 0);
    eh->id = htons(id);
    eh->seqno = htons(seq);
    memset(sbuf + sizeof(struct icmp_echo_hdr), 0x5A, ETH_TEST_PING_PAYLOAD);
    eh->chksum = 0;
    eh->chksum = inet_chksum(eh, ping_size);

    memset(&to, 0, sizeof(to));
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    inet_addr_from_ip4addr(&to.sin_addr, dst);

    t0 = rt_tick_get();
    ret = lwip_sendto(s, sbuf, ping_size, 0, (struct sockaddr *)&to, sizeof(to));
    if (ret != ping_size)
    {
        lwip_close(s);
        return -2;
    }

    t_start = t0;
    t_max = rt_tick_from_millisecond(ETH_TEST_PING_TIMEOUT_MS);
    while ((rt_tick_get() - t_start) < t_max)
    {
        fromlen = sizeof(from);
        ret = lwip_recvfrom(s, rbuf, sizeof(rbuf), 0,
                            (struct sockaddr *)&from, &fromlen);
        t1 = rt_tick_get();
        if (ret <= 0)
            break;

        if (ret >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr)))
        {
            struct ip_hdr *ip = (struct ip_hdr *)rbuf;
            int hl = IPH_HL(ip) * 4;
            struct icmp_echo_hdr *reh = (struct icmp_echo_hdr *)(rbuf + hl);

            if (ICMPH_TYPE(reh) == ICMP_ER &&
                reh->id == htons(id) &&
                reh->seqno == htons(seq))
            {
                *rtt_ms = (int)((t1 - t0) * 1000 / RT_TICK_PER_SECOND);
                matched = 1;
                break;
            }
        }
    }

    lwip_close(s);
    return matched ? 0 : -3;
}

static int eth_test_do_ping(const char *ip_str, int times)
{
    ip4_addr_t dst;
    int i, ok = 0, fail = 0, rtt;
    int rtt_min = 0x7fffffff, rtt_max = 0, rtt_sum = 0;

    if (times <= 0)
        times = ETH_TEST_PING_DEFAULT_N;
    if (times > 20)
        times = 20;

    if (!ip4addr_aton(ip_str, &dst))
    {
        rt_kprintf("[eth_test] bad ip: %s\n", ip_str);
        return -1;
    }

    rt_kprintf("[eth_test] ping %s x%d\n", ip_str, times);
    for (i = 1; i <= times; i++)
    {
        int rc = eth_test_ping_once(&dst, (u16_t)i, &rtt);
        if (rc == 0)
        {
            ok++;
            if (rtt < rtt_min)
                rtt_min = rtt;
            if (rtt > rtt_max)
                rtt_max = rtt;
            rtt_sum += rtt;
            rt_kprintf("  seq=%d rtt=%d ms\n", i, rtt);
        }
        else
        {
            fail++;
            rt_kprintf("  seq=%d %s\n", i,
                       (rc == -1) ? "socket fail" :
                       (rc == -2) ? "send fail" : "timeout");
        }
        rt_thread_mdelay(200);
    }

    rt_kprintf("[eth_test] ping done: ok=%d fail=%d", ok, fail);
    if (ok > 0)
        rt_kprintf(" rtt min/avg/max=%d/%d/%d ms",
                   rtt_min, rtt_sum / ok, rtt_max);
    rt_kprintf("\n");

    return (ok > 0 && fail == 0) ? 0 : -1;
}

/* ---- TCP echo server ---- */

static void eth_test_tcp_echo_thread(void *param)
{
    int port = (int)(rt_ubase_t)param;
    int listen_fd = -1, client_fd = -1;
    struct sockaddr_in addr, cli;
    socklen_t clen;
    char buf[ETH_TEST_BUF_SZ];
    int yes = 1;

    listen_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        rt_kprintf("[eth_test] tcp_echo: socket failed\n");
        goto out;
    }

    lwip_setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u16_t)port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (lwip_bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        rt_kprintf("[eth_test] tcp_echo: bind %d failed\n", port);
        goto out;
    }
    if (lwip_listen(listen_fd, ETH_TEST_TCP_BACKLOG) < 0)
    {
        rt_kprintf("[eth_test] tcp_echo: listen failed\n");
        goto out;
    }

    /* Non-blocking accept loop via short RCVTIMEO on accept isn't portable;
     * use select with timeout so stop can exit cleanly. */
    g_tcp_echo_listen_fd = listen_fd;
    g_tcp_echo_port = port;
    rt_kprintf("[eth_test] tcp_echo listening on %d\n", port);

    while (g_tcp_echo_run)
    {
        fd_set rfds;
        struct timeval tv;
        int n;

        FD_ZERO(&rfds);
        FD_SET(listen_fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 200 * 1000;

        n = lwip_select(listen_fd + 1, &rfds, RT_NULL, RT_NULL, &tv);
        if (n <= 0)
            continue;
        if (!FD_ISSET(listen_fd, &rfds))
            continue;

        clen = sizeof(cli);
        client_fd = lwip_accept(listen_fd, (struct sockaddr *)&cli, &clen);
        if (client_fd < 0)
            continue;

        rt_kprintf("[eth_test] tcp_echo accept %s:%u\n",
                   inet_ntoa(cli.sin_addr), (unsigned)ntohs(cli.sin_port));

        {
            struct timeval ctv = { 5, 0 };
            lwip_setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &ctv, sizeof(ctv));
        }

        while (g_tcp_echo_run)
        {
            int r = lwip_recv(client_fd, buf, sizeof(buf), 0);
            if (r < 0)
                break;
            if (r == 0)
                break;
            if (lwip_send(client_fd, buf, r, 0) != r)
                break;
        }

        lwip_close(client_fd);
        client_fd = -1;
        rt_kprintf("[eth_test] tcp_echo client closed\n");
    }

out:
    if (client_fd >= 0)
        lwip_close(client_fd);
    if (listen_fd >= 0)
        lwip_close(listen_fd);
    g_tcp_echo_listen_fd = -1;
    g_tcp_echo_run = RT_FALSE;
    g_tcp_echo_tid = RT_NULL;
    rt_kprintf("[eth_test] tcp_echo stopped\n");
}

static int eth_test_tcp_echo_start(int port)
{
    if (port <= 0 || port > 65535)
        port = ETH_TEST_TCP_PORT_DEFAULT;

    if (g_tcp_echo_run || g_tcp_echo_tid != RT_NULL)
    {
        rt_kprintf("[eth_test] tcp_echo already running on %d (stop first)\n",
                   g_tcp_echo_port);
        return -1;
    }

    g_tcp_echo_run = RT_TRUE;
    g_tcp_echo_tid = rt_thread_create("eth_techo",
                                      eth_test_tcp_echo_thread,
                                      (void *)(rt_ubase_t)port,
                                      ETH_TEST_TCP_STACK,
                                      ETH_TEST_TCP_PRIO, 10);
    if (g_tcp_echo_tid == RT_NULL)
    {
        g_tcp_echo_run = RT_FALSE;
        rt_kprintf("[eth_test] tcp_echo thread create failed\n");
        return -1;
    }
    rt_thread_startup(g_tcp_echo_tid);
    return 0;
}

static int eth_test_tcp_echo_stop(void)
{
    if (!g_tcp_echo_run && g_tcp_echo_tid == RT_NULL)
    {
        rt_kprintf("[eth_test] tcp_echo not running\n");
        return 0;
    }
    g_tcp_echo_run = RT_FALSE;
    /* thread exits after select timeout / client close */
    rt_kprintf("[eth_test] tcp_echo stop requested\n");
    return 0;
}

/* ---- UDP echo (blocking wait, one session window) ---- */

static int eth_test_udp_echo(int port, int wait_s)
{
    int s, ret;
    struct sockaddr_in addr, from;
    socklen_t fromlen;
    char buf[ETH_TEST_BUF_SZ];
    struct timeval tv;
    rt_tick_t start, limit;
    int pkts = 0, bytes = 0;

    if (port <= 0 || port > 65535)
        port = ETH_TEST_UDP_PORT_DEFAULT;
    if (wait_s <= 0)
        wait_s = 30;

    s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        rt_kprintf("[eth_test] udp_echo: socket failed\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u16_t)port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (lwip_bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        rt_kprintf("[eth_test] udp_echo: bind %d failed\n", port);
        lwip_close(s);
        return -1;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    rt_kprintf("[eth_test] udp_echo on %d, wait %ds (send UDP from host)\n",
               port, wait_s);

    start = rt_tick_get();
    limit = rt_tick_from_millisecond(wait_s * 1000);
    while ((rt_tick_get() - start) < limit)
    {
        fromlen = sizeof(from);
        ret = lwip_recvfrom(s, buf, sizeof(buf), 0,
                            (struct sockaddr *)&from, &fromlen);
        if (ret > 0)
        {
            lwip_sendto(s, buf, ret, 0, (struct sockaddr *)&from, fromlen);
            pkts++;
            bytes += ret;
            rt_kprintf("  echo %d B from %s:%u\n",
                       ret, inet_ntoa(from.sin_addr),
                       (unsigned)ntohs(from.sin_port));
        }
    }

    lwip_close(s);
    rt_kprintf("[eth_test] udp_echo done: pkts=%d bytes=%d %s\n",
               pkts, bytes, pkts > 0 ? "PASS" : "FAIL/idle");
    return pkts > 0 ? 0 : -1;
}

/* ---- iperf (lwIP lwiperf raw API; same as former iperf_cmd) ---- */

#ifdef BSP_USING_IPERF

#define ETH_TEST_IPERF_PORT_DEFAULT  LWIPERF_TCP_PORT_DEFAULT

static void *g_iperf_session = RT_NULL;
static volatile rt_bool_t g_iperf_busy = RT_FALSE;
static char g_iperf_host[32];
static u16_t g_iperf_port = ETH_TEST_IPERF_PORT_DEFAULT;
static struct rt_semaphore g_iperf_sem;
static volatile rt_bool_t g_iperf_sem_ready = RT_FALSE;

static void eth_test_iperf_sync_init(void)
{
    if (!g_iperf_sem_ready)
    {
        rt_sem_init(&g_iperf_sem, "eiperf", 0, RT_IPC_FLAG_PRIO);
        g_iperf_sem_ready = RT_TRUE;
    }
}

static const char *eth_test_iperf_type_str(enum lwiperf_report_type t)
{
    switch (t)
    {
    case LWIPERF_TCP_DONE_SERVER:             return "DONE_SERVER";
    case LWIPERF_TCP_DONE_CLIENT:             return "DONE_CLIENT";
    case LWIPERF_TCP_ABORTED_LOCAL:           return "ABORTED_LOCAL";
    case LWIPERF_TCP_ABORTED_LOCAL_DATAERROR: return "ABORTED_DATAERROR";
    case LWIPERF_TCP_ABORTED_LOCAL_TXERROR:   return "ABORTED_TXERROR";
    case LWIPERF_TCP_ABORTED_REMOTE:          return "ABORTED_REMOTE";
    default:                                  return "UNKNOWN";
    }
}

static void eth_test_iperf_report(void *arg, enum lwiperf_report_type report_type,
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

    rt_kprintf("[eth_test] iperf %s %s:%u <-> %s:%u\n",
               eth_test_iperf_type_str(report_type),
               local_ip, (unsigned)local_port,
               remote_ip, (unsigned)remote_port);
    rt_kprintf("[eth_test] iperf %u bytes in %u ms, %u kbit/s\n",
               (unsigned)bytes_transferred,
               (unsigned)ms_duration,
               (unsigned)bandwidth_kbitpsec);

    if (report_type == LWIPERF_TCP_DONE_CLIENT ||
        report_type == LWIPERF_TCP_ABORTED_LOCAL ||
        report_type == LWIPERF_TCP_ABORTED_LOCAL_DATAERROR ||
        report_type == LWIPERF_TCP_ABORTED_LOCAL_TXERROR ||
        report_type == LWIPERF_TCP_ABORTED_REMOTE)
    {
        /* Server keeps listening; client / abort ends session */
        g_iperf_session = RT_NULL;
        g_iperf_busy = RT_FALSE;
    }
}

struct eth_test_iperf_start_arg
{
    int is_server;
    ip_addr_t addr;
    u16_t port;
    void *session;
    err_t err;
};

struct eth_test_iperf_abort_arg
{
    void *session;
};

static void eth_test_iperf_start_tcpip(void *arg)
{
    struct eth_test_iperf_start_arg *a = (struct eth_test_iperf_start_arg *)arg;

    a->err = ERR_OK;
    if (a->is_server)
    {
        a->session = lwiperf_start_tcp_server(&a->addr, a->port,
                                              eth_test_iperf_report, RT_NULL);
    }
    else
    {
        a->session = lwiperf_start_tcp_client(&a->addr, a->port,
                                              LWIPERF_CLIENT,
                                              eth_test_iperf_report, RT_NULL);
    }
    if (a->session == RT_NULL)
        a->err = ERR_MEM;
    rt_sem_release(&g_iperf_sem);
}

static void eth_test_iperf_abort_tcpip(void *arg)
{
    struct eth_test_iperf_abort_arg *a = (struct eth_test_iperf_abort_arg *)arg;

    if (a->session != RT_NULL)
        lwiperf_abort(a->session);
    rt_sem_release(&g_iperf_sem);
}

static void eth_test_iperf_usage(void)
{
    rt_kprintf("Usage (lwIP lwiperf, iperf2-compatible):\n");
    rt_kprintf("  eth_test iperf -s [port]       TCP server (default %d)\n",
               ETH_TEST_IPERF_PORT_DEFAULT);
    rt_kprintf("  eth_test iperf -c <ip> [port]  TCP client\n");
    rt_kprintf("  eth_test iperf stop            abort session\n");
    rt_kprintf("Host: iperf -c 192.168.16.50 -p 5001 -t 10\n");
}

/* argv[0] is first token after "iperf" (e.g. -s / -c / stop) */
static int eth_test_do_iperf(int argc, char **argv)
{
    struct eth_test_iperf_start_arg sa;
    ip4_addr_t ip4;

    eth_test_iperf_sync_init();

    if (argc < 1)
    {
        eth_test_iperf_usage();
        return 0;
    }

    if (!strcmp(argv[0], "stop") || !strcmp(argv[0], "--stop"))
    {
        struct eth_test_iperf_abort_arg aa;

        if (g_iperf_session == RT_NULL)
        {
            rt_kprintf("[eth_test] iperf: no session\n");
            g_iperf_busy = RT_FALSE;
            return 0;
        }
        aa.session = g_iperf_session;
        while (rt_sem_trytake(&g_iperf_sem) == RT_EOK)
            ;
        if (tcpip_callback(eth_test_iperf_abort_tcpip, &aa) != ERR_OK)
        {
            rt_kprintf("[eth_test] iperf: tcpip_callback failed\n");
            return -1;
        }
        rt_sem_take(&g_iperf_sem, RT_WAITING_FOREVER);
        g_iperf_session = RT_NULL;
        g_iperf_busy = RT_FALSE;
        rt_kprintf("[eth_test] iperf stopped\n");
        return 0;
    }

    if (g_iperf_busy || g_iperf_session != RT_NULL)
    {
        rt_kprintf("[eth_test] iperf busy, stop first: eth_test iperf stop\n");
        return -1;
    }

    g_iperf_port = ETH_TEST_IPERF_PORT_DEFAULT;
    rt_memset(&sa, 0, sizeof(sa));

    if (!strcmp(argv[0], "-s") || !strcmp(argv[0], "s"))
    {
        if (argc >= 2)
            g_iperf_port = (u16_t)atoi(argv[1]);
        sa.is_server = 1;
        IP_ADDR4(&sa.addr, 0, 0, 0, 0);
        sa.port = g_iperf_port;
        g_iperf_busy = RT_TRUE;
        while (rt_sem_trytake(&g_iperf_sem) == RT_EOK)
            ;
        if (tcpip_callback(eth_test_iperf_start_tcpip, &sa) != ERR_OK)
        {
            g_iperf_busy = RT_FALSE;
            rt_kprintf("[eth_test] iperf: tcpip_callback failed\n");
            return -1;
        }
        rt_sem_take(&g_iperf_sem, RT_WAITING_FOREVER);
        if (sa.session == RT_NULL)
        {
            g_iperf_busy = RT_FALSE;
            rt_kprintf("[eth_test] iperf: start server failed\n");
            return -1;
        }
        g_iperf_session = sa.session;
        rt_kprintf("[eth_test] iperf server listening on %u\n",
                   (unsigned)g_iperf_port);
        return 0;
    }

    if (!strcmp(argv[0], "-c") || !strcmp(argv[0], "c"))
    {
        if (argc < 2)
        {
            eth_test_iperf_usage();
            return 0;
        }
        rt_strncpy(g_iperf_host, argv[1], sizeof(g_iperf_host) - 1);
        g_iperf_host[sizeof(g_iperf_host) - 1] = '\0';
        if (argc >= 3)
            g_iperf_port = (u16_t)atoi(argv[2]);
        if (!ip4addr_aton(g_iperf_host, &ip4))
        {
            rt_kprintf("[eth_test] iperf: bad ip %s\n", g_iperf_host);
            return -1;
        }
        sa.is_server = 0;
        ip_addr_copy_from_ip4(sa.addr, ip4);
        sa.port = g_iperf_port;
        g_iperf_busy = RT_TRUE;
        while (rt_sem_trytake(&g_iperf_sem) == RT_EOK)
            ;
        if (tcpip_callback(eth_test_iperf_start_tcpip, &sa) != ERR_OK)
        {
            g_iperf_busy = RT_FALSE;
            rt_kprintf("[eth_test] iperf: tcpip_callback failed\n");
            return -1;
        }
        rt_sem_take(&g_iperf_sem, RT_WAITING_FOREVER);
        if (sa.session == RT_NULL)
        {
            g_iperf_busy = RT_FALSE;
            rt_kprintf("[eth_test] iperf: start client failed\n");
            return -1;
        }
        g_iperf_session = sa.session;
        rt_kprintf("[eth_test] iperf client -> %s:%u\n",
                   g_iperf_host, (unsigned)g_iperf_port);
        return 0;
    }

    eth_test_iperf_usage();
    return 0;
}

#endif /* BSP_USING_IPERF */

/* ---- auto suite ---- */

static int eth_test_auto(void)
{
    struct netif *netif;
    int pass = 1;
    char gw[IP4ADDR_STRLEN_MAX];

    rt_kprintf("======== eth_test auto ========\n");

    netif = eth_test_netif();
    if (netif == RT_NULL)
    {
        rt_kprintf("[1] netif          FAIL (no interface)\n");
        return -1;
    }
    rt_kprintf("[1] netif          PASS (%c%c)\n", netif->name[0], netif->name[1]);

    eth_test_print_info(netif);

    if (!(netif->flags & NETIF_FLAG_UP))
    {
        rt_kprintf("[2] admin UP       FAIL\n");
        pass = 0;
    }
    else
        rt_kprintf("[2] admin UP       PASS\n");

    if (!(netif->flags & NETIF_FLAG_LINK_UP))
    {
        rt_kprintf("[3] link UP        FAIL (cable / PHY?)\n");
        pass = 0;
    }
    else
        rt_kprintf("[3] link UP        PASS\n");

    if (ip4_addr_isany_val(*netif_ip4_addr(netif)))
    {
        rt_kprintf("[4] IP assigned    FAIL\n");
        pass = 0;
    }
    else
        rt_kprintf("[4] IP assigned    PASS (%s)\n",
                   ip4addr_ntoa(netif_ip4_addr(netif)));

    if (ip4_addr_isany_val(*netif_ip4_gw(netif)))
    {
        rt_kprintf("[5] gateway ping   SKIP (no gateway)\n");
    }
    else if (!(netif->flags & NETIF_FLAG_LINK_UP))
    {
        rt_kprintf("[5] gateway ping   SKIP (link down)\n");
        pass = 0;
    }
    else
    {
        ip4addr_ntoa_r(netif_ip4_gw(netif), gw, sizeof(gw));
        rt_kprintf("[5] gateway ping   -> %s\n", gw);
        if (eth_test_do_ping(gw, ETH_TEST_PING_DEFAULT_N) != 0)
        {
            rt_kprintf("[5] gateway ping   FAIL\n");
            pass = 0;
        }
        else
            rt_kprintf("[5] gateway ping   PASS\n");
    }

    rt_kprintf("======== eth_test %s ========\n", pass ? "PASS" : "FAIL");
    rt_kprintf("tips: eth_ifconfig | dping <ip> | eth_test iperf -s | eth_test tcp_echo\n");
    return pass ? 0 : -1;
}

static void eth_test_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  eth_test                 auto: link + IP + ping gw\n");
    rt_kprintf("  eth_test info            show netif\n");
    rt_kprintf("  eth_test ping [ip] [n]   ICMP ping (default gw, 4)\n");
    rt_kprintf("  eth_test tcp_echo [port] TCP echo server (default 7)\n");
    rt_kprintf("  eth_test tcp_echo stop   stop TCP echo\n");
    rt_kprintf("  eth_test udp_echo [port] [sec]  UDP echo window\n");
#ifdef BSP_USING_IPERF
    rt_kprintf("  eth_test iperf -s [port] TCP iperf server (lwiperf)\n");
    rt_kprintf("  eth_test iperf -c <ip> [port]\n");
    rt_kprintf("  eth_test iperf stop\n");
#endif
    rt_kprintf("Also: eth_ifconfig, dping, arp\n");
}

static int eth_test(int argc, char **argv)
{
    struct netif *netif;

    if (argc < 2)
        return eth_test_auto();

    if (!strcmp(argv[1], "help") || !strcmp(argv[1], "-h"))
    {
        eth_test_usage();
        return 0;
    }

    if (!strcmp(argv[1], "info"))
    {
        eth_test_print_info(eth_test_netif());
        return 0;
    }

    if (!strcmp(argv[1], "ping"))
    {
        const char *ip;
        int n = ETH_TEST_PING_DEFAULT_N;
        char gw[IP4ADDR_STRLEN_MAX];

        if (argc >= 3)
            ip = argv[2];
        else
        {
            netif = eth_test_netif();
            if (netif == RT_NULL || ip4_addr_isany_val(*netif_ip4_gw(netif)))
            {
                rt_kprintf("[eth_test] no gateway, pass ip: eth_test ping <ip>\n");
                return -1;
            }
            ip4addr_ntoa_r(netif_ip4_gw(netif), gw, sizeof(gw));
            ip = gw;
        }
        if (argc >= 4)
            n = atoi(argv[3]);
        return eth_test_do_ping(ip, n);
    }

    if (!strcmp(argv[1], "tcp_echo"))
    {
        if (argc >= 3 && (!strcmp(argv[2], "stop") || !strcmp(argv[2], "--stop")))
            return eth_test_tcp_echo_stop();
        {
            int port = ETH_TEST_TCP_PORT_DEFAULT;
            if (argc >= 3)
                port = atoi(argv[2]);
            return eth_test_tcp_echo_start(port);
        }
    }

    if (!strcmp(argv[1], "udp_echo"))
    {
        int port = ETH_TEST_UDP_PORT_DEFAULT;
        int sec = 30;
        if (argc >= 3)
            port = atoi(argv[2]);
        if (argc >= 4)
            sec = atoi(argv[3]);
        return eth_test_udp_echo(port, sec);
    }

#ifdef BSP_USING_IPERF
    if (!strcmp(argv[1], "iperf"))
        return eth_test_do_iperf(argc - 2, argv + 2);
#endif

    eth_test_usage();
    return 0;
}
MSH_CMD_EXPORT(eth_test, "ETH test: info/ping/tcp_echo/udp_echo/iperf");

#endif /* BSP_USING_ETH */
