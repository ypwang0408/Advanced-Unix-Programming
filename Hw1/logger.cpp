#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string>
using namespace std;
int main(int argc, char *argv[]) {
    int opt;
    char* output_file = NULL;
    char* so_path = NULL;
    while ((opt = getopt(argc, argv, "o:p:")) != -1) {
        switch (opt) {
            case 'o':
                output_file = optarg;
                FILE *tmp_outfile;
                tmp_outfile = fopen(output_file, "w");
                fclose(tmp_outfile);
                break;
            case 'p':
                so_path = optarg;
                break;
            default:
                fprintf(stderr, "Usage: ./logger [-o file] [-p sopath] command [args...]\n");
                exit(EXIT_FAILURE);
        }
    }
    
    char *arguments[argc - optind] = {0};
    arguments[0] = argv[optind + 1];
    for (int i = optind + 2; i < argc; i++) {
        arguments[i - optind - 1] = argv[i];
    }
    arguments[argc - optind - 1] = NULL;

    char *env[] = {0, 0, 0, 0};
    env[0] = (char*)malloc(100);
    env[1] = (char*)malloc(100);
    env[2] = (char*)malloc(100);
    if (so_path != NULL) 
        sprintf(env[0], "LD_PRELOAD=%s", so_path);
    else
        sprintf(env[0], "LD_PRELOAD=./logger.so");
    sprintf(env[1], "CONFIG_PATH=%s", argv[optind]);
    if (output_file != NULL)
        sprintf(env[2], "OUTPUT_FILE=%s", output_file);
    else
        sprintf(env[2], "OUTPUT_FILE=stderr");
    env[3] = NULL;

/*
    printf("== env ==\n");
    for (int i = 0; env[i] != NULL; i++) {
        printf("%s\n", env[i]);
    }
    printf("== argv ==\n");
    for (int i = 0; arguments[i] != NULL; i++) {
        printf("arguments[%d] = %s\n", i, arguments[i]);
    }
*/

    pid_t pid;
    pid = fork();
    if (pid == 0) {
        // Child process
        execve(arguments[0], arguments, env);
    } else {
        // Parent process
        wait(NULL);
    }
    
    return 0;
   
}