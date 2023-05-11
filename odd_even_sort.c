#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int cmp(const void* a, const void* b){
    float* va = (float*) a;
    float* vb = (float*) b;

    if(*va > *vb) return 1;
    else if(*va < *vb) return -1;
    else return 0;
}

void merge_func(float* local_data, float* temp, float* merge, int local_n_of_a, int local_n_of_b){
    //這邊做merge
    int a=0, b=0, c=0, j;
    
    while(a < local_n_of_a && b < local_n_of_b){ //比較小的先填進merge
        if(local_data[a] < temp[b]){
            merge[c++] = local_data[a++];
        }
        else{ //local_data[a] >= temp[b]
            merge[c++] = temp[b++];
        }
    }
    //補填剩下的
    while(a < local_n_of_a){
        merge[c++] = local_data[a++];
    }
    while(b < local_n_of_b) {
        merge[c++] = temp[b++];
    }
    //前半塞給local_data
    for(j=0; j<local_n_of_a; j++) local_data[j] = merge[j];
}

int main(int argc, char** argv) {
    
    int rank, p, i, j;
    float* data = NULL;
    float* local_data = NULL;
    float* merge = NULL;
    float* temp = NULL;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &p);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    // 讀取參數
    int n = atoi(argv[1]); //紀錄有幾筆數字要sort
    char *input_filename = argv[2]; //紀錄input file
    char *output_filename = argv[3]; //紀錄output file
    MPI_File input_file, output_file;
    data = (float*)malloc(n * sizeof(float));

    // Scatter data 給每個process
    int local_n = n / p;
    int remainder = n % p; //多餘的elements
    if(rank == 0){
        printf("p = %d\n", p);
        printf("local_n = %d\n", local_n);
        printf("remainder = %d\n", remainder);
        fflush(stdout);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    //分配多餘的elements
    if(rank < remainder) local_data = (float*)malloc( (local_n + 1) * sizeof(float));
    else local_data = (float*)malloc(local_n * sizeof(float));

    //準備scatterv
    int* send_count = (int*) malloc(p * sizeof(int));
    int* displs = (int*) malloc(p * sizeof(int));

    for(i=0; i<p; i++){
        send_count[i] = (i<remainder) ? (local_n+1) : local_n;
        displs[i] = (i==0) ? 0 : (displs[i-1] + send_count[i-1]);
    }

    MPI_File_open(MPI_COMM_WORLD, input_filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &input_file);
    MPI_File_read_at(input_file, sizeof(float) * displs[rank], local_data, send_count[rank], MPI_FLOAT, MPI_STATUS_IGNORE);
    MPI_File_close(&input_file);

    //process各自sort
    qsort(local_data, send_count[rank], sizeof(float), cmp);
    
    //odd-even sorting
    temp = (float*)malloc((2*local_n) * sizeof(float));
    merge = (float*)malloc(2*(local_n+1) * sizeof(float));
    for(i=0; i<p; i++){ //跑size次，代表有幾個數字
        //even phase偶數階段
        if(i % 2 == 0){
            if(rank % 2 == 0){ //rank是偶數
                if(rank < p-1){ //確保在範圍內
                    MPI_Recv(temp, send_count[rank+1], MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //從下一個奇數項領取資料
                    
                    merge_func(local_data, temp, merge, send_count[rank], send_count[rank+1]);  
                    //後半傳給奇數
                    MPI_Send(&merge[send_count[rank]], send_count[rank+1], MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD);
                }
            }
            else{ //rank是奇數
                if(rank >0){ //確保在範圍內
                    MPI_Send(local_data, send_count[rank], MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD);
                    MPI_Recv(temp, send_count[rank], MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    
                    for(j=0; j<send_count[rank]; j++){
                        local_data[j] = temp[j];
                    }
                }
            }
        }
        else{ //odd phase奇數階段
            if(rank % 2 == 0){ //rank是偶數
                if(rank > 0 ){ //確保在範圍內
                    MPI_Send(local_data, send_count[rank], MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD);
                    MPI_Recv(temp, send_count[rank], MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    for(j=0; j<send_count[rank]; j++){
                        local_data[j] = temp[j];
                    }
                }
            }
            else if(rank%2 == 1){ //rank是奇數
                if(rank < p-1){
                    MPI_Recv(temp, send_count[rank+1], MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //從下一個奇數項領取資料
                    merge_func(local_data, temp, merge, send_count[rank], send_count[rank+1]);
                    //後半傳給偶數
                    MPI_Send(&merge[send_count[rank]], send_count[rank+1], MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD);
                }
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    //Gather資料給rank 0 
    MPI_Barrier(MPI_COMM_WORLD); //同步所有process
    MPI_Gatherv(local_data, send_count[rank], MPI_FLOAT, data, send_count, displs, MPI_FLOAT, 0, MPI_COMM_WORLD);

    MPI_File_delete(output_filename, MPI_INFO_NULL);//先刪掉檔案以做到覆寫效果
    MPI_File_open(MPI_COMM_WORLD, output_filename, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &output_file);  
    if(rank==0) MPI_File_write_at(output_file, sizeof(float) *0, data, n, MPI_FLOAT, MPI_STATUS_IGNORE); 
    MPI_File_close(&output_file);
    

    free(temp);
    free(data);
    free(merge);
    free(local_data);
    MPI_Finalize();
    return 0;
}
