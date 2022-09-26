#include "kernel/types.h"
#include "user.h"


int main(int argc,char* argv[]){
    while(1){
        char new_argv[20][100]={'\0'};
        int arg_num=0;

        for(int i=1;i<=argc;i++){
            strcpy(new_argv[i], argv[i]);
            arg_num++;
        }
        char buf[512]={'\0'}; 
        int num = arg_num;
    
        if(strlen(gets(buf,512)) != 0){
            char *p,*q;
            p = buf;
            while(*p != '\0'){
                //printf("%d",(*p != ' '));
                if(*p != ' '){
                    q=p;
                    while(*q!=' ' && *q!='\n' && *q!='\0'){
                        q++;
                    }
                    *q = '\0';
                    strcpy(new_argv[num++],p);
                    p= q+1;
                }else{
                    p++;
                }
            }

            char *argv_s[20];
            for(int i=0;i<num;i++){
                (argv_s[i]) = new_argv[i+1];
            }

            int ret = fork();
            if(ret == 0){
                int back = exec(argv[1],argv_s);
                if(back == -1){
                    fprintf(2,"Faild to execute %s\n",argv[1]);
                }
            }
        
            int status;
            wait(&status);
            
        }else{
           exit(0); 
        }
    
    }
    exit(0);
    
}