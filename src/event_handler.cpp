#include "event_handler.h"
#include <errno.h>
#include <limits>
#include <assert.h>
#include <setjmp.h>
#include <cstring>

using namespace std;

static long long LL_MAX = numeric_limits<long long>::max();
static sigjmp_buf jmp_buf_env;

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
	ll_time=struct_time->tv_sec*1000;
	ll_time+=struct_time->tv_nsec/1000000;
	return ll_time;
}

static void do_signal_events(int signum)
{
	sigset_t old_signal_mask;
	sigemptyset(&old_signal_mask);
	//set new sigmask and save old sigmask
	if(sigprocmask(SIG_SETMASK, &(event_handler::get_signal_mask()), &old_signal_mask) == -1)
	{
		perror("sigprocmask");
		exit(EXIT_FAILURE);
	}
	
	const vector<event_handler::signal_event_t> &signal_events = event_handler::get_signal_events();
	for(vector<event_handler::signal_event_t>::const_iterator citer = signal_events.cbegin(); 
		citer != signal_events.cend(); ++citer)
	{
		if(citer->d_signum == signum)
		{
			citer->d_pevent->do_signal_event(signum);
		}
	}
	//restore old sigmask
	if(sigprocmask(SIG_SETMASK, &old_signal_mask, NULL) == -1)
	{
		perror("sigprocmask");
		exit(EXIT_FAILURE);
	}
	//goto event_handler::start();
	siglongjmp(jmp_buf_env, 1);
}

std::list<event_handler::timer_event_t> event_handler::d_soft_timer_events;
size_t event_handler::d_soft_timer_events_size;
std::vector<event_handler::signal_event_t> event_handler::d_signal_events;
std::set<int> event_handler::d_signal_set;
sigset_t event_handler::d_signal_mask;
const char *event_handler::hints[] = {
	"success.",

	"err_sys_start.(If you see it, you may use ERR_SYS_START badly)",
	"signal",
	"err_sys_end.(If you see it, you may use ERR_SYS_END badly)",

	"unvalid parameters.",
	"end.(If you see it, you may use ERR_END badly)"
};


int event_handler::init()
{
	d_soft_timer_events_size = 0;
	sigemptyset(&d_signal_mask);
}

int event_handler::destroy()
{
	//free soft timer events
	for(std::list<timer_event_t>::iterator iter = d_soft_timer_events.begin(); iter != d_soft_timer_events.end(); ++iter)
	{
		assert(iter->d_pevent != NULL);
		delete iter->d_pevent;
	}

	//free signal events
	for(std::vector<signal_event_t>::iterator iter = d_signal_events.begin(); iter != d_signal_events.end(); ++iter)
	{
		assert(iter->d_pevent != NULL);
		delete iter->d_pevent;
	}
	return 0;
}

int event_handler::add_soft_timer_event(long long begin, long long interval, event_handler::base_timer_event *pevent)
{
	if(begin < 0 || interval < 0 || pevent == NULL)
	{
		return event_handler::ERR_INVAL;
	}

	timer_event_t event;
	event.d_begin = begin;
	event.d_interval = interval;
	event.d_pevent = pevent;
	//++d_soft_timer_events_size in insert_timer_event()
	insert_timer_event(event);
	return event_handler::SUCCESS;
}

int event_handler::add_signal_event(int signum, base_signal_event *pevent)
{
	if(signum < 1 || signum > 31 || pevent == NULL)
	{
		return event_handler::ERR_INVAL;
	}
	signal_event_t event;
	event.d_signum = signum;
	event.d_pevent = pevent;
	d_signal_events.push_back(event);
	d_signal_set.insert(signum);
	sigaddset(&d_signal_mask, signum);
	return event_handler::SUCCESS;
}

