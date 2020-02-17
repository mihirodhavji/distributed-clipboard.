#include "library.c"


int main()
{

	int fd;
	size_t size = 10000000;
	char *dados = NULL;
	FILE *image;
	fd = clipboard_connect("./CLIPBOARD_SOCKET");
	if (fd == -1){
		exit(0);
	}
	dados = (char *)malloc(sizeof(char) * size);
	if (dados == NULL){
		perror("malloc");
		exit(0);

	}
	size = clipboard_paste(fd,0,dados,size);
	if (size == 0){exit(0);}

	image = fopen("c1.png", "w");
	fwrite(dados, 1, size, image);
	fclose(image);

	clipboard_close(fd);

	exit(0);
}
