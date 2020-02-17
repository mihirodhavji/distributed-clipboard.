#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>

#include "clipboard.h"

#define msg_max_size 100

typedef struct  info1estrutura{
    int local;
    int opcao;
    size_t size;
}info;

size_t send_info(info info1, int acc)
{
  char *msg_i = NULL;
  size_t n_bytes;

  msg_i = (char *) malloc(sizeof(info));
  if (msg_i == NULL){
    perror("malloc");
    return -1;
  }
  if (memcpy(msg_i,&info1,sizeof(info)) == NULL){
    perror("memcpy");
    return -1;
  }

  n_bytes = send(acc, msg_i, sizeof(info),0);
  if (n_bytes == -1){
    perror("send");
    return -1;
  }
  free(msg_i);
  return n_bytes;
}

///////////////////////////////////////////////////

size_t send_data(size_t size, int acc, char *dados)
{
  size_t n_bytes_sum,n_bytes;
  n_bytes_sum = 0;
  char *aux = dados;
  while (n_bytes_sum != size)
  {
    n_bytes = send(acc, aux + n_bytes_sum, size - n_bytes_sum, 0);
    if (n_bytes == -1){
      perror("send");
      return -1;
    }
    n_bytes_sum = n_bytes_sum + n_bytes;
  }

  return n_bytes_sum;
}

///////////////////////////////////////////////////

int clipboard_connect(char * clipboard_dir)
{
    int s;
    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, clipboard_dir);

    if((s = socket(AF_UNIX,SOCK_STREAM,0)) == -1) {
        perror("socket \n");
        return -1;
    }

    if (connect(s,( struct sockaddr *) &server,sizeof(server)) == -1){
        perror("connect");
        return -1;
    }
    return s;
}

///////////////////////////////////////////////////

void clipboard_close(int clipboard_id)
{
    if (close(clipboard_id) == -1){
        perror("socket close");
        ///////////duvida
    }
    return;
}

///////////////////////////////////////////////////

int clipboard_wait(int clipboard_id, int region, void *buf, size_t count)
{
    if (region > 9 || region < 0){
        printf("Local Invalido\n");
        return 0;
    }
    if (count <= 0){
        printf("Tamanho Invalido\n");
        return 0;
    }
    if (buf == NULL){
        printf("pointer is NULL\n");
        return 0;
    }

    char *msg_i = NULL;
    info info1;
    char *msg_m = NULL;
    size_t n_bytes_sum, n_bytes;

    info1.opcao = 3;
    info1.local = region;

    //////////enviar info para o server
    n_bytes = send_info(info1, clipboard_id);
    if (n_bytes == -1){
        return 0;
    }
    ////// receber info do server
    msg_i = (char *)malloc(sizeof(info));
    if(msg_i == NULL) {
        perror("msg allocation");
        exit(1);
    }
    n_bytes = recv(clipboard_id, msg_i, sizeof(info), 0);
    if (n_bytes == -1){
        perror("recv");
        return 0;
    }
    if (memcpy(&info1, msg_i, sizeof(info)) == NULL){
        perror("memcpy");
        exit(1);
    }
    free(msg_i);
    /////////receber a palavra completa do server
    msg_m = (char *)malloc(sizeof(char) * info1.size);
    if(msg_m == NULL) {
        perror("msg allocation\n");
        exit(1);
    }
    n_bytes_sum = 0;
    while (n_bytes_sum != info1.size)
    {
        n_bytes = recv(clipboard_id, msg_m + n_bytes_sum , sizeof(char) * (info1.size - n_bytes_sum),0);
        if (n_bytes == -1){
            perror("recv");
            return 0;
        }
        n_bytes_sum = n_bytes + n_bytes_sum;
    }
    printf("recv %lu bytes\n",n_bytes_sum);

    ///////// copiar para o buf
    if ( info1.size >= count){
        if (memcpy(buf, msg_m, count * sizeof(char)) == NULL){
            perror("memcpy");
            exit(1);
        }
        free(msg_m);
        return count;
    }
    else{
        if (memcpy(buf, msg_m, info1.size * sizeof(char)) == NULL){
            perror("memcpy");
            exit(1);
        }
        free(msg_m);
        return info1.size;
    }
    return 0;
}

