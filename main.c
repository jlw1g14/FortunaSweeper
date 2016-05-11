/*

Author: Jordan West (jlw1g14)
Licence: This work is licensed under the Creative Commons Attribution-ShareAlike 3.0.
View this license at https://creativecommons.org/licenses/by-sa/3.0/

This code works off of libraries developed by other authors.
* Steve Gunn's code to drive the display under the lib/lcd folder (adapted by Klaus-Peter Zauner).
* Peter Dannegger's code to read switch input under the lib/routa folder (adapted by Klaus-Peter Zauner).
* Frank Vahid, Bailey Miller, and Tony Givargis's code for the lib/rios under the rios folder (adapted by Klaus-Peter Zauner).

Developed for the following hardware:
* avr90usb1286 MCU
* 240x320 LCD with ILI9341 driver

*/

#include <stdlib.h>
#include "os.h"
#include "color.h"

#define ROWS 9
#define COLS 9
#define BOMBS 10
#define BOMB 9

volatile int board[ROWS][COLS];
volatile uint16_t player_row = 0;
volatile uint16_t player_col = 0;
volatile int gameover = 1;
volatile int win = 0;
volatile int seed_set = 0;
volatile int flag_mode = 0;
volatile int flags_left = BOMBS;
volatile uint16_t blocksize = 240 / 9;

void init_board() {
	int i, j;
	
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			board[i][j] = 10;
		}
	}
}

void increment_tile(int row, int col) {
	if (row < 0 || row >= ROWS) return;
	if (col < 0 || col >= COLS) return;
	if (board[row][col] % 10 == BOMB) return;
	
	board[row][col]++;
}

void place_bombs() {
	int n = 0;
	
	while (n < BOMBS) {
		int row = rand() % ROWS;
		int col = rand() % COLS;
		
		if (board[row][col] % 10 != BOMB) {
			board[row][col] = BOMB + 10;
			increment_tile(row - 1, col - 1);
			increment_tile(row - 1, col);
			increment_tile(row - 1, col + 1);
			increment_tile(row + 1, col - 1);
			increment_tile(row + 1, col);
			increment_tile(row + 1, col + 1);
			increment_tile(row, col - 1);
			increment_tile(row, col + 1);
		}
		
		n++;
	}
}

int reveal_board(int row, int col) {
	if (row < 0 || row >= ROWS) return 0;
	if (col < 0 || col >= COLS) return 0;
	if (board[row][col] < 10) return 0;
	
	int i, j;
	
	if (board[row][col] % 10 == BOMB) {
		for (i = 0; i < ROWS; i++) {
			for (j = 0; j < COLS; j++) {
				board[i][j] %= 10;
			}
		}
		
		return 1;
	} else {
		if (board[row][col] >= 20) {
			flags_left += 1;
		}
		
		board[row][col] %= 10;
		
		if (board[row][col] % 10 == 0) {
			reveal_board(row - 1, col);
			reveal_board(row + 1, col);
			reveal_board(row, col - 1);
			reveal_board(row, col + 1);
		}
	}
	
	return 0;
}

uint16_t map_to_color(int k) {
	if (k >= 10 && k < 20) {
		return DARK_SLATE_GRAY;
	}
	
	if (k >= 20 && k < 30) {
		return RED;
	}
	
	switch (k) {
		case 0 :
			return LIGHT_CYAN;
			break;
		case 1 :
			return BLUE;
			break;
		case 2 :
			return GREEN;
			break;
		case 3 :
			return RED;
			break;
		case 4 :
			return BLUE;
			break;
		case 5 :
			return GREEN;
			break;
		case 6 :
			return RED;
			break;
		case 7 :
			return BLUE;
			break;
		case 8 :
			return GREEN;
			break;
		case 9 :
			return BLACK;
			break;
	}
	
	return DARK_SLATE_GRAY;
}

