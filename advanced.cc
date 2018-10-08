#include <iostream>
#include <algorithm>
#include "mpi.h"

using namespace std;

int main(int argc, char* argv[])
{
	ios::sync_with_stdio(false);
	cin.tie(0);

	MPI_Init(&argc, &argv);

	int n = atoi(argv[1]);

	int rank, size;
      	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	MPI_Status status;

	// data input
	int dataSize = n/size, leftSize = n/size, rightSize = n/size;
	int tmp = n - dataSize*size;
	int base = dataSize*rank;

	if(tmp>rank)
		dataSize++;
	if(rank>0 && tmp>rank-1)
		leftSize++;
	if(rank<size-1 && tmp>rank+1)
		rightSize++;

	base += min(tmp, rank);

	float *data = new float[dataSize];
	float *left = new float[leftSize];
	float *right = new float[rightSize];

	MPI_File fin, fout;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &fin);
	MPI_File_seek(fin, base*sizeof(float), MPI_SEEK_SET);
	MPI_File_read_all(fin, data, dataSize, MPI_FLOAT, &status);
	MPI_File_close(&fin);

	if(size>n)
		size = n;

	sort(data, data+dataSize);

	// odd even sort by rank
	MPI_Request req1, req2;

	char sorted = 0;
	while(!sorted)
    	{
		sorted = 1;
		for(int odd_even = 0 ; odd_even < 2 ; ++odd_even)
		{
			if(rank<size)
			{
        			if(rank%2==odd_even)
				{
					if(rank<size-1)
					{	
						MPI_Isend(data, dataSize, MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD, &req1);
						MPI_Irecv(right, rightSize, MPI_FLOAT, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, &req2);
						MPI_Wait(&req2, &status);

						if(right[0] < data[dataSize-1])
						{
							float *a = new float[dataSize], *ttmp;
							sorted = 0;
							int cnt = 0, rid = 0, id = 0;
							while(cnt < dataSize) // merge
							{
								if(rid >= rightSize)
									a[cnt++] = data[id++];
								else if(id >= dataSize)
									a[cnt++] = right[rid++];
								else if(data[id] < right[rid])
									a[cnt++] = data[id++];
								else
									a[cnt++] = right[rid++];

							}
							MPI_Wait(&req1, &status);
							ttmp = data;
							data = a;
							delete ttmp;
						}
					}
				}
				else
				{
					if(rank > 0)
					{
						MPI_Isend(data, dataSize, MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD, &req1);
						MPI_Irecv(left, leftSize, MPI_FLOAT, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &req2);
						MPI_Wait(&req2, &status);

						if(data[0] < left[leftSize-1])
						{
							float *a = new float[dataSize], *ttmp;
							sorted = 0;
							int cnt = dataSize-1, lid = leftSize-1, id = dataSize-1;
						        while(cnt >= 0) // merge
							{
								if(lid < 0)
									a[cnt--] = data[id--];
								else if(id < 0)
									a[cnt--] = left[lid--];
								else if(left[lid] < data[id])
									a[cnt--] = data[id--];
								else
									a[cnt--] = left[lid--];
							}
							MPI_Wait(&req1, &status);
							ttmp = data;
							data = a;
							delete ttmp;
						}
					}
				}
			}
		}
		char send = sorted;
		MPI_Allreduce(&send, &sorted, 1, MPI_CHAR, MPI_LAND, MPI_COMM_WORLD);
    	}

	// data output
	MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &fout);
	MPI_File_seek(fout, base*sizeof(float), MPI_SEEK_SET);
	MPI_File_write_all(fout, data, dataSize, MPI_FLOAT, &status);
	MPI_File_close(&fout);

	// finish
	delete data;
	delete left;
	delete right;
	MPI_Finalize();
	return 0;
}
