#ifndef SV_FILE_H
#define SV_FILE_H

typedef struct {
	char file_name[256];
	pid_t pid;
	int flags;
}svfile_u2k_args_st;

#endif