void number_to_string(char* buffer, int value) {
	if (value >= 20 && value < 30) {
		*buffer = '!';
		return;
	}
	
	switch(value) {
		case 1 : *buffer = '1'; break;
		case 2 : *buffer = '2'; break;
		case 3 : *buffer = '3'; break;
		case 4 : *buffer = '4'; break;
		case 5 : *buffer = '5'; break;
		case 6 : *buffer = '6'; break;
		case 7 : *buffer = '7'; break;
		case 8 : *buffer = '8'; break;
		case 9 : *buffer = '*'; break;
	}
}

void print_tile(uint16_t row, uint16_t col) {
	int k = board[row][col];
	char ck[2] = " ";
	rectangle r = {col * blocksize + 1, col * blocksize + blocksize - 1, row * blocksize + 1, row * blocksize + blocksize - 1};
	
	if (win == 1) {
		number_to_string(ck, k % 10);
	} else {
		number_to_string(ck, k);
	}
	
	
	if (row == player_row && col == player_col) {
		if (flag_mode == 0) {
			fill_rectangle(r, WHITE);
		} else {
			fill_rectangle(r, RED);
		}
	} else {
		if (k > 9) {
			fill_rectangle(r, map_to_color(10));
			display.background = map_to_color(10);
		} else {
			fill_rectangle(r, map_to_color(0));
			display.background = map_to_color(0);
		}
		
		if (win == 1 && k % 10 == BOMB) {
			display.foreground = BLACK;
		} else {
			display.foreground = map_to_color(k);
		}
		
		display_string_xy(ck, (r.left + r.right) / 2, (r.top + r.bottom) / 2);
	}
	
	display.background = BLACK;
}

int check_win() {
	int i, j;
	
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (board[i][j] >= 10 && board[i][j] < 20 && board[i][j] % 10 != BOMB) {
				return 0;
			}
		}
	}
	
	return 1;
}

void print_board() {
	uint16_t i, j;
	
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {			
			print_tile(i, j);
		}
	}
}

void clear_text() {
	display_string_xy("              ", blocksize * COLS + 1, 1);
	display_string_xy("              ", blocksize * COLS + 1, 11);
	display_string_xy("              ", blocksize * COLS + 1, 21);
}

void update_flag_mode(int mode) {
	flag_mode = mode;
	
	display.foreground = WHITE;
	
	display_string_xy("              ", blocksize * COLS + 1, 11);
	
	if (mode == 1) {
		display_string_xy("FLAG MODE", blocksize * COLS + 1, 11);
	} else {
		display_string_xy("REVEAL MODE", blocksize * COLS + 1, 11);
	}
}

void update_flag_display() {
	display.foreground = WHITE;
	
	display_string_xy("              ", blocksize * COLS + 1, 1);
	
	char str[30];
	sprintf(str, "Flags left: %d", flags_left);
	display_string_xy(str, blocksize * COLS + 1, 1);
}

void start_new_game() {
	clear_screen();
	init_board();
	place_bombs();
	gameover = 0;
	win = 0;
	flags_left = 10;
	print_board();
	update_flag_display();
	update_flag_mode(0);
}

void place_flag() {
	if (board[player_row][player_col] >= 10) {
		if (board[player_row][player_col] >= 20) {
			board[player_row][player_col] %= 10;
			board[player_row][player_col] += 10;
			flags_left += 1;
			update_flag_display();
		} else {
			if (flags_left > 0) {
				board[player_row][player_col] %= 10;
				board[player_row][player_col] += 20;
				flags_left -= 1;
				update_flag_display();
			}
		}
	}
}

void check_for_gameover() {
	if (gameover != 0) {
		clear_text();
		display.foreground = WHITE;
		display_string_xy("GAME OVER!", blocksize * COLS + 1, 1);
		display_string_xy("Press to start", blocksize * COLS + 1, 11);
		display_string_xy("over!", blocksize * COLS + 1, 21);
	} else if (check_win() == 1) {
		clear_text();
		win = 1;
		gameover = 1;
		print_board();
		display.foreground = WHITE;
		display_string_xy("GAME WON!", blocksize * COLS + 1, 1);
		display_string_xy("Press to start", blocksize * COLS + 1, 11);
		display_string_xy("over!", blocksize * COLS + 1, 21);
	}
}

