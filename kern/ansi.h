
#ifndef _ANSI_H_
#define _ANSI_H_
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

#define ANSI_MAX_VALUES 4

typedef enum { 
	STATE_INIT,
	STATE_ESC,
	STATE_BRACE,
	STATE_SAVE,
	STATE_RESTORE,
	STATE_CLEAR_1,
	STATE_CLEAR_2,
	STATE_ERASE_LINE,
	STATE_DIGIT,
	STATE_CUR_UP,
	STATE_CUR_DOWN,
	STATE_CUR_FORWARD,
	STATE_CUR_BACKWARD,
	STATE_COLOR,
	STATE_SEMI,
	NUM_STATES
} state_t;

typedef struct {
	state_t state;
	int data;
	int save;
	uint8_t values[ANSI_MAX_VALUES];
	uint16_t index;
	uint16_t n_values;
} ansi_data_t;

typedef state_t state_func_t(char data);

void ansi_init(void);
bool ansi_parse(char);

#endif /* _ANSI_H_ */
