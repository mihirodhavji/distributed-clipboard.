int clipboard_connect(char * clipboard_dir);
void clipboard_close(int clipboard_id);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
