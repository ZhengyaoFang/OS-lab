#include "kernel/types.h"
#include "user.h"

void initial_child(int p1[],int p2[]){
    close(p1[1]);
    close(p2[0]);
}

int main(int argc,char* argv[]){

    /*Load the initial nums*/
    int initial_nums[35];

    for(int i=2;i<36;i++){
        initial_nums[i-2] = i;
    }
    initial_nums[34] = 0;   //end of the input

    /*initial the pipe*/
    int p1[2],p2[2],*status=0;    //p1 for self-read from father,p2 for self-write to child
    pipe(p1);
    pipe(p2);
    write(p2[1],initial_nums,140);



    /*Start the forks*/
    int ret = fork();
//    while(1){
        if(ret == 0){   //child ps
            initial_child(p1,p2);
            char prime[4],receive[4];
            read(p1[0],prime,4);
            int receive_num,prime_num = atoi(prime);
            if(prime_num == 0){
                exit(0);
            }
            printf("prime %d",prime);
            ret = fork();
//                if(ret == 0){continue;}

            do{
                read(p1[0],receive,4);
                receive_num = atoi(receive);
                if(receive_num%prime_num!=0 || receive_num == 0){
                    write(p2[1],receive,4);
                }
            }while(receive_num != 0);
            wait(status);
            exit(0);
        }else{
            for(int i=2;i<36;i++){
                write(p2[1],i+"0",4);
            }
            write(p2[1],"0",4);
            wait(status);
            exit(0);
        }

//    }

}