///////////////////////////////////////////////////

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count)
{
    if (region > 9 || region < 0){
        printf("Local Invalido\n");
        return 0;
    }
    if (count <= 0){
        printf("Tamanho Invalido\n");
        return 0;
    }
    if (buf == NULL){
        printf("pointer is NULL\n");
        return 0;
    }

    size_t n_bytes;
    info info1;
    char *msg_i = NULL;
    char *msg_m = NULL;

    info1.opcao = 2;
    info1.local = region;
    info1.size = count;

    /* envio da struct com as infomacoes necessarias*/
    n_bytes = send_info(info1, clipboard_id);
    if (n_bytes == -1){
        return 0;
    }
    printf("sent %lu bytes\n", n_bytes);

    /* envio da mensagem*/
    msg_m = (char *)malloc(sizeof(char) * count);
    if(msg_m == NULL) {
        perror("msg allocation");
        exit(1);
    }
    if (memcpy(msg_m,buf,count) == NULL){
        perror("memcpy");
        exit(1);
    }
    n_bytes = send_data(count, clipboard_id, msg_m);

    printf("sent %lu bytes\n", n_bytes);
    printf("Operação concluída.\n");
    free(msg_i);
    free(msg_m);
    return n_bytes;
}

///////////////////////////////////////////////////

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count)
{
    if (region > 9 || region < 0){
        printf("Local Invalido\n");
        return 0;
    }
    if (count <= 0){
        printf("Tamanho Invalido\n");
        return 0;
    }
    if (buf == NULL){
        printf("pointer is NULL\n");
        return 0;
    }

    int n_bytes;
    size_t n_bytes_sum = 0;
    info info1;
    char *msg_i = NULL;
    char *msg_m = NULL;

    /* envio estrutura a pedir donde quero info*/
    info1.opcao = 1;
    info1.local = region;
    n_bytes = send_info(info1, clipboard_id);
    if (n_bytes == -1){
        return 0;
    }
    printf("sent %d bytes\n", n_bytes);

    /* receber estrutura com o tamanho das coisas que vou receber*/
    msg_i = (char *)malloc(sizeof(info));
    if(msg_i == NULL) {
        perror("msg allocation");
        exit(1);
    }
    n_bytes = recv(clipboard_id, msg_i, sizeof(info), 0);
    if (n_bytes == -1){
        perror("recv");
        return 0;
    }
    if( memcpy(&info1, msg_i, sizeof(info)) == NULL){
        perror("memcpy");
        exit(1);
    }
    free(msg_i);
    /* se nao houver palavra nesse local*/
    if (info1.size == 0) {
        printf("Não existe informação no local %d\n",region);
        return 1;
    }
    else{ //// se houver palavra
        msg_m = (char *)malloc(sizeof(char) * info1.size);
        if(msg_m == NULL) {
            perror("msg allocation");
            exit(1);
        }
        n_bytes_sum = 0;
        while (n_bytes_sum != info1.size)
        {
            n_bytes = recv(clipboard_id, msg_m + n_bytes_sum , sizeof(char) * (info1.size - n_bytes_sum),0);
            if (n_bytes == -1){
                perror("recv");
                return 0;
            }
            n_bytes_sum = n_bytes + n_bytes_sum;
        }
        printf("recv %lu bytes\n",n_bytes_sum);

        if ( info1.size >= count){
            if (memcpy(buf, msg_m, count * sizeof(char)) == NULL){
                perror("memcpy");
                exit(1);
            }
            free(msg_m);
            return count;
        }
        else{
            if (memcpy(buf, msg_m, info1.size * sizeof(char)) == NULL){
                perror("memcpy");
                exit(1);
            }
            free(msg_m);
            return info1.size;
        }
    }
    return 0;
}
