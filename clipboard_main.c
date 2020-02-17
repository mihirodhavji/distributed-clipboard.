#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#define PASTE 1
#define COPY 2
#define WAIT 3
#define NAOGUARDA 4
#define GUARDA 5
#define pai 2
#define filho 1
#define unconnected 0
#define connected 1
#define clip_max_regions 10

typedef struct  info_estrutura{
  int local;
  int opcao;
  size_t size;
}info;

typedef struct clip{
  size_t size;
  pthread_cond_t cond;
  pthread_mutex_t mux;
  pthread_rwlock_t lock;
  char *dados;
}clip;


typedef struct node_c{
  int acc_con;
  pthread_t pthr;
  pthread_rwlock_t lock;
  int rel;
  int sock;
  struct node_c *next;
}node_con;

typedef struct node_a{
  int acc_con;
  int sock;
  pthread_t pthr;
  pthread_rwlock_t lock;
  struct node_a *next;
}node_app;

typedef struct  estrutura_SAM{
  /* estrutura para a thread/funcao single app manager*/
  clip *clipboard;
  int acc;
  node_con *head_clip;
  node_app *head_app;
}estrutura_SAM;

typedef struct  estrutura_CBL{
  /*  estrutura para a enviar o clipboard*/
  clip *clipboard;
  node_con *head_clip;
  node_app *head_app;
  int *mode;
}estrutura_CBL;

typedef struct  estrutura_CM{
  /*  estrutura para a enviar o clipboard*/
  clip *clipboard;
  node_con *head_clip;
  node_app *head_app;
  node_con *current;
  int *mode;
}estrutura_CM;

static int key = 1;

void desligar ()
{
  key = 0;
  printf("\nkey is now false\n");
  return;
}

///////////////////////////////////////////////////

void limpeza_function(node_app *head_app,node_con *head_clip,clip *clipboard)
{
  int i;
  node_app *aux_app, *node_app1;
  node_con *aux, *node;

  for(i = 0;i < clip_max_regions; i++){

    free(clipboard[i].dados);

    if ( pthread_cond_destroy(&clipboard[i].cond) != 0 ){
      perror("cond destroy");
      exit(1);
    }
    if ( pthread_mutex_destroy(&clipboard[i].mux) != 0){
      perror("mux destroy");
      exit(1);
    }
    if ( pthread_rwlock_destroy(&clipboard[i].lock) != 0){
      perror("rwlock destroy");
      exit(1);
    }
  }

  free(clipboard);
  /// limpar listas/*
  if (pthread_rwlock_wrlock(&head_clip->lock) != 0){
    perror("rdlock");
    exit(1);
  }

  node = head_clip->next;
  while(node != NULL)
  {
    aux = node;
    node = node->next;
    if (aux->rel == 1){
      if ( close (aux->acc_con) == -1){
        perror("socket close");
      }
      if (shutdown(aux->sock,SHUT_RDWR) == -1){
        perror("socket shutdown");
      }
    }
    free(aux);
  }
  if (pthread_rwlock_unlock(&head_clip->lock) != 0){
    perror("unlock");
    exit(1);
  }

  if ( pthread_rwlock_destroy(&head_clip->lock) != 0){
    perror("rwlock destroy");
    exit(1);
  }

  free(head_clip);

  if (pthread_rwlock_wrlock(&head_app->lock) != 0){
    perror("rdlock");
    exit(1);
  }

  node_app1 = head_app->next;
  while(node_app1 != NULL)
  {
    aux_app = node_app1;
    node_app1 = node_app1->next;
    if ( close (aux_app->acc_con) == -1){
      perror("socket close");
    }
    if ( shutdown(aux_app->sock,SHUT_RDWR) == -1){
      perror("socket shutdown");
    }
    free(aux_app);
  }
  if (pthread_rwlock_unlock(&head_app->lock) != 0){
    perror("unlock");
    exit(1);
  }
  if ( pthread_rwlock_destroy(&head_app->lock) != 0){
    perror("rwlock destroy");
    exit(1);
  }
  free(head_app);
  return ;
}

