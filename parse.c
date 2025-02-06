#include "ft_ping.h"

// -v verbose
// -? usage

int is_valid_number(char *str, int neg, int flo)
{
    int i = 0;
    while (str[i])
    {
        if (str[i] < '0' || str[i] > '9' || (i == 0 && str[i] == '-' && !neg) || (str[i] == '.' && !flo))
            return 0;
        i++;
    }
    return 1;
}

ParsedArgs missing_arg(ParsedArgs args, char param)
{
    args.usage = 1;
    sprintf(args.error, "option requires an argument -- '%c'", param);
    return args;
}

ParsedArgs invalid_arg(ParsedArgs args, char *param)
{
    args.usage = 1;
    sprintf(args.error, "invalid value (`%s' near `%s')", param, param);
    return args;
}

ParsedArgs parse_args(int ac, char **av)
{
    ParsedArgs args;
    args.address = NULL;
    args.usage = 0;
    args.verbose = 0;
    bzero(args.error, 100);
    args.interval = 1000;
    args.size = PACKET_SIZE;
    args.timeout = -1;
    args.ttl = TTL;
    args.quiet = 0;
    int i = 1;
    while (i < ac)
    {
        if (!strcmp(av[i], "-v"))
            args.verbose = 1;
        else if (!strcmp(av[i], "-?"))
            args.usage = 1;
        else if (!strcmp(av[i], "-i")) // Wait time between packet
        {
            char *value = av[++i];
            if (!value || !strlen(value)) return missing_arg(args, 'i');
            if (!is_valid_number(value, 1, 1)) return invalid_arg(args, value);
            args.interval = atof(value) * 1000;
        }
        else if (!strcmp(av[i], "-s"))
        {
            char *value = av[++i];
            if (!value || !strlen(value)) return missing_arg(args, 's');
            if (!is_valid_number(value, 1, 0)) return invalid_arg(args, value);
            long v = atol(av[i]);
            if (v < 0 || v > INT32_MAX) {
                args.usage = 1;
                sprintf(args.error, "option value too big: %s", value);
                return args;
            }
            args.size = atoi(value);
        }
        else if (!strcmp(av[i], "-w")) // Wait x second and cut
        {
            char *value = av[++i];
            if (!value || !strlen(value)) return missing_arg(args, 'w');
            if (!is_valid_number(value, 1, 0)) return invalid_arg(args, value);
            long v = atol(value);
            if (v < 0 || v > INT32_MAX) {
                args.usage = 1;
                sprintf(args.error, "option value too big: %s", value);
                return args;
            }

            args.timeout = atoi(value);
        }
        else if (!strcmp(av[i], "--ttl"))
        {
            char *value = av[++i];
            if (!value || !strlen(value)) 
            {
                args.usage = 1;
                sprintf(args.error, "option '--ttl' requires an argument");
                return args;
            }
            if (!is_valid_number(value, 1, 0)) return invalid_arg(args, value);
            long v = atol(value);
            if (v < 0 || v > INT32_MAX) {
                args.usage = 1;
                sprintf(args.error, "option value too big: %s", value);
                return args;
            }
            args.ttl = atoi(value);
        }
        else if (!strcmp(av[i], "-q")) // Quiet
            args.quiet = 1;
        else if (av[i][0] == '-')
        {
            args.usage = 1;
            sprintf(args.error, "invalid option -- '%s'", av[1]);

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

int print_usage(ParsedArgs args, char *prog)
{
    if (strlen(args.error))
    {
        printf("%s: %s\n", prog, args.error);
        printf("Try 'ft_ping -?' for more information.\n");
        return 64;
    }
    printf("\nUsage:  ft_ping [OPTION...] HOST ...\n");
    printf("Send ICMP ECHO_REQUEST packets to network hosts\n\n");
    printf("  -i              wait NUMBER seconds between sending each packet\n");
    printf("  --ttl           specify N as time-to-live\n");
    printf("  -v              verbose output\n");
    printf("  -w              stop after N seconds\n");
    printf("  -q              quiet output\n");
    printf("  -s              send NUMBER data octets\n");
    printf("  -?              give this help list\n");
    return 2;
}