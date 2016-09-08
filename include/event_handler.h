#ifndef _EVENT_HANDLER_H
#define _EVENT_HANDLER_H

#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <signal.h>

class event_handler
{
public:
	class base_timer_event
	{
	public:
		base_timer_event(const std::string &name): d_name("timer_" + name){}
		virtual ~base_timer_event(){}
		//how to do timer event
		virtual int do_timer_event() = 0;
		const std::string &get_name();
		const std::string &get_name() const;
	private:
		//name of event
		const std::string d_name;
	};
	
	class base_signal_event
	{
	public:
		base_signal_event(const std::string &name): d_name("signal_" + name){}
		virtual ~base_signal_event(){}
		//how to do signal event
		virtual int do_signal_event(int signum) = 0;
		const std::string &get_name();
		const std::string &get_name() const;
	private:
		//name of event
		const std::string d_name;
	};

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
		int d_signum;
		//which event do signal event
		base_signal_event *d_pevent;
	};


	static int init();
	static int destroy();
	//if interval == 0, it is one shot.
	//static int add_hard_timer_event(long long begin, long long interval, base_event *pevent);
	static int add_soft_timer_event(long long begin, long long interval, base_timer_event *pevent);
	static int add_signal_event(int signum, base_signal_event *pevent);
	static int start();
	static const sigset_t &get_signal_mask();
	static const std::vector<signal_event_t> &get_signal_events();
	static const char *return_hints(int ret);
private:
	enum
	{
		SUCCESS = 0,
		//for system call error
		ERR_SYS_START,
		ERR_SYS_SIGNAL,
		ERR_SYS_END,
		
		//for other error
		ERR_INVAL,
		ERR_END
	};

	event_handler();
	//static long long get_current_msec();
	//static long long get_hard_timer_min_msec(long long &curent_msec);
	static long long get_soft_timer_min_msec(long long have_sleep_msec, bool &hava_run);
	static std::list<timer_event_t>::iterator insert_timer_event(timer_event_t &event);
	
	//timer events
	//static std::list<base_signal_event> d_hard_timer_events;
	static std::list<timer_event_t> d_soft_timer_events;
	//size() of "list" is linear before c++11, so we should save them.
	//static size_t d_hard_timer_events_size;
	static size_t d_soft_timer_events_size;

	//signal events
	static std::vector<signal_event_t> d_signal_events;
	static std::set<int> d_signal_set;
	static sigset_t d_signal_mask;
	
	static const char *hints[ERR_END - SUCCESS + 1];
};

inline
const std::string &event_handler::base_timer_event::get_name()
{
	return (static_cast<const event_handler::base_timer_event *>(this))->get_name();
}

inline
const std::string &event_handler::base_timer_event::get_name() const
{
	return d_name;
}

inline
const std::string &event_handler::base_signal_event::get_name()
{
	return (static_cast<const event_handler::base_signal_event *>(this))->get_name();
}

inline
const std::string &event_handler::base_signal_event::get_name() const
{
	return d_name;
}

inline 
const sigset_t &event_handler::get_signal_mask()
{
	return d_signal_mask;
}

inline 
const std::vector<event_handler::signal_event_t> &event_handler::get_signal_events()
{
	return d_signal_events;
}
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
