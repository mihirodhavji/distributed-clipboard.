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

  printf("Onde queres ler?(0-9)\n");
  if( fgets(aux, msg_max_size, stdin)== NULL) {
    printf("Error: fgets\n");
  }
  if (sscanf(aux, "%d", &local) == EOF){
    perror("sscanf");
  }

  printf("Quantos carateres queres ler?\n");
  if (fgets(aux, msg_max_size, stdin) == NULL) {
    printf("Error: fgets\n");
  }
  if (sscanf(aux, "%lu", &tam) == EOF){
    perror("sscanf");
  }

  dados = (char *)malloc(sizeof(char) * (tam));
  if(dados == NULL) {
    perror("msg allocation");
    exit(1);
  }
  while (1){
    tam_aux = clipboard_paste(fd, local, dados, tam);
    printf("foram lidos %lu bytes de %d\n",tam_aux,local);
  }
  free(dados);
  clipboard_close(fd);
}
