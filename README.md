# Introduction to Parallel Computing

A university project for MPI

## Requirements

- [MPI](https://www.open-mpi.org/)
- [WSL2 for Windows](https://docs.microsoft.com/en-us/windows/wsl/install) or any Linux distribution 
- [GCC]()

Ypu can install GCC and MPI using
```
sudo apt install gcc
sudo apt install mpi
```

Compile MPI code
```
mpicc file.c -o file
```

Execute MPI code 
```
mpirun -np X ./file
```

where X is the number of processes you want.
