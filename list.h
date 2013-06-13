#define MAXNAME 16
struct user_t
{
    char name[MAXNAME];
    int sfd;
    char ip_address[16];
    bool auth;
    struct user_t *next;
};

struct user_list_t
{
    int num;
    struct user_t *first;
    struct user_t *last;
};
