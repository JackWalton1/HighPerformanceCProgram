#include <sys/mman.h> 
#include <sys/stat.h>  
#include <fcntl.h> 
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
 
//THIS IS PROGRAM 2

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr, "Not enough arguments\n");
        return -1;
    }
    int pid;
    int par_id = 0;
    char* program_name = argv[1];
    char* par_count = argv[2];
    char par_id_chr[10];
    do{
        //arguments = {par_id, par_count};
        pid = fork();
        if(pid>0){
            par_id++;
        }
    }while(par_id<atoi(par_count) && pid>0);
    
    if(!pid){
        sprintf(par_id_chr, "%d", par_id);
        execv(program_name, (char*[]){par_id_chr, par_count});
        return 0;
    }

    return 0;

}
