/*
chatserver
Simple toy chat server
Martin Cochner, 2010

-----------------------------------

usage: 
<NAME>::<MESSAGE>  ... send <MESSAGE> to the nick <NAME>
all::<MESSAGE>  ... send <MESSAGE> to all the connected people
!ls ... print all the connected users


Model situation:

Let's say that we have 3 users:
adam
bob
cecil

and where bob and cecil already connected to the server. Adam connects the last and 
bob greets him with the message "Ahooj!". Afterwards Adam is send "Hello everyone!" to 
everone who is connected.

----------------------------------
Situation from the adam's point of view:

adam@asus:~/syso/syso/showip$ nc 127.0.0.1 6666
What is your nick? adam
Welcome adam!
bob >> adam::Ahoj!
!ls

 User name    socket 	 IP Address 	 Authorized 
.      adam 	        4 	 127.0.0.1	 1 
.       bob 	        5 	 127.0.0.1	 1 
.     cecil 	        6 	 127.0.0.1	 1 
all::Hello everyone! 

----------------------------------

Situation from the bob's point of view:

martin@asus:~$ nc 127.0.0.1 6666
What is your nick? bob
Welcome bob!
adam::Ahoj!
adam >> all::Hello everyone!

----------------------------------
Situation from the cecil's point of view:
cecil@asus:~$ nc 127.0.0.1 6666
What is your nick? cecil
Welcome cecil!
adam >> all::Hello everyone!

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdbool.h>

#include "list.h"

#define PORT 6666
#define MAXLIST 5
#define DEBUG
#define MAXBUF 100

char q1[]="What is your nick? ";
char space[]=" >> ";

/* delimiter of name and message*/
char str2[]="::";

int server_fd, new_fd;

int user_add(struct user_list_t *user_list , struct sockaddr_in *addr, int new_fd);
int user_rem(struct user_list_t *user_list, int sfd_rem);
struct user_t *user_find_sfd(struct user_list_t *user_list, int i);
struct user_t *user_find_name(struct user_list_t *user_list, char *name_loc);
int user_auth_true(struct user_t *user);
void user_print(struct user_list_t *user_list, int fd);

int user_message(struct user_list_t *user_list, fd_set *main_fds_p, char buf[], int server_fd, int i, int nfds);
int user_message_parse(struct user_list_t *user_list, char *buf, int fromsfd);

