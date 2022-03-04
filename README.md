# HighPerformanceCProgram
Uses execv() to split up task of matrix multiplication

[assignment 5-1.pdf](https://github.com/JackWalton1/HighPerformanceCProgram/files/8188999/assignment.5-1.pdf)

compile with:

gcc mpi.c -o mpi -lrt (-lrt because we are creating named shared memory in matrix.c)

gcc matrix.c -o matrix -lrt


Run with:

./mpi matrix {1-100}

i.e.:

./mpi matrix 100
