#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

// Works only if numOfProc multiple of n

int main(int argc, char **argv)
{

    // Initialize MPI
    int rankOfProc, numOfProc;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankOfProc);
    MPI_Comm_size(MPI_COMM_WORLD, &numOfProc);

    // Initialize values
    int n, num, temp, linePerProc, flag = 1, flag_id, *flagArray, flagSum = 1;
    double *array, *recv, max, *maxArray, m = 0, *newArray, *newRecv, min, *minArray, tempMin;

    // Process 0 read the data
    if (rankOfProc == 0)
    {
        printf("Give N: ");
        scanf("%d", &n);

        // Initialize array
        // Array is 2D for us but 1D for the program.
        // So we allocate n*n space
        temp = n * n;
        array = (double *)malloc(temp * sizeof(double));

        // Read elements
        for (int i = 0; i < temp; i++)
        {
            printf("Give element:");
            scanf("%lf", &array[i]);
        }

        num = temp / numOfProc;
        linePerProc = n / numOfProc;

        // Allocating memory
        flagArray = (int *)malloc(numOfProc * sizeof(int));
        for (int i = 0; i < numOfProc; i++)
        {
            flagArray[i] = 1;
        }
        maxArray = (double *)malloc(numOfProc * sizeof(double));
        newArray = (double *)malloc(temp * sizeof(double));
        minArray = (double *)malloc(numOfProc * sizeof(double));
    }

    // Sending needed values to all processes
    MPI_Bcast(&temp, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&linePerProc, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocating memory for the receiving array
    recv = (double *)malloc(num * sizeof(double));

    // Send the right part of the array to the right process
    MPI_Scatter(array, num, MPI_DOUBLE, recv, num, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // For every element on the array we check if it is in the diagonal position
    // If it is, we check if it has the correct value to be a diagonal matrix.
    // Some times this triggers false positives so we store which i (index) trigered
    // the flag (flag=0) and check again if it was a false positive.
    for (int i = 0; i < num; i++)
    {
        for (int ncount = 0; ncount < linePerProc; ncount++)
        {
            if (recv[i] == 0 && i == (rankOfProc * linePerProc + ncount * (n + 1)))
            {
                flag = 0;
                flag_id = i;
            }
            else if (recv[i] != 0 && i != (rankOfProc * linePerProc + ncount * (n + 1)))
            {
                flag = 0;
                flag_id = i;
            }
            else if (i == flag_id)
            {
                flag = 1;
                break;
            }
            else
            {
                break;
            }
        }
    }

    free(recv);

    // Sending the parts of the array to process 0
    MPI_Gather(&flag, 1, MPI_INT, flagArray, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Process 0 takes the flags and check if any was triggered and responds appropriately.
    if (rankOfProc == 0)
    {
        for (int i = 0; i < numOfProc; i++)
        {
            if (flagArray[i] == 0)
            {
                printf("\nNO, the provided matrix is not diagonal.\n");
                printf("Terminated. Reason: Not diagonal.\n");
                free(array);
                free(flagArray);
                free(maxArray);
                free(newArray);
                free(minArray);
                flagSum=0;
            }
        }
        if(flagSum!=0) {
            printf("\nYes, the provided matrix is diagonal.\n");
        }
    }

    // Tell all processes to finalize if necessary
    MPI_Bcast(&flagSum, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (flagSum == 0)
    {
        MPI_Finalize();
        exit(0);
    }

    // Allocating memory for the receiving array
    recv = (double *)malloc(num * sizeof(double));

    // Send the right part of the array to the right process
    MPI_Scatter(array, num, MPI_DOUBLE, recv, num, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Calculate the max value of the diagonal elements.
    max = recv[0];

    for (int i = 0; i < num; i++)
    {
        for (int ncount = 0; ncount < linePerProc; ncount++)
        {
            if (i == (rankOfProc * linePerProc + ncount * (n + 1)) && max < fabs(recv[i]))
            {
                max = fabs(recv[i]);
                break;
            }
        }
    }

    // Send the max values to process 0
    MPI_Gather(&max, 1, MPI_DOUBLE, maxArray, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Process 0 compares max values
    if (rankOfProc == 0)
    {
        m = maxArray[0];
        for (int i = 1; i < numOfProc; i++)
        {
            if (m < maxArray[i])
            {
                m = maxArray[i];
            }
        }

        printf("Max value matrix A: %f\n", m);
    }

    // Send max value to all processes
    MPI_Bcast(&m, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // At the same time we create matrix B and check for min values
    newRecv = (double *)malloc(num * sizeof(double));
    tempMin = m - recv[0];
    for (int i = 0; i < num; i++)
    {
        for (int ncount = 0; ncount < linePerProc; ncount++)
        {
            if (i == (rankOfProc * linePerProc + ncount * (n + 1)))
            {
                newRecv[i] = m - recv[i];
                if (tempMin > newRecv[i])
                {
                    tempMin = newRecv[i];
                }
                break;
            }
            else
            {
                newRecv[i] = m;
                break;
            }
        }
    }

    // Process 0 collects pieces of matrix B and min values
    MPI_Gather(newRecv, num, MPI_DOUBLE, newArray, num, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&tempMin, 1, MPI_DOUBLE, minArray, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Process 0 prints matrix B and chooses a min value
    if (rankOfProc == 0)
    {
        for (int i = 0; i < temp; i++)
        {
            if (i % n == 0 && i != 0)
            {
                printf("\n");
            }
            printf("B: %lf\t", newArray[i]);
        }
        min = minArray[0];

        for (int j = 0; j < numOfProc; j++)
        {
            if (min > minArray[j])
            {
                min = minArray[j];
            }
        }
        printf("\nMin value matrix B: %lf\n", min);
    }

    free(recv);
    free(newRecv);

    if (rankOfProc == 0)
    {
        free(flagArray);
        free(array);
        free(maxArray);
        free(newArray);
        printf("\nTerminated. Reason: End of program.\n");
    }

    MPI_Finalize();
    return 0;
}