void Init();
int main(int argc, char *argv[])
{
    struct user_list_t user_list={0, NULL,NULL};
    fd_set main_fds, read_fds;
    struct sockaddr_in cl_addr;
    char buf[MAXBUF];

    Init();
#ifdef DEBUG
    printf("Init() succesful!\n");
#endif

    FD_ZERO(&main_fds);
    FD_ZERO(&read_fds);

    /* add server to main_fds */
    FD_SET(server_fd, &main_fds);

    /* the highest file descriptor */
    int nfds = server_fd;

    while (1)
    {
#ifdef DEBUG
        printf("In the main loop!\n\n");
#endif

        read_fds = main_fds;
        if (select(nfds+1, &read_fds,  NULL, NULL, NULL)==-1)
        {
            perror("select");
            exit(1);
        }

#ifdef DEBUG
        printf("select() ... OK\n");
#endif

        /* going throug filedescriptors to read data*/
        for (int i=0; i<=nfds; i++)
        {
            /* */
            if (FD_ISSET(i, &read_fds))
            {

#ifdef DEBUG
                printf("connection from ... OK \n");
#endif

                /* NEW CONNECTION */
                if (i==server_fd)
                {
                    size_t addrlen = sizeof(cl_addr);
                    if ((new_fd = accept(server_fd, (struct sockaddr * )&cl_addr, &addrlen))==-1)
                    {
                        perror("accept()");
                    }
                    else
                    {
                        FD_SET(new_fd,&main_fds);
                        if (new_fd>nfds)
                        {
                            nfds=new_fd;
                        }
                        if (send(new_fd, q1, strlen(q1)+1, 0) == -1)
                                printf("send() error %d",new_fd);
                        printf("Client from %s on socket %d\n", inet_ntoa(cl_addr.sin_addr), new_fd);

                        user_add(&user_list, &cl_addr, new_fd);
                        user_print(&user_list, 1);

                    }
                }
                else
                /* Handling old Clients */
                {
                    int nbytes;
                    memset(buf, 0, sizeof buf);
                    if ((nbytes=recv(i, buf, sizeof buf, 0))<=0)
                    {
                        // Error
                        if (nbytes == 0)
                        {
                            printf("Client from socket %d is gone \n",i);
                            if (user_rem(&user_list, i)==-1)
                            {
                                perror("user_rem");
                            }
                        }
                        else
                        {
                            perror("recv()...");
                        }
                        close(i);
                        FD_CLR(i, &main_fds);
                    }
                    else
                    {
                        /* handling data from client */
                        /* who is writing */
                        #ifdef DEBUG
                            printf("data from client from socket %d \n",i);
                        #endif
                        struct user_t *p=user_find_sfd(&user_list, i);
                        /* is user authentificated? */
                        if (user_auth_true(p)==0)
                        {
                            /* Did he send username? */
                            if ((strlen(buf))<MAXNAME)
                            {
                                /* Make it a username */
                                if (buf[strlen(buf)-1]=='\n')
                                {
                                    buf[strlen(buf)-1]='\0';
                                    memset(p->name,0,sizeof(p->name));
                                    strcat (p->name, buf);
                                    p->auth=1;
                                    /* Welcome new user */
                                    memset(buf, 0, MAXBUF*(sizeof(char)));
                                    sprintf(buf, "Welcome %s!\n",p->name);
                                    if (send(i, buf, strlen(buf)+1, 0) == -1)
                                        printf("send() error %d",i);
                                }
                                else
                                {
                                    printf("Authorization of client on socket %d unsucesfull", i);
                                }
                            }
                        }
                        else
                        {
                            /* Client is authorized and he sended message */
                            user_message(&user_list, &main_fds, buf, server_fd, i, nfds);
                        }

                        #ifdef DEBUG
                            user_print(&user_list,1);
                        #endif
                    }
                }
            }
        }
    }
}

void Init()
{
    struct sockaddr_in my_addr={0};

    if ((server_fd = socket(PF_INET, SOCK_STREAM, 0))==-1)
    {
        perror("socket()");
        exit(1);
    }
#ifdef DEBUG
    printf("socket() ... OK \n");
#endif

    /* Allow to reuse port.*/
    int yes=1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr=INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&my_addr, sizeof my_addr)==-1)
    {
        printf("bind()");
        exit(1);
    }
#ifdef DEBUG
    printf("bind() ... OK \n");
#endif

    if (listen(server_fd, MAXLIST)==-1)
    {
        perror("listen()");
        exit(1);
    }
#ifdef DEBUG
    printf("listen() ... OK \n");
#endif
}

int user_add(struct user_list_t *user_list , struct sockaddr_in *addr, int new_fd)
{
    struct user_t *p;
    p=calloc(sizeof(struct user_t),1);
    /*not enough space */
    if (p==NULL)
        return -1;
    strcat(p->name,"Anonym");
    p->sfd=new_fd;
    strcat(p->ip_address, inet_ntoa(addr->sin_addr));
    p->auth=0;
    if (user_list->last!=NULL)
    {
        user_list->last->next=p;
        p->next=NULL;
    }
    if (user_list->first==NULL)
        user_list->first=p;
    user_list->last=p;
    return 0;
}

int user_rem(struct user_list_t *user_list, int sfd_rem)
{
    struct user_t *p_prev=NULL;
    for (struct user_t *p=user_list->first; p!=NULL ; p=p->next)
    {
        /* I found the user */
        if ((p->sfd)==sfd_rem)
        {
            /* take care about previous user in the list */
            if (p_prev!=NULL)
            {
                p_prev->next=p->next;
            }

            /* take care about user.next in the list */
            if (p_prev!=NULL)
            {
                p_prev->next=p->next;
            }
            /* when remove the first user I need also take care about user_list.first */
            else
            {
                user_list->first=p->next;
            }
            /* when remove the last user I need also take care about user_list.last */
            if ((p->next)==NULL)
            {
                user_list->last=p_prev;
            }

            free(p);
            return 0;
        }
        p_prev=p;
    }
    return -1;
}

