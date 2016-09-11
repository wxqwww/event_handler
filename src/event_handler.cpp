#include "event_handler.h"
#include <cstdio>
#include <cstdlib>
#include <errno.h>
using namespace std;

inline
struct timespec *ll_to_timespec(long long ll_time, struct timespec *struct_time)
{
	assert(struct_time && ll_time >= 0);
	struct_time->tv_sec = ll_time / 1000;
	struct_time->tv_nsec = ll_time % 1000 * 1000000;
	return struct_time;
}

inline
long long timespec_to_ll(struct timespec *struct_time)
{
	assert(struct_time);
	long long ll_time;
	ll_time=time.tv_sec*1000;
	ll_time+=time.tv_nsec/1000000;
	return ll_time;
}

static jmp_buf jmp_buf_env;

static void do_signal_events(int signum)
{

	siglongjmp(jmp_buf_env, 1);
}

int event_handler::start()
{
	struct timespec req_sleep_time, rem_sleep_time;
	bool have_run = false;
	ll_to_timespec(0, rem_sleep_time);

	sigsetjmp(jmp_buf_env, 1);
	while(1)
	{
		long long ret;
		ret = get_soft_timer_min_msec(timespec_to_ll(&rem_sleep_time), have_run);
		if(nanosleep(ll_to_timespec(ret, &req_sleep_time), &rem_sleep_time) == -1)
		{
			if(errno != EINTR)
			{
				perror("[event_handler::start]:nanosleep");
				exit(EXIT_FAILURE);
			}
		}
	}
}



