#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define PACKET_SIZE 64
#define TIMEOUT 1

// Calculează checksum-ul pachetului
unsigned short checksum(void *b, int len) {
    unsigned short *buf = (unsigned short *)b;
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

// Crează pachetul ICMP
void create_packet(struct icmphdr *icmp_hdr) {
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->code = 0;
    icmp_hdr->un.echo.id = getpid();
    icmp_hdr->un.echo.sequence = 1;
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = checksum(icmp_hdr, sizeof(struct icmphdr));
}

// Trimite un pachet ICMP Echo Request și așteaptă răspunsul
int ping(const char *ip_address) {
    struct sockaddr_in addr;
    int sockfd;
    struct icmphdr icmp_hdr;
    char packet[PACKET_SIZE];
    struct timeval start, end;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_address);

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Enable broadcast option
    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("Error in setting Broadcast option");
        close(sockfd);
        return 1;
    }

    memset(packet, 0, PACKET_SIZE);
    create_packet(&icmp_hdr);
    memcpy(packet, &icmp_hdr, sizeof(icmp_hdr));

    gettimeofday(&start, NULL);
    if (sendto(sockfd, packet, sizeof(icmp_hdr), 0, (struct sockaddr *)&addr, sizeof(addr)) <= 0) {
        perror("Send failed");
        close(sockfd);
        return 1;
    }

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    char recv_packet[PACKET_SIZE];
    socklen_t addr_len = sizeof(addr);

    if (recvfrom(sockfd, recv_packet, PACKET_SIZE, 0, (struct sockaddr *)&addr, &addr_len) <= 0) {
        if (errno == EAGAIN) {
            std::cout << "Request timed out." << std::endl;
        } else {
            perror("Receive failed");
        }
        close(sockfd);
        return 1;
    } else {
        gettimeofday(&end, NULL);
        double time_taken = (end.tv_sec - start.tv_sec) * 1000.0;
        time_taken += (end.tv_usec - start.tv_usec) / 1000.0;
        std::cout << "Ping successful. Time taken: " << time_taken << " ms" << std::endl;
    }

    close(sockfd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <IP address>" << std::endl;
        return 1;
    }

    return ping(argv[1]);
}
