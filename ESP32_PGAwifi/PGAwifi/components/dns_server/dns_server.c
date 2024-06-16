#include "dns_server.h"
#include <lwip/sockets.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include <byteswap.h>
#include "sdkconfig.h"

#define DEFAULT_AP_IP "192.168.4.1"

typedef struct __attribute__((__packed__)) dns_header_t
{
    uint16_t ID;
    uint8_t RD : 1;
    uint8_t TC : 1;
    uint8_t AA : 1;
    uint8_t OPCode : 4;
    uint8_t QR : 1;
    uint8_t RCode : 4;
    uint8_t RESERVED : 3;
    uint8_t RA : 1;
    uint16_t QDCount;
    uint16_t ANCount;
    uint16_t NSCount;
    uint16_t ARCount;
} dns_header_t;

typedef struct __attribute__((__packed__)) dns_answer_t
{
    uint16_t NAME;
    uint16_t TYPE;
    uint16_t CLASS;
    uint32_t TTL;
    uint16_t RDLENGTH;
    uint32_t RDATA;
} dns_answer_t;

static TaskHandle_t task_handler = NULL;
static int socket_fd = 0;
static const int DNS_QUERY_MAX_SIZE = 80;
static const int DNS_ANSWER_MAX_SIZE = DNS_QUERY_MAX_SIZE + 16;
static int port = 53;

static void init_server(void *param);
static void update_response(char *response_buffer);

void dns_start()
{
    if (task_handler == NULL)
        xTaskCreate(&init_server, "DNSService", 4096, NULL, configMAX_PRIORITIES, &task_handler);
}

void dns_stop()
{
    if (task_handler)
    {
        close(socket_fd);
        vTaskDelete(task_handler);
        task_handler = NULL;
    }
}

void init_server(void *param)
{
    struct sockaddr_in dns_server;
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
        return;

    dns_server.sin_family = AF_INET;
    dns_server.sin_addr.s_addr = INADDR_ANY;
    dns_server.sin_port = htons(port);

    if (bind(socket_fd, (struct sockaddr *)&dns_server, sizeof(struct sockaddr_in)) != -1)
    {
        struct sockaddr_in dns_client;
        socklen_t dns_client_len = sizeof(dns_client);
        char request_buffer[DNS_QUERY_MAX_SIZE];
        int request_buffer_length = 0;
        char response_buffer[DNS_ANSWER_MAX_SIZE];
        ip4_addr_t ip_resolved;
        inet_pton(AF_INET, DEFAULT_AP_IP, &ip_resolved);
        int status;

        while (true)
        {
            memset(request_buffer, 0x00, sizeof(request_buffer));
            request_buffer_length = recvfrom(socket_fd, request_buffer, sizeof(request_buffer), 0, (struct sockaddr *)&dns_client, &dns_client_len); /* read udp request */

            if (request_buffer_length > 0 && ((request_buffer_length + sizeof(dns_answer_t) - 1) < DNS_ANSWER_MAX_SIZE))
            {

                request_buffer[request_buffer_length] = '\0';
                memcpy(response_buffer, request_buffer, sizeof(dns_header_t));
                update_response(response_buffer);

                memcpy(response_buffer + sizeof(dns_header_t), request_buffer + sizeof(dns_header_t), request_buffer_length - sizeof(dns_header_t));

                dns_answer_t *dns_answer = (dns_answer_t *)&response_buffer[request_buffer_length];
                dns_answer->NAME = __bswap_16(0xC00C);
                dns_answer->TYPE = __bswap_16(1);
                dns_answer->CLASS = __bswap_16(1);
                dns_answer->TTL = (uint32_t)0x00000000;
                dns_answer->RDLENGTH = __bswap_16(0x0004);
                dns_answer->RDATA = ip_resolved.addr;

                status = sendto(socket_fd, response_buffer, request_buffer_length + sizeof(dns_answer_t), 0, (struct sockaddr *)&dns_client, dns_client_len);
            }
        }
    }

    close(socket_fd);
    vTaskDelete(NULL);
}

void update_response(char *response_buffer)
{
    dns_header_t *dns_header = (dns_header_t *)response_buffer;
    dns_header->QR = 1;
    dns_header->OPCode = 0;
    dns_header->AA = 1;
    dns_header->TC = 0;
    dns_header->RD = 0;
    dns_header->RCode = 0;
    dns_header->ANCount = dns_header->QDCount;
    dns_header->NSCount = 0x0000;
    dns_header->ARCount = 0x0000;
}
