#include <time.h>


time_t time(time_t* arg)
{
    //todo: implement
    time_t time = 1429986348;

    if (arg)
        *arg = time;

    return time;
}
