#include "ft_ping.h"

char *resolve_hostname(char *hostname) 
{
    struct addrinfo hints, *result, *elem;
    char *ip = malloc(INET_ADDRSTRLEN);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) 
    {
        free(ip);
        return NULL;
    }
    elem = result;
    while (elem != NULL)
    {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)elem->ai_addr;
        void *addr = &(ipv4->sin_addr);
        if (inet_ntop(AF_INET, addr, ip, INET_ADDRSTRLEN) != NULL) {
            freeaddrinfo(result);
            return ip;
        }
        elem = elem->ai_next;
    }
    freeaddrinfo(result);
    free(ip);
    return NULL;
}

int init_socket(ParsedArgs args)
{
    int sock;
    struct timeval timeout;
    int ttl = args.ttl;

    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) return -1;

    if (setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1) {
		close(sock);
		return -1;
	}

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
        close(sock);
        return -1;
    }

    return sock;
}

unsigned short checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

char *build_packet(ParsedArgs args)
{
    static int seq = 0;
    char *packet = malloc(args.size + sizeof(struct icmp));
    struct icmp *icmp = (struct icmp *)(packet);

    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_seq = seq++;
    icmp->icmp_id = getpid();
    icmp->icmp_cksum = 0;

    for (unsigned int i = 0; i < args.size; i++) *(packet + sizeof(struct icmp) + i) = i;

    icmp->icmp_cksum = checksum(packet, args.size + sizeof(struct icmp));

    return packet;
}

void calculate_stats(Stats *stats)
{
    float min = 0, max = 0, avg = 0, dev = 0, sum = 0, sum_sq = 0;
    int count = 0;
    Stats *current = stats;

    if (current == NULL) return;

    min = max = current->value;

    while (current != NULL) {
        float value = current->value;
        if (value < min) min = value;
        if (value > max) max = value;
        sum += value;
        sum_sq += value * value;
        count++;
        current = current->next;
    }

    avg = sum / count;
    dev = sqrt((sum_sq / count) - (avg * avg));

    printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", min, max, avg, dev);

}