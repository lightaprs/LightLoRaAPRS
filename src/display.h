#ifndef DISPLAY_H_
#define DISPLAY_H_

void setup_Display();

void display_clear();
void display_toggle(bool toggle);
void show_display_print(String line1, int wait = 0, int textSize = 1);
void show_display_println(String line1, int wait = 0, int textSize = 1);
void show_display(String line1, int wait = 0, int textSize = 1);
void show_display(String line1, String line2, int wait = 0);
void show_display(String line1, String line2, String line3, int wait = 0);
void show_display(String line1, String line2, String line3, String line4, int wait = 0);
void show_display(String line1, String line2, String line3, String line4, String line5, int wait = 0);
void show_display(String line1, String line2, String line3, String line4, String line5, String line6, int wait = 0);
void show_display(String line1, String line2, String line3, String line4, String line5, String line6, String line7, int wait = 0);
void show_display_two_lines_big_header(String line1, String line2, int wait = 0);
void show_display_three_lines_big_header(String line1, String line2, String line3, int wait = 0);
void show_display_six_lines_big_header(String line1, String line2, String line3, String line4, String line5, String line6, int wait = 0);

#endif