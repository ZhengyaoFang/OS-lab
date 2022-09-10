#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void checkname(char* path,char* name){
    /*Find the last '/'*/
    char *p;
    for(p=path+strlen(path);p>=path && *p != '/';p--);
    p++;
    if(strcmp(p, name) == 0){
        printf("%s\n", path);
    }
    return ;
}

void find(char* path,char* name){
    char buf[512],*p;
    int fd;
    struct dirent de;
    struct stat   st;

    if((fd = open(path,0)) < 0){  //error occured when read the file
        fprintf(2,"find: cannot open %s\n", path);
        return ;
    }

    if(fstat(fd, &st) < 0){     //error occured when get the stat of the file
        fprintf(2,"ls: cannot stat %s\n", path);
        close(fd);
        return ;
    }

    /*First,whether it is a file*/
    switch(st.type){
        case T_FILE:            //this is a file,check whether it's name is what we want to find.
            checkname(path,name);
            close(fd);
            break;
    /*This is a director,just get in to recursively checkout the subfile.*/
        case T_DIR:             
            /* Let the buffer become the "path/" */
            strcpy(buf, path);
            p = buf + strlen(buf);
            *(p++) = '/';
            /* Start to read the dirent of this director.*/
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0 || strcmp(de.name,".") == 0 || strcmp(de.name,"..") == 0){continue;} //TODO:Don't sure whether find instruction need to throw away the blank file.
                memmove(p,de.name,DIRSIZ);  //get the sub-file path
                find(buf,name);
            }
        break;
    }
    return ;
    
}

int main(int argc,char* argv[]){
    if(argc!=3){
        printf("Please input the right instruct:find path name\n");
        exit(0);
    }else{
        find(argv[1],argv[2]);
        exit(0);
    }
}