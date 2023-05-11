# usage
### Makefile 
  compile the program
### job_script
  launch slurm job script using sbatch. 
  command : sbatch job_script
  note : you can modify the job_script so that it fits your demand
  
### print_output.exe
  use this to print out the output.txt that you get after running odd_even_sort
  because the output uses MPI data format. 
  command : print_output.exe output.txt


# implementations

Note: IntelMPI is used to compile odd_even_sort 

First, initialize and read three arguments from argv. The first argument is the number of elements, n, to be sorted. The second argument is the input file path, and the third argument is the output path.

Based on the number of elements, n, and the number of processors, p, calculate the number of elements each processor needs to process. This can be done by dividing n by p, local_n = n/p, and also calculate the remainder, remainder = n % p. The extra elements are then allocated to the first "remainder" processors. Use malloc to allocate the required data space based on these values.

For example, if there are 20 elements and 6 processors, each processor will first take 3 elements, and the extra 2 elements that cannot be evenly divided will be assigned to the first two processors, rank 0 and rank 1.

Calculate the offset for MPI_File_read_at based on local_n and remainder to determine which data each processor should read.

In each processor, use qsort to sort the local elements.

Start odd-even sort. The boundary conditions are considered in the judgment.

In the even phase:
Odd ranks send elements to even ranks, and even ranks use the merge_func (see 7) to sort the local elements of the two processors by size. Then, the first half of the elements (the smaller ones) are left with the even ranks, and the second half of the elements (the larger ones) are sent to the odd ranks.

In the odd phase:
Even ranks send elements to odd ranks, and odd ranks use the merge_func (see 7) to sort the local elements of the two processors by size. Then, the first half of the elements (the smaller ones) are left with the odd ranks, and the second half of the elements (the larger ones) are sent to the even ranks.

Use MPI_Barrier and MPI_Gatherv to collect the sorted data from each process to rank 0 and write it to output.txt. Finally, free the space allocated by malloc and use MPI_Finalize.

Additional information on merge_func():
Adopting the concept of merge sort, two pre-sorted data sets, a and b, are read, and a space c is allocated to contain the two data sets. Each time the smaller item from a and b is added to c, and its index is increased until all items from a and b have been added to c. Then, c will be the sorted data.

# Detailed Chinese version Hackmd document
https://hackmd.io/5WSG_jASSb6XE3R94VA3Vg
