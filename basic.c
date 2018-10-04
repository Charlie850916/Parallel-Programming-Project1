#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);

	int n = atoi(argv[1]);

	int rank, size;
      	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	MPI_Status status;

	// data input
	
	int dataSize = n/size;
	int tmp = n - dataSize*size;
	int base = dataSize*rank;

	for(int i=0 ; i<size && tmp>0 ; ++i)
	{
		if(i==rank)
			dataSize++;
		if(i<rank)
			base++;
		tmp--;
	}

	float *data = (float*) malloc(dataSize*sizeof(float));

	MPI_File fin;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &fin);
	MPI_File_seek(fin, base*sizeof(float), MPI_SEEK_SET);
	MPI_File_read_all(fin, data, dataSize, MPI_FLOAT, &status);
	MPI_File_close(&fin);

	// odd even transposition sort
	MPI_Barrier(MPI_COMM_WORLD);

	for(int times = 0 ; times < n ; ++times)
    	{
		MPI_Barrier(MPI_COMM_WORLD);

        	float dataInL, dataInR, dataOutL = data[0], dataOutR = data[dataSize-1];

		MPI_Request req1, req2, req3, req4;

        	if(rank > 0)
            		MPI_Isend(&dataOutL, 1, MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD, &req1);

		if(rank < size-1)
			MPI_Isend(&dataOutR, 1, MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD, &req2);

		if(rank > 0)
			MPI_Irecv(&dataInL, 1, MPI_FLOAT, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &req3);

       		if(rank < size-1)
            		MPI_Irecv(&dataInR, 1, MPI_FLOAT, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, &req4);

		for(int i = 0 ; i<dataSize ; ++i)
		{
			int id = base+i;
			if(id%2 == times%2)
				if(i+1 < dataSize)
					if(data[i] > data[i+1])
					{
						float tmp = data[i];
						data[i] = data[i+1];
						data[i+1] = tmp;
					}
		}

        	if(rank > 0)
            	{
			MPI_Wait(&req1, &status);
			MPI_Wait(&req3, &status);
		}

        	if(rank < size-1)
            	{
			MPI_Wait(&req2, &status);
			MPI_Wait(&req4, &status);
		}

		if(base%2 != times%2)
			if(rank > 0)
				if(data[0] < dataInL)
					data[0] = dataInL;

		if((base+dataSize-1)%2 == times%2)
			if(rank < size-1)
				if(data[dataSize-1] > dataInR)
					data[dataSize-1] = dataInR;
    	}

	// data output
	MPI_Barrier(MPI_COMM_WORLD);

	MPI_File fout;
	MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &fout);
	MPI_File_seek(fout, base*sizeof(float), MPI_SEEK_SET);
	MPI_File_write(fout, data, dataSize, MPI_FLOAT, &status);
	MPI_File_close(&fout);

	free(data);
	MPI_Finalize();
	return 0;
}
