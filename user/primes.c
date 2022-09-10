#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[]){
    /*Load the initial nums*/
    int initial_nums[35];

    for(int i=2;i<36;i++){
        initial_nums[i-2] = i+'0';
    }
    initial_nums[34] = '0';   //end of the input

    /*initial the pipes*/
    int p[15][2];
    
    /*initial the first pipe between main and child1*/
    int *status,mark = 0;
    pipe(p[mark]);


    /*Start the forks*/
    int ret = fork();
    while(1){
        if(ret == 0){//the child ps
            close(p[mark][1]);
            mark = mark+1;
            int prime[1],receive[1];
            read(p[mark-1][0],prime,4);
            int receive_num,prime_num = prime[0]-'0';
            if(prime_num == 0){
                exit(0);
            }else{
                printf("prime %d\n",prime_num);
                pipe(p[mark]);
                ret = fork();
                if(ret == 0){
                    continue;
                }
                close(p[mark][0]);
                do{
                    read(p[mark-1][0],receive,4);
                    receive_num = receive[0]-'0';
//                    printf("receive_num:%d\n",receive_num);
                    if(receive_num%prime_num!=0 || receive_num == 0){
                        write(p[mark][1],receive,4);
                    }
                }while(receive_num != 0);
                wait(status);
                exit(0);
            }
        }else if(ret<0){
            printf("Error for the fork!!!\n");
        }else{//main ps
            close(p[mark][0]);
            write(p[mark][1],initial_nums,140);
            wait(status);   //等待子进程结束后再结束
            exit(0);
        }
    }
}