void perform_move(int move) {
	if (seed_set == 0) {
		if (move == 4) {
			srand(TCNT2);
			seed_set = 1;
			start_new_game();
		}
		
		return;
	}
	
	switch(move) {
		case 0 :
			if (player_row > 0) {
				uint16_t old_player_row = player_row;
				player_row--;
				
				print_tile(old_player_row, player_col);
				print_tile(player_row, player_col);
			}
			
			break;
		case 1 :
			if (player_col < (COLS - 1)) {
				uint16_t old_player_col = player_col;
				player_col++;
				
				print_tile(player_row, old_player_col);
				print_tile(player_row, player_col);
			}
			
			break;
		case 2 :
			if (player_row < (ROWS - 1)) {
				uint16_t old_player_row = player_row;
				player_row++;
				
				print_tile(old_player_row, player_col);
				print_tile(player_row, player_col);
			}
			
			break;
		case 3 :
			if (player_col > 0) {
				uint16_t old_player_col = player_col;
				player_col--;
				
				print_tile(player_row, old_player_col);
				print_tile(player_row, player_col);
			}
			
			break;
		case 4 :
			if (flag_mode == 0) {
				if (gameover == 0) {
					gameover = reveal_board(player_row, player_col);
					
					update_flag_display();
				
					print_board();
					
					check_for_gameover();
				} else {
					start_new_game();
				}
			} else {
				place_flag();
			}
			
			break;
		case 5 :
			if (flag_mode == 0) {
				update_flag_mode(1);
			} else {
				update_flag_mode(0);
			}
			
			print_board(player_row, player_col);
	}
}

int check_switches() {
	if (get_switch_press(_BV(SWN))) {
		perform_move(0);
	}
	
	if (get_switch_press(_BV(SWE))) {
		perform_move(1);
	}
	
	if (get_switch_press(_BV(SWS))) {
		perform_move(2);
	}
	
	if (get_switch_press(_BV(SWW))) {
		perform_move(3);
	}
	
	if (get_switch_press(_BV(SWC))) {
		perform_move(4);
	}
	
	int i;
	
	for (i = 0; i < 10; i++) {
		if (os_enc_delta() != 0) {
			perform_move(5);
			i = 10;
		}
	}
	
	
	return 0;
}

int main() {
	os_init();
	
	TCCR2B |= (1 << CS10);
	
	display.background = BLACK;
	
	display.foreground = YELLOW;
	display_string_xy("#FORTUNA SWEEPER#", 0, 1);
	display.foreground = WHITE;
	display_string_xy("Reveal all of the tiles without detonating any mines.", 0, 21);
	display_string_xy("A numbered tile indicates how many neighbouring tiles", 0, 31);
	display_string_xy("contain mines, where a neighbouring tile includes", 0, 41);
	display_string_xy("diagonals.", 0, 51);
	
	display.foreground = RED;
	display_string_xy("CONTROLS", 0, 71);
	display.foreground = WHITE;
	display_string_xy("To move your cursor, use the directional buttons.", 0, 91);
	display_string_xy("To uncover a tile, press the centre button.", 0, 101);
	display_string_xy("To switch between uncover and flag mode, turn the", 0, 111);
	display_string_xy("rotary encoder.", 0, 121);
	display_string_xy("When in flag mode, you can place flags where you", 0, 131);
	display_string_xy("think bombs are, instead of uncovering a tile.", 0, 141);
	
	display.foreground = CYAN;
	display_string_xy("Press the centre button to begin playing!", 0, 171);
	
	display.foreground = WHITE;
	
	os_add_task(check_switches, 100, 1);
	
	sei();
	
	while (1) {
		
	}
}
