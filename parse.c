#include "ft_ping.h"

// -v verbose
// -? usage
ParsedArgs parse_args(int ac, char **av)
{
    ParsedArgs args;
    args.address = NULL;
    args.usage = 0;
    args.verbose = 0;
    args.error = NULL;
    args.interval = 1;
    args.size = PACKET_SIZE;
    args.timeout = 1;
    args.ttl = TTL;
    args.timestamp = 0;
    int i = 1;
    while (i < ac)
    {
        if (!strcmp(av[i], "-v"))
            args.verbose = 1;
        else if (!strcmp(av[i], "-?"))
            args.usage = 1;
        else if (!strcmp(av[i], "-i")) // Wait time between packet
            args.interval = atof(av[++i]) * 1000;
        else if (!strcmp(av[i], "-s")) // Packet size
            args.size = atoi(av[++i]);
        else if (!strcmp(av[i], "-W")) // Wait for reply
            args.timeout = atoi(av[++i]);
        else if (!strcmp(av[i], "-t")) // Time to live
            args.ttl = atoi(av[++i]);
        else if (!strcmp(av[i], "-D")) // Timestamp
            args.timestamp = 1;
        else if (av[i][0] == '-')
        {
            args.usage = 1;
            args.error = av[i];

        }
        else {
            if (args.address)
                free(args.address);
            args.address = strdup(av[i]);
        }
        i++;
    }
    return args;
}

int print_usage(ParsedArgs args)
{
    if (args.error)
    {
        char c;
        int i = 0;
        int n = strlen(args.error);
        while (i < n)
        {
            if (args.error[i] != '-' && args.error[i] != '?' && args.error[i] != 'v')
            {
                c = args.error[i];
                break;
            }
            i++;
        }
        if (n) printf("ft_ping: invalid option -- '%c'\n", c);
    }
    printf("\nUsage\n  ft_ping [options] <destination>\n\nOptions:\n");
    printf("  <destination>   dns name or ip address\n");
    printf("  -v              verbose output\n");
    printf("  -?              print help and exit\n");
    return 2;
}