///////////////////////////////////////////////////

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
  while (n_bytes_sum != size)
  {
    n_bytes = send(acc, dados + n_bytes_sum, size - n_bytes_sum, 0);
    if (n_bytes == -1){
      perror("send");
      return -1;
    }
    n_bytes_sum = n_bytes_sum + n_bytes;
  }

  return n_bytes_sum;
}

///////////////////////////////////////////////////

int enviar_todos(char *dados, info info1, node_con *head_clip)
{
  node_con *aux = head_clip->next;
  size_t n_bytes_sum;
  //sends information to connected clipboards
  while(aux != NULL)
  {
    if(aux->acc_con >= 0 && aux->rel == 0){
      while (aux->rel == 0){}
    }

    if(aux->rel == filho){
      if (send_info(info1, aux->acc_con) == -1){
        return 1;
      }
      n_bytes_sum = send_data(info1.size, aux->acc_con, dados);
      if (n_bytes_sum == -1){
        return 1;
      }
      printf("sent(copy) %lu bytes\n",n_bytes_sum);
    }
    aux = aux->next;
  }
  return 0;
}

///////////////////////////////////////////////////

void imprimir_clip(clip *clipboard, node_app *head_app,node_con *head_clip )
{
  int i = 0;
  size_t size;

  for ( i = 0; i < clip_max_regions; i++){
    if (pthread_rwlock_rdlock(&clipboard[i].lock) != 0){
      perror("wrlock");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }

    size = clipboard[i].size;

    if (pthread_rwlock_unlock(&clipboard[i].lock) != 0){
      perror("unlock");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }
    printf("Local %d: %lu bytes\n",i, size);
  }
  return ;
}

///////////////////////////////////////////////////

void actualiza_clip(info info1, clip *clipboard,char *dados)
{
  char *dados_aux = NULL;
  dados_aux = clipboard[info1.local].dados;
  clipboard[info1.local].size = info1.size;
  clipboard[info1.local].dados = dados;
  free(dados_aux);

  return;
}

///////////////////////////////////////////////////

int enviar_para_pai(char *dados, info info1,node_con *head, node_app *head_app,clip *clipboard)
{
  size_t n_bytes;
  node_con *aux = head->next;
  while (aux != NULL)
  {
    if (aux->rel == pai){
      break;
    }
    aux=aux->next;
  }
  if (aux == NULL){//// ele é que é o pai
    info1.opcao = GUARDA;
    if (pthread_rwlock_wrlock(&clipboard[info1.local].lock) != 0){
      perror("wrlock");
      limpeza_function (head_app,head,clipboard);
      free(dados);
      exit(1);
    }

    actualiza_clip(info1, clipboard,dados);

    if (pthread_rwlock_unlock(&clipboard[info1.local].lock) != 0){
      perror("unlock");
      limpeza_function (head_app,head,clipboard);
      exit(1);
    }

    /// broadcast
    if ( pthread_mutex_lock(&clipboard[info1.local].mux) != 0){
      perror("mux lock");
      limpeza_function (head_app,head,clipboard);
      exit(1);
    }

    if ( pthread_cond_broadcast(&clipboard[info1.local].cond) != 0){
      perror("cond broadcast");
      limpeza_function (head_app,head,clipboard);
      exit(1);
    }

    if ( pthread_mutex_unlock(&clipboard[info1.local].mux) != 0){
      perror("mux unlock");
      limpeza_function (head_app,head,clipboard);
      exit(1);
    }
    if ( enviar_todos(dados, info1, head) == -1){
      return -1;
    }
  }
  else{

    n_bytes = send_info(info1,aux->acc_con);
    if (n_bytes == -1){
      return -1;
    }
    n_bytes = send_data(info1.size, aux->acc_con, dados);
    if (n_bytes == -1){
      return -1;
    }
  }
  return n_bytes;
}

///////////////////////////////////////////////////

