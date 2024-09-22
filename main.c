#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>



typedef struct ParsedArgs {
    char *address;
    int verbose;
    int usage;
    char *error;
} ParsedArgs;

// -v verbose
// -? usage
ParsedArgs parse_args(int ac, char **av)
{
    ParsedArgs args;
    args.address = NULL;
    args.usage = 0;
    args.verbose = 0;
    args.error = NULL;
    int i = 1;
    while (i < ac)
    {
        if (!strcmp(av[i], "-v"))
            args.verbose = 1;
        else if (!strcmp(av[i], "-?"))
            args.usage = 1;
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
        printf("ft_ping: invalid option -- '%c'\n", c);
    }
    printf("\nUsage\n  ft_ping [options] <destination>\n\nOptions:\n");
    printf("  <destination>   dns name or ip address\n");
    printf("  -v              verbose output\n");
    printf("  -?              print help and exit\n");
    return 2;
}

int main(int ac, char **av)
{
    ParsedArgs args = parse_args(ac, av);
    if (args.usage) {
        if (args.address)
            free(args.address);
        return print_usage(args);
    }
    printf("Address : %s --- V : %d  --- U : %d", args.address, args.verbose, args.usage);
}