int event_handler::start()
{
	struct timespec req_sleep_time, rem_sleep_time;
	bool have_run = false;
	ll_to_timespec(0, &req_sleep_time);
	ll_to_timespec(0, &rem_sleep_time);

	//for register signal functions
	for(set<int>::const_iterator citer = d_signal_set.cbegin(); citer != d_signal_set.cend(); ++citer)
	{
		if(signal(*citer, do_signal_events) == SIG_ERR)
		{
			return event_handler::ERR_SYS_SIGNAL;
		}
	}

	//from do_signal_events
	sigsetjmp(jmp_buf_env, 1);

	while(1)
	{
		long long ret = timespec_to_ll(&req_sleep_time) - timespec_to_ll(&rem_sleep_time);
		//get request sleep time
		ret = get_soft_timer_min_msec(ret, have_run);
		if(ret == LL_MAX)
		{
			break;
		}

		//printf("need sleep %lld ms\n", ret);
	
		//for sleeping, and clear rem_sleep_time
		if(nanosleep(ll_to_timespec(ret, &req_sleep_time), ll_to_timespec(0, &rem_sleep_time)) == -1)
		{
			if(errno != EINTR)
			{
				perror("[event_handler::start]:nanosleep");
				exit(EXIT_FAILURE);
			}
		}
		//printf("remainder sleep %lld ms\n", timespec_to_ll(&rem_sleep_time));
	}
}

const char *event_handler::return_hints(int ret)
{
	if(ret < event_handler::SUCCESS || ret > event_handler::ERR_END)
	{
		return NULL;
	}

	if(ret > ERR_SYS_START && ret < ERR_SYS_END)
	{
		static char buf[128];
		snprintf(buf, sizeof(buf),  "%s: %s\n", hints[ret], strerror(errno));
		return buf;
	}
	return hints[ret];
}

long long event_handler::get_soft_timer_min_msec(long long have_sleep_msec, bool &hava_run)
{
	assert(have_sleep_msec >= 0);
	if(d_soft_timer_events_size == 0)
	{
		return LL_MAX;
	}

	list<timer_event_t> free_list;
	for(list<timer_event_t>::iterator iter = d_soft_timer_events.begin(); iter != d_soft_timer_events.end();)
	{
		iter->d_begin -= have_sleep_msec;
		//the timer is alarm
		if(iter->d_begin <= 0)
		{
			{
				sigset_t old_signal_mask;
				sigemptyset(&old_signal_mask);
				//set new sigmask and save old sigmask
				if(sigprocmask(SIG_SETMASK, &(event_handler::get_signal_mask()), &old_signal_mask) == -1)
				{
					perror("sigprocmask");
					exit(EXIT_FAILURE);
				}
				//do timer event function
				iter->d_pevent->do_timer_event();
				//restore old sigmask
				if(sigprocmask(SIG_SETMASK, &old_signal_mask, NULL) == -1)
				{
					perror("sigprocmask");
					exit(EXIT_FAILURE);
				}
			}
			timer_event_t &tmp = *iter;
			//move alarm event to free_list
			if(tmp.d_interval != 0)
			{
				free_list.push_back(tmp);
			}
			iter = d_soft_timer_events.erase(iter);
			--d_soft_timer_events_size;
		}
		else
		{
			++iter;
		}
	}
	
	//repush timer event  to d_soft_timer_events
	for(list<timer_event_t>::iterator iter = free_list.begin(); iter != free_list.end();)
	{
		timer_event_t &tmp = *iter;
		tmp.d_begin = tmp.d_interval;
		insert_timer_event(tmp);
		iter = free_list.erase(iter);
	}
	return d_soft_timer_events_size > 0 ? d_soft_timer_events.front().d_begin : LL_MAX;
}

list<event_handler::timer_event_t>::iterator event_handler::insert_timer_event(event_handler::timer_event_t &event)
{
	list<timer_event_t>::iterator iter;
	for(iter = d_soft_timer_events.begin(); iter != d_soft_timer_events.end(); ++iter)
	{
		if(iter->d_begin >= event.d_begin)
		{
			iter = d_soft_timer_events.insert(iter, event);
			break;
		}
	}
	if(iter == d_soft_timer_events.end())
	{
		iter = d_soft_timer_events.insert(iter, event);
	}
	++d_soft_timer_events_size;
	return iter;
}
