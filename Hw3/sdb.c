#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/user.h>
#include <string.h>
#include <elf.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <capstone/capstone.h>
#include <fcntl.h>
#define ull unsigned long long
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define BREAKPOINT_MAX 10

pid_t child;
void *ptr;
Elf64_Addr e_ptr;

off_t text_offset = 0;      
size_t text_size = 0;
ull base_addr = 0;

unsigned char buf[0x10000];
csh handel = 0;
int exit_flag = 0;
char cmd[50];
int breakpoint_count = 0;
int in_syscall = 0;
int on_breakpoint = 0;

struct breakpoint{
    ull addr;
    unsigned char data;
};

struct breakpoint breakpoints[10];

int disasm(int offset);
ull get_current_rip();
int step_instruction();
int continue_execution();
int info_registers();
int set_breakpoint(ull addr);
int info_breakpoints();
int delete_breakpoint(int idx);
int patch_memory(ull addr, ull value, int len);
int system_call();
int hit_break_point();
int find_index(ull addr);

int main(int argc, char* argv[]){
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    int new_argc = 3;
    char *new_argv[new_argc];
    for (int i = 0; i < argc; i++)
        new_argv[i] = argv[i];
    if (argc < 2){
LOAD_AGAIN:
        printf("(sdb) ");
        char c[50];
        fgets(c, 50, stdin);
        if (strncmp(c, "load", 4) == 0){
            char *token = strtok(c, " ");
            token = strtok(NULL, " ");
            new_argv[1] = token;
            new_argv[1][strlen(new_argv[1]) - 1] = '\0';
        }else{
            printf("** please load a program first\n");
            goto LOAD_AGAIN;
        }
    }
    new_argv[2] = NULL;

    if((child = fork()) < 0)
        perror("fork()");
    else if (child == 0){
        if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0)
            perror("ptrace");
        execvp(new_argv[1], new_argv+1);
        perror("execvp");
        exit(0);
    }else{
        waitpid(child, 0, 0);
        if(ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_EXITKILL) < 0)
            perror("ptrace_setoptions");
        int fd = open(new_argv[1], O_RDONLY);
        if(fd < 0)
            perror("open");
        ptr = mmap(NULL, 0x10000, PROT_READ, MAP_PRIVATE, fd, 0);
        if(ptr == MAP_FAILED)
            perror("mmap");

        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)ptr;
        e_ptr = ehdr->e_entry;

        Elf64_Shdr *shdr = (Elf64_Shdr *)(ptr + ehdr->e_shoff);
        char *strtab = (char *)(ptr + shdr[ehdr->e_shstrndx].sh_offset);

        for(int i = 0; i < ehdr->e_shnum; i++){
            if(shdr[i].sh_type == SHT_PROGBITS && !strcmp(".text", strtab + shdr[i].sh_name)){
                text_offset = shdr[i].sh_offset;
                text_size = shdr[i].sh_size;
                base_addr = shdr[i].sh_addr;
                break;
            }
        }

        if (text_offset == 0 || text_size == 0 || base_addr == 0)
            exit(0);

        fprintf(stdout, "** program \'%s\' loaded. entry point 0x%lx.\n", new_argv[1], e_ptr);

        memset(buf, 0, 0x10000);
        memcpy(buf, ptr + text_offset, text_size);

        if(cs_open(CS_ARCH_X86, CS_MODE_64, &handel) != CS_ERR_OK)
            perror("cs_open");

        cs_insn* insn;
        int count = cs_disasm(handel, &buf[e_ptr - base_addr], text_size, e_ptr, 0, &insn);

        cs_free(insn, count);

        disasm(0);

        ull rip = get_current_rip();

        while(!exit_flag){
            printf("(sdb) ");
            memset(cmd, 0, 50);
            fgets(cmd, 50, stdin);
            if(cmd[strlen(cmd) - 1] == '\n')
                cmd[strlen(cmd) - 1] = '\0';
            if(!strcmp(cmd, "exit"))
                exit_flag = 1;
            else if(!strcmp(cmd, "si")){
                if(on_breakpoint)
                    on_breakpoint = hit_break_point();
                else
                    on_breakpoint = step_instruction();
                if(exit_flag)
                    continue;
                disasm(0);
            }else if(!strcmp(cmd, "cont")){
                if(on_breakpoint){
                    on_breakpoint = hit_break_point();
                    if(!on_breakpoint)
                        on_breakpoint = continue_execution();
                }else
                    on_breakpoint = continue_execution();
                if(exit_flag)
                    continue;
                disasm(0);
            }else if(!strcmp(cmd, "info reg")){
                info_registers();
            }else if(!strncmp(cmd, "break", 5)){
                char *token = strtok(cmd, " ");
                token = strtok(NULL, " ");
                ull addr = strtoul(token, NULL, 16);
                set_breakpoint(addr);
            }else if(!strcmp(cmd, "info break")){
                info_breakpoints();
            }else if(!strncmp(cmd, "delete", 6)){
                char *token = strtok(cmd, " ");
                token = strtok(NULL, " ");
                int idx = atoi(token);
                delete_breakpoint(idx);
            }else if(!strncmp(cmd, "patch", 5)){
                ull addr, value;
                int len;
                char *token = strtok(cmd, " ");
                token = strtok(NULL, " ");
                addr = strtoul(token, NULL, 16);
                token = strtok(NULL, " ");
                value = strtoul(token, NULL, 16);
                token = strtok(NULL, " ");
                len = atoi(token);
                patch_memory(addr, value, len);
            }else if(!strcmp(cmd, "syscall")){
                if(on_breakpoint){
                    on_breakpoint = hit_break_point();
                    if(!on_breakpoint)
                        on_breakpoint = system_call();
                }else
                    on_breakpoint = system_call();
                if(exit_flag)
                    continue;
                disasm(-4);
            }else{
                printf("** unknown command\n");
            }
        }
        munmap(ptr, 0x10000);
        close(fd);
    }
    return 0;
}

