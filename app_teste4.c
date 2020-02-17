#include "library.c"

int main()
{
	size_t tam;
	char *dados=NULL;
	int fd = clipboard_connect("./CLIPBOARD_SOCKET");
	if (fd == -1){
		exit(0);
	}

	tam = clipboard_wait(fd,10,dados,27);
	printf("return (wait) %lu\n",tam);
	tam = clipboard_wait(fd,-1,dados,27);
	printf("return (wait) %lu\n",tam);
	tam = clipboard_wait(fd,3,dados,0);
	printf("return (wait) %lu\n",tam);
	tam = clipboard_wait(fd,3,dados,-6);
	printf("return (wait) %lu\n",tam);
	tam = clipboard_wait(fd,3,NULL,4);
	printf("return (wait) %lu\n",tam);

	tam = clipboard_copy(fd,10,dados,25);
	printf("return (copy) %lu\n",tam);
	tam = clipboard_copy(fd,-3,dados,27);
	printf("return (copy) %lu\n",tam);
	tam = clipboard_copy(fd,0,dados,0);
	printf("return (copy) %lu\n",tam);
	tam = clipboard_copy(fd,1,dados,-9);
	printf("return (copy) %lu\n",tam);
	tam = clipboard_copy(fd,1,NULL,3);
	printf("return (copy) %lu\n",tam);

	tam = clipboard_paste(fd,10,dados,25);
	printf("return (paste) %lu\n",tam);
	tam = clipboard_paste(fd,-3,dados,27);
	printf("return (paste) %lu\n",tam);
	tam = clipboard_paste(fd,0,dados,0);
	printf("return (paste) %lu\n",tam);
	tam = clipboard_paste(fd,1,dados,-9);
	printf("return (paste) %lu\n",tam);
	tam = clipboard_paste(fd,1,NULL,3);
	printf("return (paste) %lu\n",tam);

	tam = clipboard_copy(fd,1,"a",30);
	printf("return (copy) %lu\n",tam);
	clipboard_close(fd);
	exit(0);
}
