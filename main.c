#include "ft_ping.h"

int loop = 1;

void handle_quit() 
{
    loop = 0;
}

float get_ms(struct timeval start, struct timeval end)
{
    return ((end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0);
}

void add_stat(Stats** head, float value) {
    Stats* new = (Stats*)malloc(sizeof(Stats));
    new->value = value;
    new->next = NULL;

    if (*head == NULL) {
        *head = new;
    } else {
        Stats* tmp = *head;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = new;
    }
}

char *get_host(char *ip)
{
    struct hostent *host;
    struct in_addr addr;
    addr.s_addr = inet_addr(ip);
    host = gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);
    if (host) return host->h_name;
    return NULL;
}

void print_verbose(char *packet)
{        
    struct ip *ip_hdr = (struct ip *)packet;
    int hdr_size = ip_hdr->ip_hl * 4;

    char *orig_packet = packet + hdr_size + 8;

    struct ip *orig_ip_hdr = (struct ip *)orig_packet;
    int orig_ip_header_size = orig_ip_hdr->ip_hl * 4;

    struct icmp *orig_icmp_hdr = (struct icmp *)(orig_packet + orig_ip_header_size);

    char *dst_ip = strdup(inet_ntoa(orig_ip_hdr->ip_dst));
    char *src_ip = strdup(inet_ntoa(orig_ip_hdr->ip_src));

    int len = orig_ip_hdr->ip_hl << 2;

    printf("IP Hdr Dump:\n ");
    for (size_t i = 0; i < sizeof(struct ip); i++) {
        printf("%02x", ((unsigned char *)orig_ip_hdr)[i]);
        if ((i + 1) % 2 == 0) printf(" ");
    }
    printf("\n");
    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst    Data\n");
    printf(" %1x  %1x  %02x %04x %04x %2x %04x %02x %02x %04x %s %s\n",
        orig_ip_hdr->ip_v,
        orig_ip_hdr->ip_hl,
        orig_ip_hdr->ip_tos,
        ntohs(orig_ip_hdr->ip_len) >> 8,
        ntohs(orig_ip_hdr->ip_id),
        (ntohs(orig_ip_hdr->ip_off) >> 13) & 0x07,
        orig_ip_hdr->ip_off,
        orig_ip_hdr->ip_ttl,
        orig_ip_hdr->ip_p,
        ntohs(orig_ip_hdr->ip_sum),
        src_ip,
        dst_ip
    );
    printf("ICMP: type %d, size %d, id 0x%04x, seq 0x%04x\n", orig_icmp_hdr->icmp_type, orig_ip_hdr->ip_len - len, orig_icmp_hdr->icmp_id, ntohs(orig_icmp_hdr->icmp_seq) >> 8);
    if (src_ip) free(src_ip);
    if (dst_ip) free(dst_ip);
}

int main(int ac, char **av)
{
    if (ac < 2)
    {
        printf("%s: missing host operand\nTry './ft_ping -?' for more information.\n", av[0]);
        return 1;
    }
    ParsedArgs args = parse_args(ac, av);
    if (args.usage) {
        if (args.address) free(args.address);
        return print_usage(args, av[0]);
    }
    if (getuid() != 0)
    {
        free(args.address);
        printf("%s: You have to be superuser to use this program\n", av[0]);
        return 1;
    }
    char *ip = resolve_hostname(args.address);
    if (!ip) {
        free(args.address);
        printf("%s: unknown host\n", av[0]);
        return 1;
    }
    int sock = init_socket(args);
    if (sock < 0) 
    {
        free(ip);
        free(args.address);
        perror("socket");
        return 1;
    }

    signal(SIGINT, handle_quit);
    if (args.timeout > 0) alarm(args.timeout);

    struct sockaddr_in dest_addr;
    Stats *stats = NULL;

    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(ip);

    int packet_send = 0;
    int packet_recv = 0;


    printf("PING %s (%s): %d data bytes", args.address, ip, args.size);
    if (args.verbose)
    {
        int pid = getpid();
        printf(", 0x%04x = %d", pid, pid);
    }
    printf("\n");

    int recv_packet_size = args.size + sizeof(struct icmp) - sizeof(struct ip);
    int error = 1;

    while (loop) 
    {
        char *packet = build_packet(args);
        char recv_packet[recv_packet_size];
        bzero(recv_packet, recv_packet_size);
        int packet_size;
        struct timeval send, recv;
        int lost = 0;

        gettimeofday(&send, NULL);
        if (sendto(sock, packet, args.size + sizeof(struct icmp), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) 
        {
            printf("ft_ping: sending packet: %s\n", strerror(errno));
            loop = error = 0;
            free(packet);
            continue;
        }
        else packet_send++;
        if ((packet_size = recvfrom(sock, &recv_packet, recv_packet_size, 0, NULL, NULL)) < 0) lost = 1;
        else packet_recv++;
        gettimeofday(&recv, NULL);
        struct ip *ip_hdr = (struct ip *)recv_packet;
        struct icmp *icmp_hdr = (struct icmp *)(recv_packet + (ip_hdr->ip_hl << 2));
        if (!lost && icmp_hdr->icmp_type == ICMP_TIMXCEED)
        {
            char *recv_ip = inet_ntoa(ip_hdr->ip_src);
            char *host = get_host(recv_ip);
            if (host) printf("%d bytes from %s (%s): Time to live exceeded\n", packet_size, host, recv_ip);
            else printf("%d bytes from %s: Time to live exceeded\n", packet_size, recv_ip);
            if (args.verbose) print_verbose(recv_packet);
            packet_recv--;
        }
        else if (!lost && !args.quiet)
        {
            float ms = get_ms(send, recv);
            printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", packet_size, ip, icmp_hdr->icmp_seq, ip_hdr->ip_ttl, ms);
            add_stat(&stats, ms);
        }
        if (!lost && loop) usleep(args.interval * 1000);
        free(packet);
    }
    if (error) {
        printf("--- %s ping statistics ---\n", args.address);
        printf("%d packets transmitted, %d received, %d%% packet loss\n", packet_send, packet_recv, (packet_send - packet_recv) * 100 / packet_send);
        calculate_stats(stats);
    }
    close(sock);
    Stats* tmp;
    while (stats != NULL) {
        tmp = stats;
        stats = stats->next;
        free(tmp);
    }
    free(args.address);
    free(ip);
    return 0;
}