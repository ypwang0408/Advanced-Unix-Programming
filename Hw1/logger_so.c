#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <elf.h>
#include <dlfcn.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
FILE *(*original_fopen)(const char *, ...) = NULL;
size_t (*original_fread)(void *, ssize_t, ssize_t, FILE *) = NULL;
size_t (*original_fwrite)(const void *, ssize_t, ssize_t, FILE *) = NULL;
int (*original_connect)(int, const struct sockaddr *, socklen_t) = NULL;
int (*original_getaddrinfo)(const char *, const char *, const struct addrinfo *, struct addrinfo **) = NULL;
int (*original_system)(const char *) = NULL;

char filename[100];


char *ltrim(char *s){
    while(isspace(*s)) s++;
    return s;
}

char *rtrim(char *s){
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

char *trim(char *s){
    return rtrim(ltrim(s)); 
}

void replace_newline(const char *str, char *word) {
    char idx = 0;
    for(int i = 0; i < strlen(str); i++) {
        if(str[i] == '\n') {
            word[idx] = '\\';
            word[idx+1] = 'n';
            idx += 2;
        }else{
            word[idx] = str[i];
            idx++;
        }
    }
    word[idx] = '\0';

}

void get_originals() {
    original_fopen = dlsym(RTLD_NEXT, "fopen");
    original_fread = dlsym(RTLD_NEXT, "fread");
    original_fwrite = dlsym(RTLD_NEXT, "fwrite");
    original_connect = dlsym(RTLD_NEXT, "connect");
    original_getaddrinfo = dlsym(RTLD_NEXT, "getaddrinfo");
    original_system = dlsym(RTLD_NEXT, "system");
}

int check_bl_open(const char *pathname, const char *config_path) {
    FILE* config_file;
    if((config_file = original_fopen(config_path, "r")) == NULL) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int place = 0;
    char *BEGIN = "BEGIN open-blacklist";
    char *END = "END open-blacklist";
    while((read = getline(&line, &len, config_file)) != -1) {
        if(strncmp(line, BEGIN, strlen(BEGIN)) == 0) {
            place = 1;
        }else if (strncmp(line, END, strlen(END)) == 0) {
            fclose(config_file);
            return 1;
        }
        if(place == 1) {
            trim(line);
            line[strlen(line)-1] = '\0';
            int star = 0;
            if(line[strlen(line)-2] == '*'){
                line[strlen(line)-2] = '\0';
                star = 1;
            }
            if(star) {
                if(strncmp(pathname, line, strlen(line)) == 0) {
                    fclose(config_file);
                    return 0;
                }
            }else{
                if(strcmp(pathname, line) == 0) {
                    fclose(config_file);
                    return 0;
                }
            }
        }
    }
    fclose(config_file);
    return 1;
}

FILE *fopen(const char *pathname, const char *mode) {
    get_originals();
    char *config_path = getenv("CONFIG_PATH");
    char *output_file = getenv("OUTPUT_FILE");
    FILE *out_fd = stderr;
    FILE *fd = NULL;

    // Get the real path of the file
    struct stat st;
    stat(pathname, &st);
    while(S_ISLNK(st.st_mode)) {
        printf("Is a link\n");
        char *linkname = malloc(st.st_size + 1);
        readlink(pathname, linkname, st.st_size + 1);
        linkname[st.st_size] = '\0';
        pathname = linkname;
        lstat(pathname, &st);
    }

    printf("pathname: %s\n", pathname);
    

    // Trim the filename from the path
    char *sec_pch = strrchr(pathname, '.');
    char *fir_pch = strrchr(pathname, '/');
    if(fir_pch == NULL)
        fir_pch = (char *)pathname;
    else
        fir_pch++;
    if(sec_pch == NULL)
        strcpy(filename, fir_pch);
    else{
        strncpy(filename, fir_pch, sec_pch - fir_pch);
        filename[sec_pch - fir_pch] = '\0';
    }

    if(strcmp(output_file, "stderr") != 0)
        out_fd = original_fopen(output_file, "a");
    
    if(!check_bl_open(pathname, config_path)) {
        fprintf(out_fd, "[logger] fopen(\"%s\", \"%s\") = 0x0\n", pathname, mode);
        errno = EACCES;
    }else{
        fd = original_fopen(pathname, mode);
        fprintf(out_fd, "[logger] fopen(\"%s\", \"%s\") = 0x%lx\n", pathname, mode, fd);
    }

    if(strcmp(output_file, "stderr") != 0)
        fclose(out_fd);
    return fd;
}

int check_bl_read(const char *ptr, const int nmemb, const char *config_path){

    char *target = malloc(nmemb + 2);
    strncpy(target, ptr, nmemb + 1);
    target[nmemb + 1] = '\0';

    FILE* config_file;
    if((config_file = original_fopen(config_path, "r")) == NULL) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int place = 0;
    char *BEGIN = "BEGIN read-blacklist";
    char *END = "END read-blacklist";
    while((read = getline(&line, &len, config_file)) != -1) {
        
        if(strncmp(line, BEGIN, strlen(BEGIN)) == 0) {
            place = 1;
        }else if (strncmp(line, END, strlen(END)) == 0) {
            fclose(config_file);
            return 1;
        }
        if(place == 1) {
            trim(line);
            line[strlen(line)-1] = '\0';
            if(strncmp(target, line, strlen(line)) == 0) {
                fclose(config_file);
                return 0;
            }
        }
    }
    fclose(config_file);
    return 1;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    get_originals();
    char *config_path = getenv("CONFIG_PATH");    
    char *output_file = getenv("OUTPUT_FILE");
    FILE *out_fd = stderr;
    size_t fd = 0;
    
    if(strcmp(output_file, "stderr") != 0)
        out_fd = original_fopen(output_file, "a");
        
    if(!check_bl_read(stream->_IO_write_ptr, nmemb, config_path)) {
        fprintf(out_fd, "[logger] fread(0x%lx, %d, %d, 0x%lx) = 0\n", ptr, size, nmemb, stream);
        errno = EACCES;
    }else{
        fd = original_fread(ptr, size, nmemb, stream);
        fprintf(out_fd, "[logger] fread(0x%lx, %d, %d, 0x%lx) = %d\n", ptr, size, nmemb, stream, fd);
        
        char log_filename[100];
        sprintf(log_filename, "./%d-%s-read.log", getpid(), filename);
        FILE *log_fd = original_fopen(log_filename, "a");
        fprintf(log_fd, "%s", ptr);
        fclose(log_fd);
    }
    if(strcmp(output_file, "stderr") != 0)
        fclose(out_fd);
    return fd;
}

int check_bl_write(const char *words, const char *config_path){
    FILE* config_file;
    if((config_file = original_fopen(config_path, "r")) == NULL) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int place = 0;
    char *BEGIN = "BEGIN write-blacklist";
    char *END = "END write-blacklist";
    while((read = getline(&line, &len, config_file)) != -1) {
        if(strncmp(line, BEGIN, strlen(BEGIN)) == 0) {
            place = 1;
        }else if (strncmp(line, END, strlen(END)) == 0) {
            fclose(config_file);
            return 1;
        }
        if(place == 1) {
            trim(line);
            line[strlen(line)-1] = '\0';
            int star = 0;
            if(line[strlen(line)-2] == '*'){
                line[strlen(line)-2] = '\0';
                star = 1;
            }
            if (star) {
                if(strncmp(words, line, strlen(line)) == 0) {
                    fclose(config_file);
                    return 0;
                }
            }else{
                if(strcmp(words, line) == 0) {
                    fclose(config_file);
                    return 0;
                }
            }
        }
    }
    fclose(config_file);
    return 1;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    get_originals();
    char *config_path = getenv("CONFIG_PATH");
    char *output_file = getenv("OUTPUT_FILE");
    FILE *out_fd = stderr;
    size_t fd = 0;
    char word[100];
    replace_newline(ptr, word);

    if(strcmp(output_file, "stderr") != 0)
        out_fd = original_fopen(output_file, "a");

    if(!check_bl_write(ptr, config_path)) {
        fprintf(out_fd, "[logger] fwrite(\"%s\", %d, %d, 0x%lx) = 0\n", word, size, nmemb, stream);
        errno = EACCES;
    }else{
        fd = original_fwrite(ptr, size, nmemb, stream);
        fprintf(out_fd, "[logger] fwrite(\"%s\", %d, %d, 0x%lx) = %d\n", word, size, nmemb, stream, fd);

        char log_filename[100];
        sprintf(log_filename, "./%d-%s-write.log", getpid(), filename);
        FILE *log_fd = original_fopen(log_filename, "a");
        fprintf(log_fd, "%s", ptr);
        fclose(log_fd);
    }
    if(strcmp(output_file, "stderr") != 0)
        fclose(out_fd);
    return fd;
}

int check_bl_connect(const struct sockaddr *addr, const char *config_path){
    char *target = inet_ntoa(((struct sockaddr_in *)addr)->sin_addr);
    FILE* config_file;
    if((config_file = original_fopen(config_path, "r")) == NULL) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int place = 0;
    char *BEGIN = "BEGIN connect-blacklist";
    char *END = "END connect-blacklist";
    while((read = getline(&line, &len, config_file)) != -1) {
        if(strncmp(line, BEGIN, strlen(BEGIN)) == 0) {
            place = 1;
        }else if (strncmp(line, END, strlen(END)) == 0) {
            fclose(config_file);
            return 1;
        }
        if(place == 1) {
            trim(line);
            line[strlen(line)-1] = '\0';
            if(strncmp(target, line, strlen(line)) == 0) {
                fclose(config_file);
                return 0;
            }
        }
    }
    fclose(config_file);
    return 1;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    get_originals();
    char *config_path = getenv("CONFIG_PATH");
    char *output_file = getenv("OUTPUT_FILE");
    FILE *out_fd = stderr;
    int fd = 0;
    

    if(strcmp(output_file, "stderr") != 0)
        out_fd = original_fopen(output_file, "a");
    
    if(!check_bl_connect(addr, config_path)) {
        fprintf(out_fd, "[logger] connect(%d, \"%s\", %d) = -1\n", sockfd, inet_ntoa(((struct sockaddr_in *)addr)->sin_addr), addrlen);
        errno = ECONNREFUSED;
        fd = -1;
    }else{
        fd = original_connect(sockfd, addr, addrlen);
        fprintf(out_fd, "[logger] connect(%d, \"%s\", %d) = %d\n", sockfd, inet_ntoa(((struct sockaddr_in *)addr)->sin_addr), addrlen, fd);
    }
    if(strcmp(output_file, "stderr") != 0)
        fclose(out_fd);
    return fd;
}

int check_bl_getaddrinfo(const char *hostname, const char *config_path){
    FILE* config_file;
    if((config_file = original_fopen(config_path, "r")) == NULL) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int place = 0;
    char *BEGIN = "BEGIN getaddrinfo-blacklist";
    char *END = "END getaddrinfo-blacklist";
    while((read = getline(&line, &len, config_file)) != -1) {
        if(strncmp(line, BEGIN, strlen(BEGIN)) == 0) {
            place = 1;
        }else if (strncmp(line, END, strlen(END)) == 0) {
            fclose(config_file);
            return 1;
        }
        if(place == 1) {
            trim(line);
            line[strlen(line)-1] = '\0';
            if(strncmp(hostname, line, strlen(line)) == 0) {
                fclose(config_file);
                return 0;
            }
        }
    }
    fclose(config_file);
    return 1;
}

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    get_originals();
    char *config_path = getenv("CONFIG_PATH");
    char *output_file = getenv("OUTPUT_FILE");
    FILE *out_fd = stderr;
    int fd = 0;


    if(strcmp(output_file, "stderr") != 0)
        out_fd = original_fopen(output_file, "a");
    
    if(!check_bl_getaddrinfo(node, config_path)) {
        fprintf(out_fd, "[logger] getaddrinfo(\"%s\", %p, 0x%lx, 0x%lx) = -1\n", node, service, hints, res);
        return EAI_NONAME;
    }else{
        fd = original_getaddrinfo(node, service, hints, res);
        fprintf(out_fd, "[logger] getaddrinfo(\"%s\", %p, 0x%lx, 0x%lx) = %d\n", node, service, hints, res, fd);
    }
    if(strcmp(output_file, "stderr") != 0)
        fclose(out_fd);
    return fd;
}

int system(const char *command) {
    get_originals();
    char *output_file = getenv("OUTPUT_FILE");
    FILE *out_fd = stderr;
    int ret = original_system(command);


    if(strcmp(output_file, "stderr") != 0)
        out_fd = original_fopen(output_file, "a");

    fprintf(out_fd, "[logger] system(\"%s\") = %d\n", command, ret);
    if(strcmp(output_file, "stderr") != 0)
        fclose(out_fd);
    return ret;
}




    