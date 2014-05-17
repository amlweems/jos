
#include <kern/ansi.h>
#include <kern/console.h>

static ansi_data_t ansi_data;

static void
ansi_inc(uint8_t n)
{
	uint8_t *v = &ansi_data.values[ansi_data.index];
	*v = (*v)*10 + n;
}

static void
ansi_color(uint8_t c)
{
	if (c == 0) {
		ansi_data.data = 0x0700;
	} else if (c == 5) {
		ansi_data.data |= 0x8000;
	} else if (c >= 30 && c <= 37) {
		ansi_data.data = (ansi_data.data & ~0x0700) | (c - 30) << 8;
	} else if (c >= 40 && c <= 47) {
		ansi_data.data = (ansi_data.data & ~0x7000) | (c - 40) << 12;
	}
}

static state_t
do_state_init(char data)
{
	ansi_data.index = 0;
	ansi_data.n_values = 0;
	uint8_t i;
	for (i = 0; i < ANSI_MAX_VALUES; i++) {
		ansi_data.values[i] = 0;
	}
	switch (data) {
	case 0x1b:
		return STATE_ESC;
	default:
		return STATE_INIT;
	}
}

static state_t
do_state_esc(char data)
{
	switch (data) {
	case '[':
		return STATE_BRACE;
	default:
		return STATE_INIT;
	}
}

static state_t
do_state_brace(char data)
{
	switch (data) {
	case '2':
		ansi_inc(2);
		ansi_data.n_values++;
		return STATE_CLEAR_2;
	case '0':
	case '1':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		ansi_inc(data & 0xf);
		ansi_data.n_values++;
		return STATE_DIGIT;
	case 'K':
		return STATE_ERASE_LINE;
	case 'u':
		return STATE_RESTORE;
	case 's':
		return STATE_SAVE;
	default:
		return STATE_INIT;
	}
}

static state_t
do_state_save(char data)
{
	ansi_data.save = ansi_data.data;
	return STATE_INIT;
}

static state_t
do_state_restore(char data)
{
	ansi_data.data = ansi_data.save;
	return STATE_INIT;
}

static state_t
do_state_clear_1(char data)
{
	switch (data) {
	case 'J':
		return STATE_CLEAR_2;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		ansi_inc(data & 0xf);
		return STATE_DIGIT;
	case ';':
		return STATE_SEMI;
	default:
		return STATE_INIT;
	}
}

static state_t
do_state_clear_2(char data)
{
	cga_clear_screen();
	return STATE_INIT;
}

static state_t
do_state_erase_line(char data)
{
	cga_clear_line();
	return STATE_INIT;
}

static state_t
do_state_digit(char data)
{
	switch (data) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		ansi_inc(data & 0xf);
		return STATE_DIGIT;
	case ';':
		return STATE_SEMI;
	case 'A':
		return STATE_CUR_UP;
	case 'B':
		return STATE_CUR_DOWN;
	case 'C':
		return STATE_CUR_FORWARD;
	case 'D':
		return STATE_CUR_BACKWARD;
	case 'm':
		return STATE_COLOR;
	default:
		return STATE_INIT;
	}
}

static state_t
do_state_cur_up(char data)
{
	if (crt_pos > CRT_COLS) {
		crt_pos -= CRT_COLS;
	}
	return STATE_INIT;
}

static state_t
do_state_cur_down(char data)
{
	if (crt_pos < CRT_SIZE - CRT_COLS) {
		crt_pos += CRT_COLS;
	}
	return STATE_INIT;
}

static state_t
do_state_cur_forward(char data)
{
	if (crt_pos < CRT_COLS) {
		crt_pos++;
	}
	return STATE_INIT;
}

static state_t
do_state_cur_backward(char data)
{
	if (crt_pos > 0) {
		crt_pos--;
	}
	return STATE_INIT;
}

static state_t
do_state_color(char data)
{
	uint16_t i;
	for (i = 0; i < ansi_data.n_values; i++) {
		ansi_color(ansi_data.values[i]);
	}
	return STATE_INIT;
}

static state_t
do_state_semi(char data)
{
	if (ansi_data.index < ANSI_MAX_VALUES) {
		ansi_data.index++;
		if (ansi_data.n_values < ANSI_MAX_VALUES) {
			ansi_data.n_values++;
		}
	} else {
		ansi_data.index = 0;
	}
	switch (data) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		ansi_inc(data & 0xf);
		return STATE_DIGIT;
	case 'm':
		return STATE_COLOR;
	default:
		return STATE_INIT;
	}
}

static state_func_t* const state_table[ NUM_STATES ] = {
	do_state_init,
	do_state_esc,
	do_state_brace,
	do_state_save,
	do_state_restore,
	do_state_clear_1,
	do_state_clear_2,
	do_state_erase_line,
	do_state_digit,
	do_state_cur_up,
	do_state_cur_down,
	do_state_cur_forward,
	do_state_cur_backward,
	do_state_color,
	do_state_semi
};

static state_t run_state(state_t cur_state, char data) {
    return state_table[cur_state](data);
};

void
ansi_init(void)
{
	ansi_data.data = 0x0700;
	ansi_data.save = 0x0700;
	ansi_data.index = 0;
	ansi_data.n_values = 0;
}

bool
ansi_parse(char c)
{
	ansi_data.state = run_state(ansi_data.state, c);
	crt_attr = ansi_data.data;
	return (ansi_data.state == STATE_INIT);
}
