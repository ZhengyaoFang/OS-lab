#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[]){
    /*创建管道*/
    int p1[2],p2[2];   //p[0]是读端，p[1]是写端
    pipe(p1);           //child write,father read
    pipe(p2);           //father write,child read

    /*生成子程序*/
    int flag = fork();
    if(flag == 0){   //子程序执行逻辑
        char buffer[5];
        /*先进行读*/
        close(p2[1]);   //close pipe2's write
        close(p1[0]);   //close pipe1's read

        //First,read message from pipe2
        read(p2[0],buffer,5);
        int pid = getpid();
        printf("%d: received %s\n",pid,buffer);

        //Next,write message into pipe1
        write(p1[1],"pong",5);
    }else{                    //father 
        char buffer[5];
        close(p2[0]);   //write into p2[1]
        close(p1[1]);   //read from p1[0]
        /*First,write message into pip2*/
        write(p2[1],"ping",5);
        /*Next,read from pipe1*/
        read(p1[0],buffer,5);
        int pid = getpid();
        printf("%d: received %s\n",pid,buffer);
    }

    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    exit(0);
}