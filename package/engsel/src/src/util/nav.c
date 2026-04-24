#include "util/nav.h"

int g_nav_goto_main = 0;

void nav_trigger_goto_main(void) { g_nav_goto_main = 1; }
void nav_reset(void)             { g_nav_goto_main = 0; }
int  nav_should_return(void)     { return g_nav_goto_main; }
