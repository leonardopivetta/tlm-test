#include "main.h"

int main() {
	state_t cur_state = STATE_UNINITIALIZED;
	SharedData sd;
	do {
		sd.currentState = cur_state;
		cur_state = run_state(cur_state, &sd);
		if(cur_state == STATE_ERROR) {
			sleep(1);
		}
	} while(1);
	return 0;
}