#include "event_handler.h"
#include <cstdio>

using namespace std;

class timer_event_print : public event_handler::base_timer_event, public event_handler::base_signal_event
{
public:
	timer_event_print(const string &name, int number): 
		event_handler::base_timer_event(name), event_handler::base_signal_event(name), d_number(number){}
	virtual ~timer_event_print(){}
	virtual int do_timer_event();
	virtual int do_signal_event(int signum);
private:
	int d_number;
};

int timer_event_print::do_timer_event()
{
	printf("%s %d use to timer\n", this->event_handler::base_timer_event::get_name().c_str(), d_number);
	return 0;
}

int timer_event_print::do_signal_event(int signum)
{
	printf("%s %d use to signal\n", this->event_handler::base_signal_event::get_name().c_str(), d_number);
	return 0;
}

int main()
{
	timer_event_print *p1 = new timer_event_print("p1", 1);
	timer_event_print *p2 = new timer_event_print("p2", 2);
	timer_event_print *p3 = new timer_event_print("p3", 3);
	
	event_handler::init();
	event_handler::add_soft_timer_event(1000, 1000, p1);
	event_handler::add_soft_timer_event(2000, 2000, p2);
	event_handler::add_soft_timer_event(3000, 3000, p3);
	event_handler::add_signal_event(SIGINT, p1);
	event_handler::add_signal_event(SIGINT, p2);
	event_handler::add_signal_event(SIGINT, p3);
	event_handler::start();
	event_handler::destroy();
	return 0;
}
