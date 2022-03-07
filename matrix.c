#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#define MATRIX_DIMENSION_XY 10

//THIS IS PROGRAM 1

int synch(int par_id,int par_count, int *ready, int next, int last)
{
    ready[par_id] = next;
    for(int i = 0; i<par_count; i++){
        while(ready[i]==last){} //wait
    }

    return next+1; // returns the value of next, for next time synch is called
}

void set_matrix_elem(float *M,int x,int y,float f)
{
    M[y + x*MATRIX_DIMENSION_XY] = f;
}

void quadratic_matrix_print(float *C)
{
        printf("\n");
    for(int a = 0;a<MATRIX_DIMENSION_XY;a++)
        {
        printf("\n");
        for(int b = 0;b<MATRIX_DIMENSION_XY;b++)
            printf("%2.2f,",C[b + a* MATRIX_DIMENSION_XY]);
        }
    printf("\n");
}

void quadratic_matrix_multiplication(float *A,float *B,float *C)
{
	//nullify the result matrix first
    for(int a = 0;a<MATRIX_DIMENSION_XY;a++)
        for(int b = 0;b<MATRIX_DIMENSION_XY;b++)
            C[b + a*MATRIX_DIMENSION_XY] = 0.0;
    //multiply
    for(int a = 0;a<MATRIX_DIMENSION_XY;a++) // over all cols a
        for(int b = 0;b<MATRIX_DIMENSION_XY;b++) // over all rows b
            for(int c = 0;c<MATRIX_DIMENSION_XY;c++) // over all rows/cols left
                {
                    C[b + a*MATRIX_DIMENSION_XY] += A[b + c*MATRIX_DIMENSION_XY] * B[c + a*MATRIX_DIMENSION_XY]; 
                }
}

int split_tasks(int par_id, int par_count, int* index_zone){
    int total_indices = MATRIX_DIMENSION_XY*MATRIX_DIMENSION_XY;
    if(par_count>total_indices){
        return -1;
    }
    int floor = (int)(total_indices/par_count); // floor = #indices from each start -> end
    int ceiling = floor + 1;
    int ceiling_occurance = (total_indices-(par_count*floor));
    int floor_occurance = par_count - ceiling_occurance;
    int start;
    int end;
    int pnumber = par_id+1;

    if((pnumber)>floor_occurance){
        start = floor_occurance*floor+(par_id-floor_occurance)*ceiling;
        end = floor_occurance*floor+(pnumber-floor_occurance)*ceiling-1;
    }else{//if (par_id+1)<floor_occurance)
        start = par_id*floor;
        end = (pnumber)*floor -1;
    }
    index_zone[0] = start;
    index_zone[1] = end;
    return 0;
}

float matrix_multiplication_1row1col(float *A,float *B, int row, int col){
    float resultant = 0;
    int i; //iteration
    for(i=0; i<MATRIX_DIMENSION_XY; i++){
        resultant += A[row*MATRIX_DIMENSION_XY+i]*B[col+MATRIX_DIMENSION_XY*i];
    }
    return resultant;
}

void quadratic_matrix_multiplication_parallel(float *A,float *B,float *C, int start, int end){
    int row_start = start/MATRIX_DIMENSION_XY;
    int col_start = start%MATRIX_DIMENSION_XY;
    int row_end = end/MATRIX_DIMENSION_XY;
    int col_end = end%MATRIX_DIMENSION_XY;

    do{
        float result = matrix_multiplication_1row1col(A,B, row_start, col_start);
        set_matrix_elem(C, row_start, col_start, result);
        if(col_start<MATRIX_DIMENSION_XY){
            col_start++;
        }
        else if(col_start>=MATRIX_DIMENSION_XY){
            row_start++;
            col_start = 0;
        }

    }while(row_start<=row_end || col_start<=col_end);
    return;
}

int quadratic_matrix_compare(float *A,float *B)
{
    for(int a = 0;a<MATRIX_DIMENSION_XY;a++)
        for(int b = 0;b<MATRIX_DIMENSION_XY;b++)
           if(A[b +a * MATRIX_DIMENSION_XY]!=B[b +a * MATRIX_DIMENSION_XY]) 
            return 0;
    
    return 1;
}

