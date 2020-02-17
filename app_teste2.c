#include "library.c"

int main(){

  char aux[msg_max_size];
  char *dados;
  int local,fd;
  size_t tam,tam_aux;

  fd = clipboard_connect("./CLIPBOARD_SOCKET");
	if (fd == -1){
		exit(0);
	}

  printf("Onde queres escrever?(0-9)\n");
  if( fgets(aux, msg_max_size, stdin)== NULL) {
    printf("Error: fgets\n");
  }
  if (sscanf(aux, "%d", &local) == EOF){
    perror("sscanf");
  }
  printf("O que queres escrever?\n");
  if ( getline(&dados,&tam,stdin) == -1){
    perror("getline");
    free(dados);
    exit(0);
  }
  while (1){
    tam_aux = clipboard_copy(fd, local, dados, tam);
    printf("foram copidos %lu bytes para %d\n",tam_aux,local);
  }
  free(dados);
  clipboard_close(fd);
}
