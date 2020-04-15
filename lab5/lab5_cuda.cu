/****************************************************************************************/
/* CSE 5441 - Lab assignment 5                                                          */
/*      This is a CUDA program to implements a matrix operation involing very large     */
/*      dimesion (4096 X 4096) matrix using a gpu. The implementation uses a grid of    */
/*      size (256 X 256) and block of size (16 X 16) with each block having its own     */
/*      thread.                                                                         */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Compile using 'nvcc' compiler                                                        */
/* Created on Apr 12 2020                                                               */
/*                                                                                      */
/****************************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N 4096                                  // row/col size of matrix
#define SIZE N*N                                // dimension of matrix
#define MAX_CELL_VALUE 2                        // max value of cells in matrix

#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }

// Method to display any error returned by cuda routines
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess) 
   {
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

// Method to initialize a square matrix of size (N X N)
//      -> if setToZero is 1 then cell values will be 0, else it will be set to randomly generated float value
void initMatrix(float *inputMatrix, int setToZero)
{
    srand(time(NULL));                                  // initialize the random number generator
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
            inputMatrix[i*N + j] = (!setToZero) ? ((float)rand()/(float)RAND_MAX) * MAX_CELL_VALUE : 0.0;
    }
}

// Method to display a square matrix of size (N X N)
void printMatrix(float *matrix)
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
            printf("%0.1f\t", matrix[i*N + j]);
        
        printf("\n");
    }
}

// Method to compare two a square matrices of size (N X N)
void cmpMatrix(float *firstArr, float *secondArr)
{
    char cell1[20], cell2[20];
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            sprintf(cell1, "%0.1f", firstArr[i*N + j]);
            sprintf(cell2, "%0.1f", secondArr[i*N + j]);
            if (strcmp(cell1, cell2) != 0)
            {
                printf("Two matrices are not equal. Array1=%s and Array2=%s at [%d][%d]\n", cell1, cell2, i, j);
                return;
            }
        }
    }

    printf("Correct answer\n");
}

// Method to perform a operation square matrix of size (N X N) serially.
void doMatrixOperationSerial(float *result, float *inputMatrix)
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            for (int k = 0; k < N; k++)
                result[i*N + j] += inputMatrix[k*N + i] * inputMatrix[k*N + j];
        }
    }
}

// Method to perform a operation square matrix of size (N X N) parallely using CUDA.
__global__ void doMatrixOperation(float *result, float *inputMatrix, int n)
{
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    int i = blockIdx.y * blockDim.y + threadIdx.y;
    for (int k = 0; k < n; k++)
        result[i*n + j] += inputMatrix[k*n + i] * inputMatrix[k*n + j];
}

// main entry point
int main()
{
    float *h_result = (float*)malloc(sizeof(float)*SIZE);                   // host result matrix.
    float *h_resultSerial = (float*)malloc(sizeof(float)*SIZE);             // host result matrix to store result from serial computation.
    float *h_inputMatrix = (float*)malloc(sizeof(float)*SIZE);              // host input matrix on which conputation is performed.
    float *d_result;                                                        // device result matrix.
    float *d_inputMatrix;                                                   // device input matrix on which computation is performed.

    initMatrix(h_result, 1);                                                // init host results matrix to 0s. This is used to store result from cuda computation.
    initMatrix(h_inputMatrix, 0);                                           // init host input matrix to random floating point numbers.
    initMatrix(h_resultSerial, 1);                                          // init host results matrix to 0s. This is used to store result from serial computation.

    gpuErrchk(cudaMalloc((void **)&d_result, sizeof(float)*SIZE));          // allocate memory for result matrix on device.
    gpuErrchk(cudaMalloc((void **)&d_inputMatrix, sizeof(float)*SIZE));     // allocate memory for input matrix on device.

    // init result and input matrix on device by sending matrix from host to device
    gpuErrchk(cudaMemcpy(d_result, h_result, sizeof(float)*SIZE, cudaMemcpyHostToDevice));              
    gpuErrchk(cudaMemcpy(d_inputMatrix, h_inputMatrix, sizeof(float)*SIZE, cudaMemcpyHostToDevice));

    dim3 dimGrid(256, 256);                                                 // set grid layout to (256 X 256)
    dim3 dimBlock(16, 16);                                                  // set block layout to (16 X 16)
    doMatrixOperation<<<dimGrid, dimBlock>>>(d_result, d_inputMatrix, N);   // call kernel
    cudaDeviceSynchronize();                                                // Wait for compute device to finish.
    gpuErrchk(cudaPeekAtLastError());                                       // check for errors
    gpuErrchk(cudaDeviceSynchronize());                                     // check for errors

    // copy final results of matrix operation from device (result array) to host (results array).
    gpuErrchk(cudaMemcpy(h_result, d_result, sizeof(float)*SIZE, cudaMemcpyDeviceToHost));
    
    doMatrixOperationSerial(h_resultSerial, h_inputMatrix);
    cmpMatrix(h_result, h_resultSerial);
    // printf("Parllel compute:\n");
    // printMatrix(h_result);
    // printf("Serial compute:\n");
    // printMatrix(h_resultSerial);

    return(0);
}