/*NO LEGAL*/

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void DoChild();

int main()
{
    pid_t pid = fork();
    if (pid != 0)
        waitpid(pid, 0, 0);
    else
        DoChild();
    return 0;
}


int Glob = 0;

static void DoChild()
{
    Glob++;
}