int disasm(int offset){
    ull rip = get_current_rip();
    cs_insn* insn;
    int count = cs_disasm(handel, &buf[e_ptr - base_addr], text_size, e_ptr, 0, &insn);
    int idx;
    for(idx = 0; (ull)insn[idx].address < rip + offset; idx++);

    for(int i = idx; i < MIN(idx + 5, count); i++){
        printf("\t%lx: ", insn[i].address);
        for(int j = 0; j < insn[i].size; j++)
            printf("%02x ", insn[i].bytes[j]);
        for(int j = insn[i].size; j < 10; j++)
            printf("   ");
        printf("\t%s\t%s\n", insn[i].mnemonic, insn[i].op_str);
    }
    if (idx + 5 >= count)
        printf("** the address is out of the range of the text section.\n");
    cs_free(insn, count);
}

ull get_current_rip(){
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, child, 0, &regs);
    return regs.rip;
}

int step_instruction(){
    if(ptrace(PTRACE_SINGLESTEP, child, 0, 0) < 0)
        perror("ptrace_singlestep");
    int status;
    if(waitpid(child, &status, 0) < 0)
        perror("waitpid");
    if(WIFEXITED(status)){
        printf("** the target program terminated.\n");
        exit_flag = 1;
        return 0;
    }
    ull rip = get_current_rip();
    ull ins = ptrace(PTRACE_PEEKTEXT, child, rip, 0);
    if((ins & 0xff) == 0xcc){
        printf("** hit a breakpoint at 0x%lx\n", rip);
        return 1;
    }

    return 0;
}


int continue_execution(){
    if(ptrace(PTRACE_CONT, child, 0, 0) < 0)
        perror("ptrace_cont");
    int status;
    if(waitpid(child, &status, 0) < 0)
        perror("waitpid");
    if(WIFEXITED(status)){
        printf("** the target program terminated.\n");
        exit_flag = 1;
        return 0;
    }
    
    struct user_regs_struct tmp;
    if(ptrace(PTRACE_GETREGS, child, 0, &tmp) < 0)
        perror("ptrace_getregs");
    tmp.rip -= 1;
    if(ptrace(PTRACE_SETREGS, child, 0, &tmp) < 0)
        perror("ptrace_setregs");
    
    ull rip = get_current_rip();
    printf("** hit a breakpoint at 0x%lx\n", rip);
    return 1;
}

int info_registers(){
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, child, 0, &regs);
    printf("rax 0x%0.16llx\t", regs.rax);
    printf("rbx 0x%0.16llx\t", regs.rbx);
    printf("rcx 0x%0.16llx\n", regs.rcx);
    printf("rdx 0x%0.16llx\t", regs.rdx);
    printf("rsi 0x%0.16llx\t", regs.rsi);
    printf("rdi 0x%0.16llx\n", regs.rdi);
    printf("rbp 0x%0.16llx\t", regs.rbp);
    printf("rsp 0x%0.16llx\t", regs.rsp);
    printf("r8  0x%0.16llx\n", regs.r8);
    printf("r9  0x%0.16llx\t", regs.r9);
    printf("r10 0x%0.16llx\t", regs.r10);
    printf("r11 0x%0.16llx\n", regs.r11);
    printf("r12 0x%0.16llx\t", regs.r12);
    printf("r13 0x%0.16llx\t", regs.r13);
    printf("r14 0x%0.16llx\n", regs.r14);
    printf("r15 0x%0.16llx\t", regs.r15);
    printf("rip 0x%0.16llx\t", regs.rip);
    printf("eflags 0x%0.16llx\n", regs.eflags);
}

int set_breakpoint(ull addr){
    ull ori = ptrace(PTRACE_PEEKTEXT, child, addr, 0);
    ull new = (ori & 0xffffffffffffff00) | 0xcc;
    if(ptrace(PTRACE_POKETEXT, child, addr, new) < 0)
        perror("ptrace_poketext");
    breakpoints[breakpoint_count].addr = addr;
    breakpoints[breakpoint_count].data = ori & 0xff;
    breakpoint_count++;

    printf("** set a breakpoint at 0x%lx\n", addr);
    return 0;
}