//print
void user_print(struct user_list_t *user_list, int fd)
{
    char buf[80];
    write(fd,"\n User name \t socket \t IP Address \t Authorized \n", sizeof("\n User name \t socket \t IP Address \t Authorized \n"));
    for (struct user_t *p=user_list->first; p!=NULL ; p=p->next)
    {
        memset(buf,0, sizeof(buf));
        sprintf(buf, ".  %8s \t %8d \t %s\t %d \n",p->name, p->sfd, p->ip_address, p->auth);
        write(fd, buf, strlen(buf)+1);
    }
    printf("END OF LIST \n");
}

struct user_t *user_find_sfd(struct user_list_t *user_list, int i)
{
    for (struct user_t *p=user_list->first; p!=NULL ; p=p->next)
    {
        if ((p->sfd)==i)
        {
            return p;
        }
    }
    return NULL;
}

int user_message(struct user_list_t *user_list, fd_set *main_fds_p, char buf[], int server_fd, int i, int nfds)
{
    int ret=user_message_parse(user_list, buf, i);
    /* we already took care about message*/
    if (ret==-1)
        return 0;

    /* send to socket ret */
    if (ret>0)
    {
        char buf2[MAXBUF+MAXNAME+sizeof(space)];
        memset(buf2, 0, sizeof(buf2));

        /*name of the sending client*/
        struct user_t *p=user_find_sfd(user_list, i);
        sprintf(buf2,"%s%s%s", p->name, space, buf);

        int nbytes=strlen(buf2);
        if ((FD_ISSET(ret, main_fds_p)) && (ret!=server_fd))
        {
             if (send(ret, buf2, nbytes, 0) == -1)
                printf("send() error %d",ret);
        }
        return 0;
    }

    /* send to everybody */
    if (ret==0)
    {
        char buf2[MAXBUF+MAXNAME+sizeof(space)];
        memset(buf2, 0, sizeof(buf2));

        /*name of the sending client*/
        struct user_t *p=user_find_sfd(user_list, i);
        sprintf(buf2,"%s%s%s", p->name, space, buf);

        int nbytes=strlen(buf2);
        for (int j = 0; j<=nfds; j++)
        {
            if ((FD_ISSET(j, main_fds_p)) && (j!=server_fd) && (j!=i))
            {
                if (send(j, buf2, nbytes, 0) == -1)
                printf("send() error %d",j);
            }
        }
        return 0;
    }
    /* something went wrong, we don't know ret*/
    return -1;
}

/* -1 don't continue, it's done  */
int user_message_parse(struct user_list_t *user_list, char *buf, int fromsfd)
{
    /* socket fd to whom the message should be send */
    char *str_p;
    /* did user wrote command? */
    if (buf[0]=='!')
    {
        /* list users connected to server */
        if (strcmp(buf,"!ls\n")==0)
        {
            user_print(user_list, fromsfd);
        }

        if (strcmp(buf,"!logout\n")==0)
        {

        }
        return -1;
    }
    else
    if ((str_p = strstr(buf, str2))!=NULL)
    {
        /* length of nick*/
        int length=str_p-buf;
        char name[MAXNAME+1];
        memset(name,0,sizeof(name));
        for (int i=0;i<length;i++)
        {
            name[i]=buf[i];
        }
        /* name match */

        struct user_t *p;
        if ((p=user_find_name(user_list, name))!=NULL)
        {
            return p->sfd;
        }
        else
        /* no name match */
        {
            /* the message is for all users */
            if (strcmp(name,"all")==0)
            {
                return 0;
            }
            else
                return -2;
        }
    }
    return -2;
}

int user_auth_true(struct user_t *user)
{
    return user->auth;
}

struct user_t *user_find_name(struct user_list_t *user_list, char *name_loc)
{
    for (struct user_t *p=user_list->first; p!=NULL ; p=p->next)
    {
        if (strcmp(p->name, name_loc)==0)
        {
            return p;
        }
    }
    return NULL;
}