void *single_app_connection(void * estrutura_SAM2)
{
  int n_bytes,flag = 0;
  estrutura_SAM *estrutura_SAM1 = (estrutura_SAM *)estrutura_SAM2;
  int a = estrutura_SAM1->acc;
  clip *clipboard = estrutura_SAM1->clipboard;
  node_con *head_clip = estrutura_SAM1->head_clip;
  node_app *head_app = estrutura_SAM1->head_app;
  info info1;
  size_t n_bytes_sum;
  char *dados = NULL;
  char *msg_i=NULL;
  if (pthread_detach(pthread_self()) != 0){
    perror("pthread_detach");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }
  msg_i = (char *)malloc(sizeof(info));
  if(msg_i == NULL) {
    perror("malloc");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }
  printf("App connected\n");

  while((n_bytes = recv(a, msg_i, sizeof(info),0)) != 0)
  {
    if (n_bytes == -1){
      perror("recv");
      break;
    }printf("recv\n");
    if ( memcpy(&info1,msg_i,sizeof(info)) == 0){
      perror("memcpy");
      limpeza_function (head_app,head_clip,clipboard);

      exit(1);
    }
    free(msg_i);
    msg_i = NULL;

    if (info1.opcao == WAIT){/// app faz wait
      if ( pthread_mutex_lock(&clipboard[info1.local].mux) != 0){
        perror("mux lock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      if (pthread_cond_wait(&clipboard[info1.local].cond,&clipboard[info1.local].mux)!= 0){
        perror("cond wait");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      if ( pthread_mutex_unlock(&clipboard[info1.local].mux) != 0){
        perror("mux unlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      if (pthread_rwlock_rdlock(&clipboard[info1.local].lock) != 0){
        perror("rdlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }

      info1.size = clipboard[info1.local].size;
      dados = clipboard[info1.local].dados;

      if (pthread_rwlock_unlock(&clipboard[info1.local].lock) != 0){
        perror("unlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }

      if( send_info(info1, a) == -1){
        break;
      }

      n_bytes_sum = send_data(info1.size, a, dados);
      if (n_bytes_sum == -1){
        break;
      }
      printf("sent(wait) %lu bytes\n",n_bytes_sum);
      dados = NULL;
    }
    else if (info1.opcao == PASTE){ /// app fez paste
      if (pthread_rwlock_rdlock(&clipboard[info1.local].lock) != 0){
        perror("rdlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }

      info1.size = clipboard[info1.local].size;

      if (pthread_rwlock_unlock(&clipboard[info1.local].lock) != 0){
        perror("unlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }

      if ( send_info(info1, a) == -1){
        break;
      }

      if (info1.size==0) {
        continue;
      }
      else{
        if (pthread_rwlock_rdlock(&clipboard[info1.local].lock) != 0){
          perror("rdlock");
          limpeza_function (head_app,head_clip,clipboard);
          exit(1);
        }
        dados = clipboard[info1.local].dados;
        if (pthread_rwlock_unlock(&clipboard[info1.local].lock) != 0){
          perror("unlock");
          limpeza_function (head_app,head_clip,clipboard);
          exit(1);
        }
        n_bytes_sum = send_data(info1.size, a, dados);
        if (n_bytes_sum == -1){
          flag = 1;
          break;
        }
        printf("sent(paste) %lu bytes\n",n_bytes_sum);
        dados = NULL;
      }
    }
    else if (info1.opcao == COPY){ /// app coloca info no clip
      dados = (char *)malloc(sizeof(char) * info1.size);
      if(dados == NULL) {
        perror("malloc");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }

      n_bytes_sum = 0;/// receber mensagem
      while (n_bytes_sum != info1.size)
      {
        n_bytes = recv(a, dados + n_bytes_sum, sizeof(char) * (info1.size - n_bytes_sum),0);
        if (n_bytes == -1){
          perror("recv");
          flag = 1;
          break;
        }
        n_bytes_sum = n_bytes + n_bytes_sum;
      }

      if (flag == 1){break;}

      printf("recv %lu bytes\n",n_bytes_sum);
      info1.opcao = NAOGUARDA;
      if (pthread_rwlock_wrlock(&head_clip->lock) != 0){
        perror("rdlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      if (enviar_para_pai(dados, info1, head_clip, head_app,clipboard) == -1){
        flag = 1;
      }

      if (pthread_rwlock_unlock(&head_clip->lock) != 0){
        perror("unlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      if (flag == 1){
        break;
      }

    }
    ///// prep msg_i para o novo ciclo
    msg_i = (char *)malloc(sizeof(info));
    if(msg_i == NULL){
      perror("estrutura allocation");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }
  }
  printf("App has closed\n");
  if(msg_i != NULL){
    free(msg_i);
  }
  estrutura_SAM2 = estrutura_SAM1;
  return 0;
}

///////////////////////////////////////////////////

void * APP_manager (void * estrutura_CBL2)
{
  estrutura_CBL *estrutura_CBL1 = (estrutura_CBL *)estrutura_CBL2;
  clip *clipboard = estrutura_CBL1->clipboard;
  estrutura_SAM estrutura_SAM1;
  node_app *aux_app;
  node_app *head_app = estrutura_CBL1->head_app;
  node_con *head_clip = estrutura_CBL1->head_clip;
  int pth = 0,sock_id,a;
  socklen_t tam;
  if (pthread_detach(pthread_self()) != 0){
    perror("pthread_detach");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }
  /***********************************
  addr é teu add para falares com a app
  addr_app é o add da app
  ************************************/
  struct sockaddr_un addr, addr_app;

  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, "CLIPBOARD_SOCKET");

  if ((sock_id = socket(AF_UNIX,SOCK_STREAM, 0)) == -1){
    perror("socket");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }

  if ( bind(sock_id,(struct sockaddr *)&addr, sizeof(addr)) == -1 ){
    perror("bind");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }

  if (listen(sock_id,100) == -1){
    perror("listen");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }

  while(key)
  {
    tam = sizeof(addr_app);

    a = accept(sock_id,( struct sockaddr *) &addr_app,&tam);
    if (a == -1){
      perror("accept");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }

    aux_app = (node_app*)malloc(sizeof(node_app));
    if (aux_app == NULL){
      perror("con_node_app allocation");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }
    aux_app->acc_con = a;
    aux_app->sock = sock_id;
    if (pthread_rwlock_wrlock(&head_app->lock) != 0){
      perror("rdlock");
      exit(1);
    }

    aux_app->next = head_app->next;
    head_app->next = aux_app;

    if (pthread_rwlock_unlock(&head_app->lock) != 0){
      perror("unlock");
      exit(1);
    }

    estrutura_SAM1.acc = aux_app->acc_con;
    estrutura_SAM1.clipboard = clipboard;
    estrutura_SAM1.head_clip = head_clip;
    estrutura_SAM1.head_app = estrutura_CBL1->head_app;
    pth = pthread_create(&(aux_app->pthr), NULL, &single_app_connection, &(estrutura_SAM1));
    if (pth != 0){
      perror("pthread_create");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }
  }
  printf("saiu do MAM\n");
  return 0;
}

///////////////////////////////////////////////////

void *single_clipboard_connection(void *estrutura_CM2)
{
  int n_bytes,flag = 0;
  estrutura_CM *estrutura_CM1 = (estrutura_CM *)estrutura_CM2;
  node_con *head_clip = estrutura_CM1->head_clip;
  node_con *current = estrutura_CM1->current;
  node_app *head_app = estrutura_CM1->head_app;
  clip *clipboard = estrutura_CM1->clipboard;
  char *msg_i = NULL;
  char *msg_m = NULL;
  char *msg_aux = NULL;
  info info1;
  int *mode = estrutura_CM1->mode;

  size_t n_bytes_sum = 0;
  if (pthread_detach(pthread_self()) != 0){
    perror("pthread_detach");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }
  msg_i = (char *)malloc(sizeof(info));
  if (msg_i == NULL){
    perror("malloc");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }

  while((n_bytes = recv(current->acc_con, msg_i, sizeof(info),0)) != 0)
  {
    if (n_bytes == -1){
      perror("recv");
      break;
    }

    if ( memcpy(&info1,msg_i,sizeof(info)) == 0){
      perror("memcpy");
      limpeza_function (head_app,head_clip,clipboard);
      free(msg_i);
      exit(1);
    }

    msg_m = (char *)malloc(sizeof(char) * info1.size);
    if (msg_m == NULL){
      perror("malloc");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }
    free(msg_i);
    msg_i = NULL;
    n_bytes_sum = 0;

    while (n_bytes_sum != info1.size)
    {
      n_bytes = recv(current->acc_con, msg_m + n_bytes_sum, (info1.size - n_bytes_sum) * sizeof(char),0);
      if (n_bytes == -1){
        perror("recv");
        flag = 1;
        break;
      }
      n_bytes_sum = n_bytes_sum + n_bytes;
    }

    if (flag == 1){break;}
    if (*mode == connected && info1.opcao == NAOGUARDA){
      if (pthread_rwlock_wrlock(&head_clip->lock) != 0){
        perror("rdlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      if (enviar_para_pai(msg_m, info1, head_clip,head_app,clipboard) == -1){
        printf("pai unreachable\n");
        free(msg_m);
        continue;
      }

      if (pthread_rwlock_unlock(&head_clip->lock) != 0){
        perror("unlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
    }
    else if (*mode == unconnected || (info1.opcao == GUARDA) ){
      if (pthread_rwlock_wrlock(&clipboard[info1.local].lock) != 0){
        perror("wrlock");
        limpeza_function (head_app,head_clip,clipboard);
        free(msg_m);
        exit(1);
      }

      actualiza_clip(info1, clipboard,msg_m);

      if (pthread_rwlock_unlock(&clipboard[info1.local].lock) != 0){
        perror("unlock");
        limpeza_function (head_app,head_clip,clipboard);
        free(msg_aux);
        exit(1);
      }
      /// broadcast
      if ( pthread_mutex_lock(&clipboard[info1.local].mux) != 0){
        perror("mux lock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      if ( pthread_cond_broadcast(&clipboard[info1.local].cond) != 0){
        perror("cond broadcast");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      if ( pthread_mutex_unlock(&clipboard[info1.local].mux) != 0){
        perror("mux unlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }

      info1.opcao = GUARDA;
      if (pthread_rwlock_wrlock(&head_clip->lock) != 0){
        perror("rdlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      if ( enviar_todos(msg_m, info1, head_clip) == -1){
        free(msg_m);
        continue;
      }

      if (pthread_rwlock_unlock(&head_clip->lock) != 0){
        perror("unlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
    }
    printf("imprimir clip no SUM\n");
    imprimir_clip(clipboard,head_app,head_clip);
    msg_i = (char *)malloc(sizeof(info));
    if (msg_i == NULL){
      perror("malloc");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }
  }

  if (current->rel == pai){
    *mode = unconnected;
  }
  current->acc_con = -10;
  current->rel = 0;
  if (msg_i != NULL){
    free(msg_i);
  }
  printf("SUM saiu\n");
  return 0;
}

///////////////////////////////////////////////////

void * CLIPBOARD_manager(void *estrutura_CBL2)
{
  estrutura_CBL *estrutura_CBL1 = (estrutura_CBL *)estrutura_CBL2;
  clip *clipboard = estrutura_CBL1->clipboard;
  struct sockaddr_in local_add, client_add;
  int i,pth,flag = 0;
  size_t n_bytes_sum;
  socklen_t tam;
  node_app *head_app = estrutura_CBL1->head_app;
  node_con *head_clip = estrutura_CBL1->head_clip;
  node_con *aux;
  estrutura_CM estrutura_CM1;
  int a,s,aux_p;
  char *msg_m = NULL;
  info info1;

  if (pthread_detach(pthread_self()) != 0){
    perror("pthread_detach");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }

  if ((s = socket(AF_INET,SOCK_STREAM, 0)) == -1){
    perror("socket");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }

  aux_p = 8100;
  do{
    local_add.sin_family = AF_INET;
    local_add.sin_port = htons(aux_p);
    local_add.sin_addr.s_addr = INADDR_ANY;
    aux_p--;
    if ( aux_p < 8000){
      aux_p = 8100;
    }
  }while (bind (s,(struct sockaddr *)&local_add, sizeof(local_add)) == -1);

  printf("port : %d\n",aux_p+1);

  if ( listen(s,100) == -1){
    perror("listen");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }

  while(key)
  {
    tam = sizeof(client_add);
    a = accept(s,(struct sockaddr *) &client_add,&tam);
    if (a == -1){
      perror("accept");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }

    aux = (node_con *)malloc(sizeof(node_con));
    if (aux == NULL){
      perror("con_node allocation");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }

    aux->acc_con = a;
    aux->sock = s;
    aux->rel = 0;
    if (pthread_rwlock_wrlock(&head_clip->lock) != 0){
      perror("wrlock");
      limpeza_function (head_app,head_clip,clipboard);
      free(aux);
      exit(1);
    }

    aux->next = head_clip->next;
    head_clip->next = aux;

    if (pthread_rwlock_unlock(&head_clip->lock) != 0){
      perror("unlock");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }
    for(i = 0;i < clip_max_regions; i++){/// enviar o estado atual do clipboard
      /// enviar estrutura com as info
      info1.local = i;
      if (pthread_rwlock_rdlock(&clipboard[info1.local].lock) != 0){
        perror("rdlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }
      info1.size = clipboard[i].size;
      if (pthread_rwlock_unlock(&clipboard[info1.local].lock) != 0){
        perror("unlock");
        limpeza_function (head_app,head_clip,clipboard);
        exit(1);
      }

      if (send_info(info1, aux->acc_con) == -1){
        break;
      }
      /// enviar mensagem
      if (info1.size != 0){
        msg_m = NULL;
        if (pthread_rwlock_rdlock(&clipboard[info1.local].lock) != 0){
          perror("rdlock");
          limpeza_function (head_app,head_clip,clipboard);
          exit(1);
        }
        msg_m = clipboard[info1.local].dados;

        if (pthread_rwlock_unlock(&clipboard[info1.local].lock) != 0){
          perror("unlock");
          limpeza_function (head_app,head_clip,clipboard);
          exit(1);
        }
        n_bytes_sum = send_data(info1.size, aux->acc_con,msg_m);
        if (n_bytes_sum == -1){
          flag = 1;
          break;
        }
        printf("sent(CM) %lu bytes\n",n_bytes_sum);
      }
    }
    if (flag == 1){
      if (close(aux->acc_con) == -1){
        perror("socket close");
      }
      if (shutdown(aux->sock,SHUT_RDWR) == -1){
        perror("socket shutdown");
      }
      free(aux);
      continue;
    }
    aux->rel = filho;
    estrutura_CM1.clipboard = clipboard;
    estrutura_CM1.head_clip = head_clip;
    estrutura_CM1.current = aux;
    estrutura_CM1.head_app = head_app;
    estrutura_CM1.mode = estrutura_CBL1->mode;
    pth = pthread_create(&(aux->pthr), NULL, &single_clipboard_connection, &estrutura_CM1);
    if (pth != 0){
      perror("pthread_create");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }
  }
  printf("Sai do CM\n");
  return 0;
}

///////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  unlink("CLIPBOARD_SOCKET");
  clip *clipboard;
  clipboard = NULL;
  clipboard = (clip *)malloc(clip_max_regions * sizeof(clip));
  if (clipboard == NULL){
    perror("clip malloc");
    exit(1);
  }
  struct sigaction sa = {{0}};

  pthread_mutexattr_t attr;

  node_con *head_clip = NULL;
  head_clip = (node_con *)malloc(sizeof(node_con));
  if(head_clip == NULL){
    perror("malloc head_clip_con");
    free(clipboard);
    exit(1);
  }
  head_clip-> acc_con = -10; /* valor absurdo*/
  head_clip->next = NULL;
  head_clip->rel = 0; /* relevancia*/

  node_app *head_app;
  head_app = (node_app *)malloc(sizeof(node_app));
  if(head_app == NULL){
    perror("malloc head_app");
    free(clipboard);
    free(head_clip);
    exit(1);
  }
  head_app-> acc_con = -10; /* valor absurdo*/
  head_app->next = NULL;

  estrutura_CM estrutura_CM1;
  /************************************
  remote_add é o add do teu fornecedor de info
  **************************************/
  struct sockaddr_in remote_add;
  info info1;
  int i;
  int n_bytes,pth;
  estrutura_CBL estrutura_CBL1;
  pthread_t pthr_MAM,pthr_CM;
  char *msg_i = NULL;
  char *msg_m = NULL;
  /// se modo = 1 estamos no connected mode
  int mode = 3;

  /// verificação de erros na inicialização do programa
  if(argc != 1 && argc != 4){
    printf("error incorrect command line\n");
    exit(1);
  }
  if (argc == 1)mode = unconnected;
  if (argc == 4 && (strcmp(argv[1],"-c") == 0)) mode = connected;
  if (mode == 3){
    printf("error incorrect command line\n");
    exit(1);
  }

  if ( pthread_mutexattr_init(&attr) != 0){
    perror("mutexattr_init");
    exit(1);
  }
  if (pthread_rwlock_init(&head_clip->lock, NULL) != 0){
    perror("rwlock init");
    exit(1);
  }
  if (pthread_rwlock_init(&head_app->lock, NULL) != 0){
    perror("rwlock init");
    exit(1);
  }
  sa.sa_handler = desligar;
  if ( sigaction(SIGINT, &sa, NULL) != 0){
    perror("sigaction");
    exit(0);
  }
  signal(SIGPIPE,SIG_IGN);

  if(mode == unconnected){	/// unconnected mode
    for(i = 0;i < clip_max_regions; i++){
      if(( clipboard[i].dados = (char *)malloc(1 * sizeof(char))) == NULL){
        perror("local region allocation");
        exit(1);
      }
      clipboard[i].size = 0;
      if ( pthread_cond_init(&clipboard[i].cond,NULL) != 0){
        perror("cond init");
        exit(1);
      }
      if ( pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK) != 0 ){
        perror("mutex settype");
        exit(1);
      }
      if ( pthread_mutex_init(&clipboard[i].mux, &attr) != 0){
        perror("mutex init");
        exit(1);
      }
      if (pthread_rwlock_init(&clipboard[i].lock, NULL) != 0){
        perror("rwlock init");
        exit(1);
      }
    }
    estrutura_CBL1.mode = &mode;
  }
  else{ /// connected mode
    /// ligação ao remote
    head_clip->next = malloc(sizeof(node_con));
    if(head_clip->next == NULL){
      perror("malloc head_clip_con");
      exit(1);
    }
    head_clip->next->rel = 0;
    if ((head_clip->next->acc_con  = socket(AF_INET,SOCK_STREAM,0)) == -1){
      perror("socket");
      exit(1);
    }
    if (pthread_rwlock_init(&head_clip->next->lock, NULL) != 0){
      perror("rwlock init");
      exit(1);
    }
    head_clip->next->next = NULL;
    if (head_clip->next->acc_con == -1){
      perror("socket");
      exit(1);
    }
    if ( memset(&remote_add,0,sizeof(struct sockaddr_in)) == 0){
      perror("memset");
      exit(1);
    }
    remote_add.sin_family = AF_INET;
    remote_add.sin_port = htons(atoi(argv[3]));
    if(inet_aton(argv[2],&remote_add.sin_addr) == 0){
      perror("inet_aton");
      exit(1);
    }
    if ( connect (head_clip->next->acc_con, (const struct sockaddr *) &remote_add, sizeof(remote_add)) == -1){
      perror("connect");
      exit(1);
    }
    /* create clipboard positions and receive clipboard current state */
    for(i = 0;i < clip_max_regions; i++){
      if ( pthread_cond_init(&clipboard[i].cond,NULL) != 0){
        perror("cond init");
        exit(1);
      }

      if ( pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK) != 0 ){
        perror("mutex settype");
        exit(1);
      }
      if ( pthread_mutex_init(&clipboard[i].mux, &attr) != 0){
        perror("mutex init");
        exit(1);
      }
      if (pthread_rwlock_init(&clipboard[i].lock, NULL) != 0){
        perror("rwlock init");
        exit(1);
      }
      /* receber estrutura com info da mensagem */
      msg_i = (char *)malloc(sizeof(info));
      if (msg_i == NULL){
        perror("malloc");
        exit(1);
      }
      n_bytes = recv(head_clip->next->acc_con, msg_i, sizeof(info),0);
      if (n_bytes == -1){
        perror("recv");
        exit(1);
      }

      if ( memcpy(&info1,msg_i,sizeof(info)) == 0){
        perror("memcpy");
        exit(1);
      }

      free(msg_i);
      msg_i = NULL;
      /// receber mensagem
      if (info1.size != 0){ ///se houver
        msg_m = (char *)malloc((info1.size)*sizeof(char));
        if (msg_m == NULL){
          perror("malloc");
          exit(1);
        }
        size_t n_bytes_sum = 0;
        while (n_bytes_sum != info1.size)
        {
          n_bytes = recv(head_clip->next->acc_con, msg_m + n_bytes_sum, info1.size - n_bytes_sum, 0);
          if (n_bytes == -1){
            perror("recv");
            exit(1);
          }
          n_bytes_sum = n_bytes_sum + n_bytes;
        }
        printf("recv %lu bytes\n",n_bytes_sum);
        clipboard[info1.local].size = info1.size;
        if((clipboard[info1.local].dados = (char *)malloc(info1.size*sizeof(char))) == NULL){
          perror("local region allocation");
          exit(1);
        }
        if ( memcpy(clipboard[info1.local].dados, msg_m, info1.size*sizeof(char)) == 0){
          perror("memcpy");
          exit(1);
        }
        free(msg_m);
        msg_m = NULL;
      }
      else{ /// se nao tiver mensagem
        clipboard[info1.local].size = info1.size;
        if((clipboard[info1.local].dados = (char *)malloc(1 * sizeof(char))) == NULL){
          perror("local region allocation");
          exit(1);
        }
        if ( memcpy(clipboard[info1.local].dados, "\0", info1.size*sizeof(char)) == 0){
          perror("memcpy");
          exit(1);
        }
      }
      printf("Local %d: %lu bytes\n",i, clipboard[i].size);
    }

    head_clip->next->rel = pai;
    estrutura_CM1.clipboard = clipboard;
    estrutura_CM1.head_clip = head_clip;
    estrutura_CM1.current = head_clip->next;
    estrutura_CM1.head_app = head_app;
    estrutura_CM1.mode = &mode;
    pth = pthread_create(&(head_clip->pthr), NULL, &single_clipboard_connection, &estrutura_CM1);
    if (pth != 0){
      perror("pthread_create");
      limpeza_function (head_app,head_clip,clipboard);
      exit(1);
    }
    estrutura_CBL1.mode = &mode;
  }

  /// thread para tratar das apps
  estrutura_CBL1.clipboard = clipboard;
  estrutura_CBL1.head_clip = head_clip;
  estrutura_CBL1.head_app = head_app;
  pth = pthread_create(&pthr_MAM, NULL, &APP_manager, &estrutura_CBL1);
  if (pth != 0){
    perror("pthread_create");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }

  /// thread para tratar das connections
  pth = pthread_create(&pthr_CM, NULL, &CLIPBOARD_manager, &estrutura_CBL1);
  if (pth != 0){
    perror("pthread_create");
    limpeza_function (head_app,head_clip,clipboard);
    exit(1);
  }

  while(key){ }

  if ( pthread_mutexattr_destroy(&attr) != 0){
    perror("mutexattr_destroy");
    exit(1);
  }
  limpeza_function (head_app,head_clip,clipboard);
  printf("adeus\n");
  return 0;
}
