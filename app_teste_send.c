#include "library.c"


int main(int argc, char* argv[])
{

	int fd;
	size_t size;
	char *dados = NULL;
	fd = clipboard_connect("./CLIPBOARD_SOCKET");
	if (fd == -1){
		exit(0);
	}
	FILE *picture;
	picture = fopen(argv[1], "r");
	fseek(picture, 0, SEEK_END);
	size = ftell(picture);
	fseek(picture, 0, SEEK_SET);

	dados = (char *)malloc(sizeof(char) * size);
	if (dados == NULL){
		perror("malloc");
		exit(0);

	}

	fread(dados, size, 1, picture);
	clipboard_copy(fd,0,dados,size);

	clipboard_close(fd);

	exit(0);
}
