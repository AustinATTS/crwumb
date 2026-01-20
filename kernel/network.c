#include "network.h"

extern void rtl8139_init(void);
extern void rtl8139_send(u8* data, u32 len);
extern void rtl8139_poll(void);
extern void rtl8139_get_mac(u8* mac);
extern void terminal_writeln(const char* s);
extern void terminal_write(const char* s);

static u8  local_mac[ETH_ALEN];
static u32 local_ip   = 0x0A00020F;
static u32 gateway_ip = 0x0A000202;
static u32 netmask    = 0xFFFFFF00;
static u8  gateway_mac[ETH_ALEN] = {0};
static u8  arp_cache[256][ETH_ALEN];
static u32 arp_cache_ips[256];
static u8  arp_cache_valid[256];
static u32 arp_cache_idx = 0;

static u16 icmp_id = 1;
static u16 icmp_seq = 0;

static u16 htons(u16 n) { return (n << 8) | (n >> 8); }
static u32 htonl(u32 n) { return ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) |
                                 ((n & 0xFF0000) >> 8) | ((n & 0xFF000000) >> 24); }
static u16 ntohs(u16 n) { return htons(n); }
static u32 ntohl(u32 n) { return htonl(n); }

static u16 checksum(void* data, u32 len) {
    u32 sum = 0;
    u8* p = (u8*)data;
    while (len > 1) {
        sum += (u16)p[0] | ((u16)p[1] << 8);
        p += 2;
        len -= 2;
    }
    if (len) sum += (u16)p[0];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

static int mac_known(u8* mac) {
    for (int i = 0; i < ETH_ALEN; i++) if (mac[i]) return 1;
    return 0;
}

static u8* get_mac_for_ip(u32 ip) {
    if (ip == gateway_ip) return gateway_mac;

    for (u32 i = 0; i < 256; i++) {
        if (arp_cache_valid[i] && arp_cache_ips[i] == ip) {
            return arp_cache[i];
        }
    }
    return 0;
}

static u8* arp_resolve_blocking(u32 ip, u32 spins) {
    u8* mac = get_mac_for_ip(ip);
    if (mac && mac_known(mac)) return mac;

    arp_request(ip);

    while (spins-- > 0) {
        net_poll();
        mac = get_mac_for_ip(ip);
        if (mac && mac_known(mac)) return mac;
        for (volatile int i = 0; i < 200; i++);
    }

    return 0;
}

static void store_mac_for_ip(u32 ip, u8* mac) {
    if (ip == gateway_ip) {
        for (int i = 0; i < ETH_ALEN; i++) {
            gateway_mac[i] = mac[i];
        }
        return;
    }

    for (u32 i = 0; i < 256; i++) {
        if (arp_cache_valid[i] && arp_cache_ips[i] == ip) {
            for (int j = 0; j < ETH_ALEN; j++) {
                arp_cache[i][j] = mac[j];
            }
            return;
        }
    }

    u32 idx = arp_cache_idx++ % 256;
    arp_cache_valid[idx] = 1;
    arp_cache_ips[idx] = ip;
    for (int i = 0; i < ETH_ALEN; i++) {
        arp_cache[idx][i] = mac[i];
    }
}

typedef struct {
    void (*init)(void);
    void (*send)(u8* data, u32 len);
    void (*poll)(void);
    void (*get_mac)(u8* mac);
} net_driver_t;

static net_driver_t* current_driver = 0;

static void rtl8139_driver_init(void) {
    rtl8139_init();
}

static void rtl8139_driver_send(u8* data, u32 len) {
    rtl8139_send(data, len);
}

static void rtl8139_driver_poll(void) {
    rtl8139_poll();
}

static void rtl8139_driver_get_mac(u8* mac) {
    rtl8139_get_mac(mac);
}

static net_driver_t rtl8139_driver = {
    .init = rtl8139_driver_init,
    .send = rtl8139_driver_send,
    .poll = rtl8139_driver_poll,
    .get_mac = rtl8139_driver_get_mac
};

void net_init(void) {
    current_driver = &rtl8139_driver;
    current_driver->init();
    current_driver->get_mac(local_mac);
}

void net_send_packet(u8* p, u32 len) {
    if (current_driver) {
        current_driver->send(p, len);
    }
}

void arp_request(u32 ip) {
    u8 pkt[sizeof(eth_header_t) + sizeof(arp_packet_t)];
    eth_header_t* e = (eth_header_t*)pkt;
    arp_packet_t* a = (arp_packet_t*)(pkt + sizeof(*e));

    for (int i = 0; i < ETH_ALEN; i++) {
        e->dest[i] = 0xFF;
        e->src[i]  = local_mac[i];
    }
    e->type = htons(ETH_TYPE_ARP);

    a->hw_type = htons(1);
    a->proto_type = htons(ETH_TYPE_IP);
    a->hw_len = ETH_ALEN;
    a->proto_len = 4;
    a->op = htons(1);
    for (int i = 0; i < ETH_ALEN; i++) {
        a->sender_mac[i] = local_mac[i];
        a->target_mac[i] = 0;
    }
    a->sender_ip = htonl(local_ip);
    a->target_ip = htonl(ip);

    net_send_packet(pkt, sizeof(pkt));
}

static void icmp_loopback(void) {
    u8 pkt[sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(icmp_header_t) + 56];
    eth_header_t* e = (eth_header_t*)pkt;
    ip_header_t* ip = (ip_header_t*)(pkt + sizeof(*e));
    icmp_header_t* ic = (icmp_header_t*)(pkt + sizeof(*e) + sizeof(*ip));
    u8* d = pkt + sizeof(*e) + sizeof(*ip) + sizeof(*ic);

    for (int i = 0; i < ETH_ALEN; i++) {
        e->src[i] = local_mac[i];
        e->dest[i] = local_mac[i];
    }
    e->type = htons(ETH_TYPE_IP);

    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = htons(sizeof(*ip) + sizeof(*ic) + 56);
    ip->id = htons(icmp_id);
    ip->flags_offset = 0;
    ip->ttl = 64;
    ip->protocol = IP_PROTO_ICMP;
    ip->src_ip = htonl(local_ip);
    ip->dest_ip = htonl(local_ip);
    ip->checksum = 0;
    ip->checksum = checksum(ip, sizeof(*ip));

    ic->type = 0;
    ic->code = 0;
    ic->id = htons(icmp_id);
    ic->sequence = htons(++icmp_seq);
    for (int i = 0; i < 56; i++) d[i] = i;
    ic->checksum = 0;
    ic->checksum = checksum(ic, sizeof(*ic) + 56);

    net_handle_packet(pkt, sizeof(pkt));
}

void icmp_send_ping(u32 dest_ip) {
    if (dest_ip == local_ip) {
        icmp_loopback();
        return;
    }

    u32 target_eth_ip = dest_ip;
    u8* target_mac;

    if ((dest_ip & netmask) != (local_ip & netmask)) {
        target_eth_ip = gateway_ip;
    }

    target_mac = get_mac_for_ip(target_eth_ip);
    if (!target_mac || !mac_known(target_mac)) {
        target_mac = arp_resolve_blocking(target_eth_ip, 200000);
        if (!target_mac) return;
    }

    u8 pkt[sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(icmp_header_t) + 56];
    eth_header_t* e = (eth_header_t*)pkt;
    ip_header_t* iph = (ip_header_t*)(pkt + sizeof(*e));
    icmp_header_t* ic = (icmp_header_t*)(pkt + sizeof(*e) + sizeof(*iph));
    u8* d = pkt + sizeof(*e) + sizeof(*iph) + sizeof(*ic);

    for (int i = 0; i < ETH_ALEN; i++) {
        e->src[i] = local_mac[i];
        e->dest[i] = target_mac[i];
    }
    e->type = htons(ETH_TYPE_IP);

    iph->version_ihl = 0x45;
    iph->tos = 0;
    iph->total_len = htons(sizeof(*iph) + sizeof(*ic) + 56);
    iph->id = htons(icmp_id);
    iph->flags_offset = 0;
    iph->ttl = 64;
    iph->protocol = IP_PROTO_ICMP;
    iph->src_ip = htonl(local_ip);
    iph->dest_ip = htonl(dest_ip);
    iph->checksum = 0;
    iph->checksum = checksum(iph, sizeof(*iph));

    ic->type = 8;
    ic->code = 0;
    ic->id = htons(icmp_id);
    ic->sequence = htons(++icmp_seq);
    for (int i = 0; i < 56; i++) d[i] = i;
    ic->checksum = 0;
    ic->checksum = checksum(ic, sizeof(*ic) + 56);

    net_send_packet(pkt, sizeof(pkt));
}

void net_handle_packet(u8* pkt, u32 len) {
    eth_header_t* e = (eth_header_t*)pkt;
    if (ntohs(e->type) == ETH_TYPE_ARP) {
        arp_packet_t* a = (arp_packet_t*)(pkt + sizeof(*e));
        if (ntohs(a->op) == 2) {
            u32 sender_ip = ntohl(a->sender_ip);
            store_mac_for_ip(sender_ip, a->sender_mac);
        }
        return;
    }

    if (ntohs(e->type) != ETH_TYPE_IP) return;

    ip_header_t* ip = (ip_header_t*)(pkt + sizeof(*e));

    if (ip->protocol == IP_PROTO_ICMP) {
        icmp_header_t* ic = (icmp_header_t*)(pkt + sizeof(*e) + sizeof(*ip));
        if (ic->type == 0) {
            terminal_write("64 bytes from ");
            char b[16];
            ip_to_str(ntohl(ip->src_ip), b);
            terminal_write(b);
            terminal_writeln(" ttl=64");
        }
    } else if (ip->protocol == IP_PROTO_TCP) {
        tcp_handle(pkt, len);
    }
}

void net_poll(void) {
    if (current_driver) {
        current_driver->poll();
    }
}

u32 str_to_ip(const char* s) {
    u32 ip = 0; u8 p = 0; u8 sh = 24;
    while (*s) {
        if (*s >= '0' && *s <= '9') p = p * 10 + (*s - '0');
        else if (*s == '.') { ip |= p << sh; sh -= 8; p = 0; }
        s++;
    }
    return ip | (p << sh);
}

void ip_to_str(u32 ip, char* b) {
    u8 p[4] = { ip>>24, ip>>16, ip>>8, ip };
    int o = 0;
    for (int i = 0; i < 4; i++) {
        if (p[i] >= 100) b[o++] = '0' + p[i]/100;
        if (p[i] >= 10)  b[o++] = '0' + (p[i]/10)%10;
        b[o++] = '0' + p[i]%10;
        if (i < 3) b[o++] = '.';
    }
    b[o] = 0;
}

void net_show_ip(void) {
    char b[16];
    terminal_write("IP: "); ip_to_str(local_ip, b); terminal_writeln(b);
    terminal_write("Gateway: "); ip_to_str(gateway_ip, b); terminal_writeln(b);
}

static u32 tcp_seq = 1;
static u32 tcp_ack = 0;

static volatile u8  tcp_http_established = 0;
static volatile u8  tcp_http_synack_received = 0;
static volatile u8  tcp_http_fin_received = 0;
static u32 tcp_http_server_ip = 0;
static u16 tcp_http_server_port = 0;
static u16 tcp_http_client_port = 0;

static u16 tcp_checksum(ip_header_t* iph, tcp_header_t* tcp, u32 tcp_len) {
    u32 sum = 0;
    u8* sip = (u8*)&iph->src_ip;
    u8* dip = (u8*)&iph->dest_ip;

    sum += (u16)sip[0] | ((u16)sip[1] << 8);
    sum += (u16)sip[2] | ((u16)sip[3] << 8);
    sum += (u16)dip[0] | ((u16)dip[1] << 8);
    sum += (u16)dip[2] | ((u16)dip[3] << 8);

    sum += htons(IP_PROTO_TCP);
    sum += htons((u16)tcp_len);

    u8* p = (u8*)tcp;
    u32 len = tcp_len;
    while (len > 1) {
        sum += (u16)p[0] | ((u16)p[1] << 8);
        p += 2;
        len -= 2;
    }
    if (len) sum += (u16)p[0];

    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (u16)(~sum);
}

static void tcp_send_raw(u32 dest_ip, u16 dest_port, u16 src_port, u32 seq, u32 ack, u16 flags, u8* data, u32 len) {
    u32 target_eth_ip = dest_ip;
    if ((dest_ip & netmask) != (local_ip & netmask)) {
        target_eth_ip = gateway_ip;
    }

    u8* target_mac = get_mac_for_ip(target_eth_ip);
    if (!target_mac || !mac_known(target_mac)) {
        target_mac = arp_resolve_blocking(target_eth_ip, 200000);
        if (!target_mac) return;
    }

    u32 tcp_len = sizeof(tcp_header_t) + len;
    u8 pkt[sizeof(eth_header_t) + sizeof(ip_header_t) + tcp_len + 12];
    eth_header_t* e = (eth_header_t*)pkt;
    ip_header_t* iph = (ip_header_t*)(pkt + sizeof(*e));
    tcp_header_t* tcp = (tcp_header_t*)(pkt + sizeof(*e) + sizeof(*iph));
    u8* tcp_data = (u8*)(tcp + 1);

    for (int i = 0; i < ETH_ALEN; i++) {
        e->src[i] = local_mac[i];
        e->dest[i] = target_mac[i];
    }
    e->type = htons(ETH_TYPE_IP);

    iph->version_ihl = 0x45;
    iph->tos = 0;
    iph->total_len = htons(sizeof(*iph) + tcp_len);
    iph->id = htons(0x1234);
    iph->flags_offset = htons(0x4000);
    iph->ttl = 64;
    iph->protocol = IP_PROTO_TCP;
    iph->src_ip = htonl(local_ip);
    iph->dest_ip = htonl(dest_ip);
    iph->checksum = 0;
    iph->checksum = checksum(iph, sizeof(*iph));

    tcp->src_port = htons(src_port);
    tcp->dest_port = htons(dest_port);
    tcp->seq = htonl(seq);
    tcp->ack = htonl(ack);
    tcp->flags = htons((5 << 12) | (flags & 0x01FF));
    tcp->window = htons(0x1000);
    tcp->checksum = 0;
    tcp->urgent = 0;

    for (u32 i = 0; i < len; i++) {
        tcp_data[i] = data[i];
    }

    tcp->checksum = tcp_checksum(iph, tcp, tcp_len);

    net_send_packet(pkt, sizeof(*e) + sizeof(*iph) + tcp_len);
}

void tcp_send(u32 dest_ip, u16 dest_port, u16 src_port, u8* data, u32 len) {
    tcp_send_raw(dest_ip, dest_port, src_port, tcp_seq, tcp_ack, 0x18, data, len);
    tcp_seq += len;
}

void tcp_handle(u8* packet, u32 len __attribute__((unused))) {
    eth_header_t* e = (eth_header_t*)packet;
    ip_header_t* ip = (ip_header_t*)(packet + sizeof(*e));

    if (ntohl(ip->dest_ip) != local_ip) return;

    tcp_header_t* tcp = (tcp_header_t*)(packet + sizeof(*e) + sizeof(*ip));
    u8* tcp_data = (u8*)(tcp + 1);

    u32 src_ip = ntohl(ip->src_ip);
    u16 src_port = ntohs(tcp->src_port);
    u16 dest_port = ntohs(tcp->dest_port);
    u16 flags = ntohs(tcp->flags);
    u8 tcp_flags = (u8)(flags & 0x3F);

    if (tcp_http_client_port && dest_port == tcp_http_client_port &&
        src_ip == tcp_http_server_ip && src_port == tcp_http_server_port) {
        if ((tcp_flags & 0x12) == 0x12) {
            tcp_http_synack_received = 1;
            tcp_http_established = 1;
            tcp_ack = ntohl(tcp->seq) + 1;
            return;
        }
        if (tcp_flags & 0x01) {
            tcp_http_fin_received = 1;
        }
    }

    if (dest_port >= 1024 && dest_port < 65535) {
        u32 data_len = ntohs(ip->total_len) - sizeof(*ip) - sizeof(*tcp);
        if (dest_port == tcp_http_client_port && tcp_http_established) {
            if (data_len > 0) {
                tcp_ack = ntohl(tcp->seq) + data_len;
                tcp_send_raw(src_ip, src_port, tcp_http_client_port, tcp_seq, tcp_ack, 0x10, 0, 0);

                u32 copy_start = http_response_len;
                for (u32 i = 0; i < data_len && (copy_start + i) < sizeof(http_response_buffer) - 1; i++) {
                    http_response_buffer[copy_start + i] = tcp_data[i];
                }
                http_response_len = copy_start + data_len;
                if (http_response_len >= sizeof(http_response_buffer)) http_response_len = sizeof(http_response_buffer) - 1;
                http_response_buffer[http_response_len] = 0;
                http_response_received = 1;
            }

            if (tcp_flags & 0x01) {
                tcp_ack = ntohl(tcp->seq) + 1;
                tcp_send_raw(src_ip, src_port, tcp_http_client_port, tcp_seq, tcp_ack, 0x10, 0, 0);
            }
        }
    }
}

char http_response_buffer[8192];
u32 http_response_len = 0;
u8 http_response_received = 0;

u32 http_request(u32 server_ip, u16 port, const char* path, u8* response, u32 max_len) {
    static u16 next_port = 50000;

    http_response_received = 0;
    http_response_len = 0;
    tcp_http_fin_received = 0;
    tcp_http_synack_received = 0;
    tcp_http_established = 0;
    tcp_http_server_ip = server_ip;
    tcp_http_server_port = port;
    tcp_http_client_port = next_port++;
    if (next_port > 60000) next_port = 50000;

    char request[512];
    char ip_str[16];
    ip_to_str(server_ip, ip_str);

    u32 pos = 0;
    const char* method = "GET ";
    for (u32 i = 0; method[i]; i++) request[pos++] = method[i];
    for (u32 i = 0; path[i]; i++) request[pos++] = path[i];
    request[pos++] = ' ';
    const char* http = "HTTP/1.0\r\nHost: ";
    for (u32 i = 0; http[i]; i++) request[pos++] = http[i];
    for (u32 i = 0; ip_str[i]; i++) request[pos++] = ip_str[i];
    const char* crlf = "\r\n\r\n";
    for (u32 i = 0; crlf[i]; i++) request[pos++] = crlf[i];
    request[pos] = 0;

    u32 target_eth_ip = server_ip;
    if ((server_ip & netmask) != (local_ip & netmask)) target_eth_ip = gateway_ip;
    if (!arp_resolve_blocking(target_eth_ip, 500000)) {
        response[0] = 0;
        return 0;
    }

    tcp_seq = 1;
    tcp_ack = 0;
    tcp_send_raw(server_ip, port, tcp_http_client_port, tcp_seq, 0, 0x02, 0, 0);
    tcp_seq += 1;

    u32 timeout = 100000000;
    while (timeout-- > 0 && !tcp_http_synack_received) {
        net_poll();
    }
    if (!tcp_http_synack_received) {
        response[0] = 0;
        return 0;
    }

    tcp_send_raw(server_ip, port, tcp_http_client_port, tcp_seq, tcp_ack, 0x10, 0, 0);

    tcp_send(server_ip, port, tcp_http_client_port, (u8*)request, pos);

    timeout = 500000000;
    u32 last_len = 0;
    u32 stable = 0;

    while (timeout-- > 0) {
        for (int i = 0; i < 20; i++) net_poll();

        if (http_response_len > last_len) {
            last_len = http_response_len;
            stable = 0;
        } else {
            stable++;
        }

        if (stable > 1000000 && http_response_len > 0) break;
        if (tcp_http_fin_received && stable > 50000) break;
    }

    u32 result_len = 0;
    if (http_response_len > 0 && http_response_len < max_len) {
        for (u32 i = 0; i < http_response_len; i++) {
            response[i] = http_response_buffer[i];
        }
        response[http_response_len] = 0;
        result_len = http_response_len;
    } else {
        response[0] = 0;
        result_len = 0;
    }

    if (tcp_http_established) {
        tcp_send_raw(server_ip, port, tcp_http_client_port, tcp_seq, tcp_ack, 0x01, 0, 0);
        tcp_seq += 1;
    }

    timeout = 10000000;
    while (timeout-- > 0) {
        net_poll();
    }

    return result_len;
}