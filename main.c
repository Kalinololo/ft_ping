#include "ft_ping.h"

float get_ms(struct timeval start, struct timeval end)
{
    return ((end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0);
}

int main(int ac, char **av)
{
    if (getuid() != 0)
    {
        printf("ft_ping: Operation not permitted\n");
        return 1;
    }
    if (ac < 2)
        return print_usage((ParsedArgs){NULL, 0, 1, NULL, TTL, 1, PACKET_SIZE, 1, 0});
    ParsedArgs args = parse_args(ac, av);
    if (args.usage) {
        if (args.address)
            free(args.address);
        return print_usage(args);
    }

    char *ip = resolve_hostname(args.address);
    if (!ip) {
        printf("ft_ping: %s: Name or service not known\n", args.address);
        return 1;
    }
    int sock = init_socket(args);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in dest_addr;

    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(ip);

    int packet_send = 0;
    int packet_recv = 0;

    printf("PING %s (%s): %d data bytes\n", args.address, ip, args.size);

    while (1) 
    {
        char *packet = build_packet(args);
        char recv_packet[args.size + sizeof(struct ip) + sizeof(struct icmp)];
        int packet_size;
        struct timeval send, recv;
        int lost = 0;

        gettimeofday(&send, NULL);
        if (sendto(sock, packet, args.size + sizeof(struct icmp), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) 
        {
            perror("sendto");
            exit(1);
        }
        else packet_send++;
        if ((packet_size = recvfrom(sock, &recv_packet, args.size + sizeof(struct ip) + sizeof(struct icmp), 0, NULL, NULL)) < 0) 
        {
            printf("Request timeout for icmp_seq %d\n", ((struct icmp*)(packet))->icmp_seq);
            lost = 1;
        }
        else packet_recv++;
        gettimeofday(&recv, NULL);
        if (!lost)
        {
            struct ip *ip_hdr = (struct ip *)recv_packet;
            struct icmp *icmp_hdr = (struct icmp *)(recv_packet + (ip_hdr->ip_hl << 2));

            (void)packet_recv;
            (void)packet_send;
            printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%f ms\n", packet_size, ip, icmp_hdr->icmp_seq, ip_hdr->ip_ttl, get_ms(send, recv));
            
            sleep(1);
        }
        free(packet);
    }
}