int info_breakpoints(){
    int breakpoint_count = 0;
    for(int i = 0; i < BREAKPOINT_MAX; i++){
        if(breakpoints[i].addr != 0)
            breakpoint_count++;
    }
    if(breakpoint_count == 0){
        printf("** no breakpoints\n");
        return 0;
    }else{
        printf("Num\tAddress\n");
        for(int i = 0; i < BREAKPOINT_MAX; i++){
            if(breakpoints[i].addr != 0)
                printf("%d\t0x%lx\n", i, breakpoints[i].addr);
        }
    }
    return 0;
}

int delete_breakpoint(int idx){
    if(breakpoints[idx].addr == 0){
        printf("** breakpoint %d does not exist.\n", idx); 
        return 0;
    }

    ull rip = breakpoints[idx].addr;
    ull tmp = ptrace(PTRACE_PEEKTEXT, child, rip, 0);
    ull new = (tmp & 0xffffffffffffff00) | breakpoints[idx].data;

    if(ptrace(PTRACE_POKETEXT, child, rip, new) < 0)
        perror("ptrace_poketext");

    breakpoints[idx].addr = 0;
    breakpoints[idx].data = 0;

    printf("** delete breakpoint %d.\n", idx);
    return 0;
}

int patch_memory(ull addr, ull value, int len){
    for(int i = 0; i < BREAKPOINT_MAX; i++)
        if(breakpoints[i].addr == addr)
            breakpoints[i].data = value;
    ull ori = ptrace(PTRACE_PEEKTEXT, child, addr, 0);
    ull new = ori;
    if (len == 1)
        new = (ori & 0xffffffffffffff00) | value;
    else if (len == 2)
        new = (ori & 0xffffffffffff0000) | value;
    else if (len == 4)
        new = (ori & 0xffffffff00000000) | value;
    else if (len == 8)
        new = value;
    else{
        printf("** invalid length\n");
        return 0;
    }
    for (int i = 0; i < len; i++){
        buf[addr - base_addr + i] = value & 0xff;
        value >>= 8;
    }
    if (ptrace(PTRACE_POKETEXT, child, addr, new) < 0)
        perror("ptrace_poketext");
    printf("** patch memory at address 0x%lx.\n", addr);

    return 0;
}

int system_call(){
    if(ptrace(PTRACE_SYSCALL, child, 0, 0) < 0)
        perror("ptrace_syscall");
    int status;
    if(waitpid(child, &status, 0) < 0)
        perror("waitpid");
    if(WIFEXITED(status)){
        printf("** the target program terminated.\n");
        exit_flag = 1;
        return 0;
    }
    struct user_regs_struct regs;
    if(ptrace(PTRACE_GETREGS, child, 0, &regs) < 0)
        perror("ptrace_getregs");
    
    ull rip = get_current_rip() - 1;
    int idx = find_index(rip);
    if(idx != -1){
        printf("** hit a breakpoint at 0x%lx\n", rip);
        struct user_regs_struct regs;
        if(ptrace(PTRACE_GETREGS, child, 0, &regs) < 0)
            perror("ptrace_getregs");
        regs.rip -= 1;
        if(ptrace(PTRACE_SETREGS, child, 0, &regs) < 0)
            perror("ptrace_setregs");
        return 1;
    }
    if(in_syscall){
        printf("** leave syscall(%lld) = %d at 0x%lx.\n", regs.orig_rax, regs.rax, regs.rip - 2);
        in_syscall = 0;
    }else{
        printf("** enter a syscall(%lld) at 0x%lx.\n", regs.orig_rax, regs.rip - 2);
        in_syscall = 1;
    }
    return 0;
}

int hit_break_point(){
    ull rip = get_current_rip();
    int idx = find_index(rip);
    if(idx == -1)
        return 0;
    ull tmp = ptrace(PTRACE_PEEKTEXT, child, rip, 0);
    ull new = (tmp & 0xffffffffffffff00) | breakpoints[idx].data;

    if(ptrace(PTRACE_POKETEXT, child, rip, new) < 0)
        perror("ptrace_poketext");
    if(ptrace(PTRACE_SINGLESTEP, child, 0, 0) < 0)
        perror("ptrace_singlestep");
    int status;
    if(waitpid(child, &status, 0) < 0)
        perror("waitpid");
    if(ptrace(PTRACE_POKETEXT, child, rip, tmp) < 0)
        perror("ptrace_poketext");
    if(WIFEXITED(status)){
        printf("** the target program terminated.");
        exit_flag = 1;
        return 0;
    }
    
    rip = get_current_rip();
    ull ins = ptrace(PTRACE_PEEKTEXT, child, rip, 0);
    if((ins & 0xcc) == 0xcc){
        printf("** hit a breakpoint at 0x%lx\n", rip);
        return 1;
    }
    return 0;
}

int find_index(ull addr){
    for(int i = 0; i < BREAKPOINT_MAX; i++){
        if(breakpoints[i].addr == addr)
            return i;
    }
    return -1;
}