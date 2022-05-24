#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

// Function prototype declaration
int menu();

// Program start
int main(int argc, char **argv)
{
    int rankOfProc, numOfProc, n, num, min, max;
    float m;
    float *array;
    MPI_Status status;

    // Initialize MPI and check for initialization error
    if (MPI_Init(&argc, &argv) != 0)
    {
        printf("\nMPI initialization error\n");
        exit(1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rankOfProc);
    MPI_Comm_size(MPI_COMM_WORLD, &numOfProc);

    // do-while loop so it can repeat the whole thing
    do
    {
        // Process 0
        if (rankOfProc == 0)
        {
            // Menu to continue or exit the program
            if (menu() == 0)
            {
                break;
            }

            // Read the number of elements that user is going to input (from stdin)
            printf("\nGive number of elements (N)\n> ");
            scanf("%d", &n);

            // Calculate the number of elements to send to every process
            num = n / numOfProc;

            // Allocate memory for the incoming elements (from stdin)
            array = (float *)malloc(n * sizeof(float));

            // Check for allocation failure
            if (array == NULL)
            {
                printf("\nError allocating memory\n");
            }

            // Read elements
            for (int i = 0; i < n; i++)
            {
                printf("Give element %d\n> ", i);
                scanf("%f", &array[i]);
            }

            // Caclulate min and max values of elements that were given
            min = array[0];
            max = array[0];
            for (int i = 1; i < n; i++)
            {
                if (array[i] < min)
                    min = array[i];
                else if (array[i] > max)
                    max = array[i];
            }

            // Send the correct part of the array and the number of elements that it contains to the rest of the processes
            float *tempAdd;
            tempAdd = array + num;
            for (int i = 1; i < numOfProc; i++)
            {
                MPI_Send(&num, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
                MPI_Send(tempAdd, num, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
                // tempAdd used to shift the address of the array
                tempAdd += num;
            }
        }
        else
        {
            // Receive the number of elements and the part corresponding of the array
            MPI_Recv(&num, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);
            array = (float *)malloc(num * sizeof(float));
            MPI_Recv(array, num, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);
        }

        // Calculate results in every process
        float result = 0;
        for (int k = 0; k < num; k++)
        {
            result += array[k];
        }

        // Every other process than 0
        if (rankOfProc != 0)
        {
            // Every process send the results back to process 0
            MPI_Send(&result, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
        }
        // Process 0
        else
        {
            // Process 0 doing the final calculations for task 1
            float tempResult = 0, sum = 0, finalResult = 0;
            sum += result;
            for (int i = 1; i < numOfProc; i++)
            {
                MPI_Recv(&tempResult, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);
                sum += tempResult;
            }
            finalResult = sum / n;
            m = finalResult;
            printf("\nAverage: %f\n", finalResult);
        }

        // Process 0
        if (rankOfProc == 0)
        {
            for (int i = 1; i < numOfProc; i++)
            {
                // Send the result of task 1 to the other processes
                MPI_Send(&m, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
            }
        }
        // Every other process than 0
        else
        {
            // Receive data
            MPI_Recv(&m, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);
        }

        // Calculations that happen in every process
        result = 0;
        for (int k = 0; k < num; k++)
        {
            result += ((array[k] - m) * (array[k] - m));
        }

        // Every other process than 0
        if (rankOfProc != 0)
        {
            // Every process send the results back to process 0
            MPI_Send(&result, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
        }
        // Process 0
        else
        {
            // Receive data and final calculations for task 2
            float tempResult = 0, sum = 0, finalResult = 0;
            sum += result;
            for (int i = 1; i < numOfProc; i++)
            {
                MPI_Recv(&tempResult, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);
                sum += tempResult;
            }
            finalResult = sum / n;
            printf("\nVariation: %f\n", finalResult);
        }

        // Process 0
        if (rankOfProc == 0)
        {
            // Send min and max values to every process
            for (int i = 1; i < numOfProc; i++)
            {
                MPI_Send(&min, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
                MPI_Send(&max, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
            }
        }
        // Every other process than 0
        else
        {
            // Receive data
            MPI_Recv(&min, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&max, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);
        }

        // Calculations that happen in every process

        // Allocating memory
        float *resultArray = (float *)malloc(num * sizeof(float));

        // Check for allocation error
        if (resultArray == NULL)
        {
            printf("\nError allocating memory\n");
            exit(2);
        }

        for (int k = 0; k < num; k++)
        {
            resultArray[k] = ((array[k] - min) / (max - min)) * 100;
        }

        // Every other process than 0
        if (rankOfProc != 0)
        {
            // Send data back to process 0 and release reallocated memory
            MPI_Send(resultArray, num, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
            free(resultArray);
        }
        // Process 0
        else
        {
            // Final calculations for task 3

            // Allocating memory
            float *finalResultArray = (float *)malloc(n * sizeof(float));

            // Check for memory allocation error
            if (finalResultArray == NULL)
            {
                printf("\nError allocating memory\n");
                exit(2);
            }

            // We use index for knowing where we are generally in the final array
            int index = 0;

            // First append to final array the results from process 0
            for (int j = 0; j < num; j++)
            {
                finalResultArray[index] = resultArray[j];
                index++;
            }

            for (int i = 1; i < numOfProc; i++)
            {
                // Receive data from other processes and append them to final array
                MPI_Recv(resultArray, num, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);
                for (int j = 0; j < num; j++)
                {
                    finalResultArray[index] = resultArray[j];
                    index++;
                }
            }

            // Print final array
            printf("\nNew Vector: \n");
            for (int index = 0; index < n; index++)
            {
                printf("Vector position %d is %f \n", index + 1, finalResultArray[index]);
            }
            // Free allocated memory
            free(resultArray);
            free(finalResultArray);
        }

        // Free allocated memory
        free(array);

    } while (1);
    printf("Terminated");
    MPI_Finalize();
    return 0;
}

int menu()
{
    int option;
    printf("\nMenu\n");
    printf("0 -> Stop\n");
    printf("1 -> Continue\n");
    printf("> ");
    scanf("%d", &option);
    switch (option)
    {
    case 0:
        return 0;
        break;
    case 1:
        return 1;
        break;
    default:
        printf("\nWrong option\n");
        return menu();
        break;
    }
}