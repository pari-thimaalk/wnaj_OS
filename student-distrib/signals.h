#include "term.h"

void* sig_handlers[5];
void signal_init();
void send_signal(int signum);
