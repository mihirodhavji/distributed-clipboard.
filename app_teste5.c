#include "library.c"

int main()
{
	char aux[msg_max_size];
	char *dados=NULL, *aux_d=NULL;
	char *aux_p = NULL;
	int local, flag = 1, fd;
	char option;
	size_t tam;

	fd = clipboard_connect("./CLIPBOARD_SOCKET");
	if (fd == -1){
		exit(0);
	}
	while(flag)
	{
		printf("Pretende ler, escrever, sair ou wait?[l/e/s/w]?\n");
		option=getchar();
		if(option != '\n')	getchar();
		switch(option)
			{
				case 'w':
				{
					printf("Onde queres fazer wait?(0-9)\n");
					if(fgets(aux, msg_max_size, stdin) == NULL){
						printf("Error: fgets\n");
						break;
					}
					if( sscanf(aux, "%d", &local) == EOF){
						printf("Error: could not read data\n");
						break;
					}
					printf("Quantos carateres queres receber?\n");
					if (fgets(aux, msg_max_size, stdin) == NULL) {
					printf("Error: fgets\n");
					break;
					}
					sscanf(aux, "%lu", &tam);
					if (tam <=0){
						printf("tamanho invalido\n");
						break;
					}
					aux_d = (char *)malloc((tam) * sizeof(char));
					if (aux_d == NULL){
						perror("malloc");
						exit(0);
					}
					tam = clipboard_wait(fd,local,aux_d,tam);
					printf("wait %lu bytes\n",tam);
					aux_d[tam] = '\0';
					printf("wait  %s\n",aux_d);

					free(aux_d);
					aux_d = NULL;
					break;
				}
				case 's':
				{
					clipboard_close(fd);
					flag = 0;
					break;
				}
				case 'l':
				{
					printf("Onde queres ler?(0-9)\n");
					if(fgets(aux, msg_max_size, stdin) == NULL){
						printf("Error: fgets\n");
						break;
					}
					if( sscanf(aux, "%d", &local) == EOF){
						printf("Error: could not read data\n");
						break;
					}
					printf("Quantos carateres queres receber?\n");
					if (fgets(aux, msg_max_size, stdin) == NULL) {
					printf("Error: fgets\n");
					break;
					}
					sscanf(aux, "%lu", &tam);
					if (tam <=0){
						printf("tamanho invalido\n");
						break;
					}
					aux_p = (char *)malloc(sizeof(char)* (tam));
					if (aux_p == NULL){
						perror("malloc");
						exit(0);
					}
					tam = clipboard_paste(fd, local, aux_p, tam);
					printf("paste  %lu bytes\n",tam);
					aux_p[tam] = '\0';
					printf("paste  %s\n",aux_p);
					free(aux_p);
					aux_p = NULL;
					break;
				}
				case 'e':
				{
					printf("Onde queres escrever?(0-9)\n");
					if( fgets(aux, msg_max_size, stdin)== NULL) {
						printf("Error: fgets\n");
						break;
					}
					sscanf(aux, "%d", &local);
					printf("O que queres escrever?\n");
					if ( getline(&dados,&tam,stdin) == -1){
						perror("getline");
						free(dados);
						exit(0);
					}
					tam = clipboard_copy(fd, local, dados, tam);
					printf("copy %lu bytes\n",tam);
					free(dados);
					dados = NULL;
					break;
				}
				default:printf("Opção Inválida\n");
			}
	}
	exit(0);
}
