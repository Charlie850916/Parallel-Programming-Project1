#include <bits/stdc++.h>
#include "mpi.h"

using namespace std;


int main(int argc, char* argv[])
{
	ios_base::sync_with_stdio(false);
	cin.tie(0);
	int n = atoi(argv[1]), rank, size, tmp, base, dataSize, leftSize, rightSize, offset, cnt, id, lid, rid;
	float *data, *left, *right, *a, *ttmp;
	char sorted, odd_even, send;

	MPI_Init(&argc, &argv);
      	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	dataSize = leftSize = rightSize = n/size;
	tmp = n - dataSize*size;
	base = dataSize*rank;

	if(tmp>rank)
		dataSize++;
	if(rank>0 && tmp>rank-1)
		leftSize++;
	if(rank<size-1 && tmp>rank+1)
		rightSize++;

	base += min(tmp, rank);

	data = new float[dataSize];
	left = new float[leftSize];
	right = new float[rightSize];
	a = new float[dataSize];
	offset = base*sizeof(float);

	MPI_File fin, fout;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &fin);
	MPI_File_read_at_all(fin, offset, data, dataSize, MPI_FLOAT, MPI_STATUS_IGNORE);
	MPI_File_close(&fin);

	if(size>n)
		size = n;

	sort(data, data+dataSize);

	sorted = 0;
	MPI_Request req1, req2;

	while(!sorted)
    	{
		sorted = 1;
		for(odd_even = 0 ; odd_even < 2 ; ++odd_even)
		{
			if(rank<size)
			{
        			if(rank%2==odd_even)
				{
					if(rank<size-1)
					{
						MPI_Isend(data, dataSize, MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD, &req1);
						MPI_Irecv(right, rightSize, MPI_FLOAT, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, &req2);
						MPI_Wait(&req2, MPI_STATUS_IGNORE);
						if(right[0] < data[dataSize-1])
						{
							sorted = cnt = id = rid = 0;
							while(cnt < dataSize)
							{
								if(rid>=rightSize)
									a[cnt++] = data[id++];
								else if(id>=dataSize)
									a[cnt++] = right[rid++];
								else if(data[id] <= right[rid])
									a[cnt++] = data[id++];
								else
									a[cnt++] = right[rid++];
							}
							ttmp = data;
							MPI_Wait(&req1, MPI_STATUS_IGNORE);
							data = a;
							a = ttmp;
						}
					}
				}
				else
				{
					if(rank > 0)
					{
						MPI_Isend(data, dataSize, MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD, &req1);
						MPI_Irecv(left, leftSize, MPI_FLOAT, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &req2);
						MPI_Wait(&req2, MPI_STATUS_IGNORE);
						if(data[0] < left[leftSize-1])
						{
							sorted = 0, id = cnt = dataSize-1, lid = leftSize-1;
						        while(cnt >= 0)
							{
								if(lid<0)
									a[cnt--] = data[id--];
								else if(id<0)
									a[cnt--] = left[lid--];
								else if(left[lid] < data[id])
									a[cnt--] = data[id--];
								else
									a[cnt--] = left[lid--];
							}
							ttmp = data;
							MPI_Wait(&req1, MPI_STATUS_IGNORE);
							data = a;
							a = ttmp;
						}
					}
				}
			}
		}
		send = sorted;
		MPI_Allreduce(&send, &sorted, 1, MPI_CHAR, MPI_LAND, MPI_COMM_WORLD);
    	}
	MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &fout);
	MPI_File_write_at_all(fout, offset, data, dataSize, MPI_FLOAT, MPI_STATUS_IGNORE);
	MPI_File_close(&fout);
	MPI_Finalize();
	return 0;
}
