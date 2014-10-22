
#include<stdio.h>
#include<utmp.h>
#include<unistd.h>
#include<fcntl.h>
#include<time.h>

void Timeformat(long timearg)
{
        char *p;    /*声明一个指针变量，在后面用来存放时间的地址*/
        p=ctime(&timearg); /*格式化时间*/
        printf("%12.12s\n", p+4); /*ctime返回的字符串从第四个字符开始*/
}//void

void Showmessage(struct utmp * utmparg)
{
   if(utmparg->ut_type != USER_PROCESS)/*判断登陆类型时候是“用户”*/
   return;
   printf("%-8.8s", utmparg->ut_name ); /*输出登陆的用户名*/
   printf(" ");
   printf("%-8.8s",utmparg->ut_line); /*输出登陆的TTY*/
   printf(" ");
   Timeformat(utmparg->ut_time); /*输出登陆的时间*/
}

int who()
{

    struct utmp pointer;   /*声明utmp结构变量*/
    int utmpfd;  /*打开的utmp文件存放在这里*/
    int re=sizeof(pointer);

    if((utmpfd = open(UTMP_FILE, O_RDONLY)) == 1)
    {/*打开utmp文件，前面说过UTMP_FILE文件在paths.h中有定义*/
    printf("error\n");
    //exit(1);
    return 0;
    }

    while( read(utmpfd, &pointer, re) == re)/*读取文件中的内容*/
    Showmessage(&pointer); 	/*调用Showmessage函数*/
    close(utmpfd); 		/*关闭utmp文件*/

}

char* whoami(){

	FILE *fd = popen("who -m|awk '{print $1}'","r");
	char *buff = (char *)malloc(64);
	int bytes = fread(buff,64,1,fd);
	int i = 0;
	while(i<64){
	if(buff[i]=='\n'){
		buff[i] = '\0';
		break;
	}//if
	i++;
	}//while
	return (char *)buff;

}//char *