int main(int argc, char *argv[]){
    struct timeval s, e;
    int par_id = 0; // the parallel ID of this process
    int par_count = 1; // the amount of processes

    int total_indices = MATRIX_DIMENSION_XY*MATRIX_DIMENSION_XY; // = 100 in this case, is robust though
    int next = 1; // for synch
    int last = 0; // for synch

    int index_zone[2]; // index_zone[0] = start | index_zone[1] = end

    float *A,*B,*C; //matrices A,B and C
    int *ready; // list of ready ints for synch

    int fd[4]; // file descriptors for named shared memory

    if(argc!=2){printf("no shared\n");}
    else{
        par_id= atoi(argv[0]); //mpi passes this var
        par_count= atoi(argv[1]); // and this var
    }

    //bounds check (1-100 processes allowed)
    if(par_count<=0 || par_count > total_indices){
        if(par_id==0 && par_count<=0){
            fprintf(stderr, "Asking for not enough processes\n");
        }
        if(par_id==0 && par_count > total_indices){
            fprintf(stderr, "Asking for too many processes\n");
        }
        return -1;
    }

    if(par_count==1)fprintf(stderr, "only one process\n");

    if(par_id==0){
        //TODO: init the shared memory for A,B,C, ready. shm_open with C_CREAT here! then ftruncate! then mmap

        fd[0] = shm_open("matrixA", O_RDWR|O_CREAT, 00777);
        fd[1] = shm_open("matrixB", O_RDWR|O_CREAT, 00777);
        fd[2] = shm_open("matrixC", O_RDWR|O_CREAT, 00777);
        fd[3] = shm_open("synchobject", O_RDWR|O_CREAT, 00777);
        ftruncate(fd[0], sizeof(float)*100);
        ftruncate(fd[1], sizeof(float)*100);
        ftruncate(fd[2], sizeof(float)*100);
        ftruncate(fd[3], sizeof(int)*par_count);
        A = (float*)mmap(NULL,  sizeof(float)*100, PROT_READ|PROT_WRITE, MAP_SHARED, fd[0], 0);
        B = (float*)mmap(NULL,  sizeof(float)*100, PROT_READ|PROT_WRITE, MAP_SHARED, fd[1], 0);
        C = (float*)mmap(NULL,  sizeof(float)*100, PROT_READ|PROT_WRITE, MAP_SHARED, fd[2], 0);
        ready = (int*)mmap(NULL,  sizeof(int)*par_count, PROT_READ|PROT_WRITE, MAP_SHARED, fd[3], 0);
        
        for(int i = 0; i<par_count; i++){
            ready[i] = 0;
        }
    
    }else{
    	//TODO: init the shared memory for A,B,C, ready. shm_open withOUT C_CREAT here! NO ftruncate! but yes to mmap
        sleep(1); //needed for initalizing synch
        fd[0] = shm_open("matrixA", O_RDWR, 00777);
        fd[1] = shm_open("matrixB", O_RDWR, 00777);
        fd[2] = shm_open("matrixC", O_RDWR, 00777);
        fd[3] = shm_open("synchobject", O_RDWR, 00777);
        A = (float*)mmap(NULL,  sizeof(float)*100, PROT_READ|PROT_WRITE, MAP_SHARED, fd[0], 0);
        B = (float*)mmap(NULL,  sizeof(float)*100, PROT_READ|PROT_WRITE, MAP_SHARED, fd[1], 0);
        C = (float*)mmap(NULL,  sizeof(float)*100, PROT_READ|PROT_WRITE, MAP_SHARED, fd[2], 0);
        ready = (int*)mmap(NULL,  sizeof(int)*par_count, PROT_READ|PROT_WRITE, MAP_SHARED, fd[3], 0);
    }
    
    next = synch(par_id, par_count, ready, next, last);
    last = next - 1;

    if(par_id==0){
    	//TODO: initialize the matrices A and B
        int number = 1;
        for(int i = 0; i<total_indices; i++){
            A[i] = (float)1*number/2;
            B[i] = (float)1*number/2;
            C[i] = 0;
            if(i%10 == 0){
                number++;
            }
        }
        //quadratic_matrix_print(A);
        //quadratic_matrix_print(B);
        //quadratic_matrix_print(C);
    }

    next = synch(par_id, par_count, ready, next, last);
    last = next - 1;

    if(par_id==0)gettimeofday(&s, NULL); //start timer
    // Each process gets a start and end index from 0-99 that corresponds to the Resultant matrix (C)
    // based on how many processes are running
    split_tasks(par_id, par_count, index_zone);
    
    next = synch(par_id, par_count, ready, next, last);
    last = next - 1;

    int start = index_zone[0];   // Each process gets a start index
    int end = index_zone[1];    // Each process fets an end index

    quadratic_matrix_multiplication_parallel(A,B,C,start,end); //Each process does it's work from start to end (with help of )

    next = synch(par_id, par_count, ready, next, last);
    last = next - 1;

    if(par_id==0){
        gettimeofday(&e, NULL); // end timer
        //quadratic_matrix_print(C);
        float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];
        quadratic_matrix_multiplication(A, B, M);
        //quadratic_matrix_print(M);
        if(quadratic_matrix_compare(C, M)) // I moved compare so that only process 0 does compare
        	fprintf(stderr, "full points!\n");
        else
        	fprintf(stderr, "buuug!\n");

        long seconds = (e.tv_sec - s.tv_sec);
        long micros = ((seconds * 1000000) + e.tv_usec) - (s.tv_usec);
        printf("Time taken for actual multiplication (SS.MS):\n\t %ld.%ld\n", seconds, micros);
    }
    //fprintf(stderr, "Process %d is finished!\n", par_id);

    next = synch(par_id, par_count, ready, next, last);
    last = next - 1;

    //fprintf(stderr, "Bye now!\n");
    close(fd[0]);
    close(fd[1]);
    close(fd[2]);
    close(fd[3]);
    shm_unlink("matrixA");
    shm_unlink("matrixB");
    shm_unlink("matrixC");
    shm_unlink("synchobject");//ready list

    return 0;
}
