#ifndef _EVENT_HANDLER_H
#define _EVENT_HANDLER_H

#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <string>
#include <list>
#include <vector>
#include <set>

class event_handler
{
public:
	class base_timer_event
	{
	public:
		base_timer_event(const std::string &name): d_name("timer_" + name)
		virtual ~base_timer_event(){}
		//how to do timer event
		virtual do_timer_event() = 0;
		const std::string &get_name();
	private:
		//name of event
		const std::sring d_timer_name;
	};
	
	class base_signal_event
	{
	public:
		base_signal_event(const std::string &name): d_name("signal_" + name)
		virtual ~base_signal_event(){}
		//how to do signal event
		virtual do_signal_event(int signum) = 0;
		const std::string &get_name();
	private:
		//name of event
		const std::sring d_signal_name;

	}

private:
	//timer accuracy is millisecond
	struct timer_event_t
	{
		//d_begin is relative time in soft timer, and is absolute timer in hard timer.
		long long d_begin;
		//always relative time
		long long d_interval;
		//which event do timer event
		base_timer_event *d_pevent;
	};

	struct signal_event_t
	{
		//number of signal, reference to <signal.h>
		int signum;
		//which event do signal event
		base_signal_event *d_pevent;
	};
public:
	//if interval == 0, it is one shot.
	//static int add_hard_timer_event(long long begin, long long interval, base_event *pevent);
	static int add_soft_timer_event(long long begin, long long interval, base_event *pevent);
	static int add_signal_event(int signum, base_event *pevent);
	static int start();
private:
	//static long long get_current_msec();
	//static long long get_hard_timer_min_msec(long long &curent_msec);
	static long long get_soft_timer_min_msec(long long have_sleep_msec, bool &hava_run);
	
	//static std::list<base_signal_event *> d_hard_timer_events;
	static std::list<base_timer_event *> d_soft_timer_events;
#if __cplusplus < 201103L
	//size() of "list" is linear before c++11, so we should save them.
	//static size_t d_hard_timer_events_size;
	static size_t d_soft_timer_events_size;
#endif //#if __cplusplus < 201103L
	static std::vector<base_signal_event *> d_signal_events;
	static std::set<int> d_signal_set;
};

/*
inline
long long event_handler::get_current_msec()
{
	struct timespec time;
	if(clock_gettime(CLOCK_REALTIME,&time)==-1)
	{   
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}   
	long long msec;
	msec=time.tv_sec*1000;
	msec+=time.tv_nsec/1000000;
	return msec;
}
*/

#endif //_EVENT_HANDLER_H
