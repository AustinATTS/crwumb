#ifndef NETWORK_H
#define NETWORK_H

#include "types.h"

#define ETH_ALEN 6
#define ETH_TYPE_IP 0x0800
#define ETH_TYPE_ARP 0x0806

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

typedef struct {
    u8 dest[ETH_ALEN];
    u8 src[ETH_ALEN];
    u16 type;
} __attribute__((packed)) eth_header_t;

typedef struct {
    u8 version_ihl;
    u8 tos;
    u16 total_len;
    u16 id;
    u16 flags_offset;
    u8 ttl;
    u8 protocol;
    u16 checksum;
    u32 src_ip;
    u32 dest_ip;
} __attribute__((packed)) ip_header_t;

typedef struct {
    u16 hw_type;
    u16 proto_type;
    u8 hw_len;
    u8 proto_len;
    u16 op;
    u8 sender_mac[ETH_ALEN];
    u32 sender_ip;
    u8 target_mac[ETH_ALEN];
    u32 target_ip;
} __attribute__((packed)) arp_packet_t;

typedef struct {
    u8 type;
    u8 code;
    u16 checksum;
    u16 id;
    u16 sequence;
} __attribute__((packed)) icmp_header_t;

typedef struct {
    u16 src_port;
    u16 dest_port;
    u32 seq;
    u32 ack;
    u16 flags;
    u16 window;
    u16 checksum;
    u16 urgent;
} __attribute__((packed)) tcp_header_t;

void net_init(void);
void net_send_packet(u8* packet, u32 len);
void net_receive(void);
void net_handle_packet(u8* packet, u32 len);
void net_poll(void);

void arp_request(u32 ip);
void icmp_reply(u8* packet, u32 len);
void icmp_send_ping(u32 dest_ip);

u32 str_to_ip(const char* str);
void ip_to_str(u32 ip, char* buf);
void net_show_ip(void);

void tcp_send(u32 dest_ip, u16 dest_port, u16 src_port, u8* data, u32 len);
void tcp_handle(u8* packet, u32 len);
u32 http_request(u32 server_ip, u16 port, const char* path, u8* response, u32 max_len);

extern char http_response_buffer[8192];
extern u32 http_response_len;
extern u8 http_response_received;

#endif
