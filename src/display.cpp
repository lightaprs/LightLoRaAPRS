#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include "display.h"
#include "variant.h"
#include "logger.h"

extern logging::Logger logger;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST_PIN);

void setup_Display() {

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Display", "SSD1306 Display init...");

  if(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Setup Display...");  
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(1);
    display.display();
    delay(500);
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Display", "SSD1306 Display init done!");     
  } else {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Display", "SSD1306 allocation failed");
  }
    
}

void display_toggle(bool toggle) {
  if (toggle) {
    display.ssd1306_command(SSD1306_DISPLAYON);
  } else {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
}

void show_display_print(String line1, int wait, int textSize) {
  display.setTextSize(textSize);
  display.print(line1);
  display.display();  
  delay(wait);
}
void show_display_println(String line1, int wait, int textSize) {
  display.setTextSize(textSize);
  display.println(line1);
  display.display();  
  delay(wait);
}

void show_display(String line1, int wait, int textSize) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(textSize);
  display.setCursor(0, 0);
  display.println(line1);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}

void show_display(String line1, String line2, int wait) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 8);
  display.println(line2);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}

void show_display(String line1, String line2, String line3, int wait) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 8);
  display.println(line2);
  display.setCursor(0, 16);
  display.println(line3);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}

void show_display(String line1, String line2, String line3, String line4, int wait) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 8);
  display.println(line2);
  display.setCursor(0, 16);
  display.println(line3);
  display.setCursor(0, 24);
  display.println(line4);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}

void show_display(String line1, String line2, String line3, String line4, String line5, int wait) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 8);
  display.println(line2);
  display.setCursor(0, 16);
  display.println(line3);
  display.setCursor(0, 24);
  display.println(line4);
  display.setCursor(0, 32);
  display.println(line5);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}

void show_display(String line1, String line2, String line3, String line4, String line5, String line6, int wait) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 8);
  display.println(line2);
  display.setCursor(0, 16);
  display.println(line3);
  display.setCursor(0, 24);
  display.println(line4);
  display.setCursor(0, 32);
  display.println(line5);
  display.setCursor(0, 40);
  display.println(line6);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}

void show_display(String line1, String line2, String line3, String line4, String line5, String line6, String line7, int wait) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(line1);
  display.setTextSize(1);
  display.setCursor(0, 16);
  display.println(line2);
  display.setCursor(0, 24);
  display.println(line3);
  display.setCursor(0, 32);
  display.println(line4);
  display.setCursor(0, 40);
  display.println(line5);
  display.setCursor(0, 48);
  display.println(line6);
  display.setCursor(0, 56);
  display.println(line7);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}

void show_display_two_lines_big_header(String line1, String line2, int wait) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(line1);
  display.setTextSize(1);
  display.setCursor(0, 17);
  display.println(line2);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}

void show_display_six_lines_big_header(String line1, String line2, String line3, String line4, String line5, String line6, int wait) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(line1);
  display.setTextSize(1);
  display.setCursor(0, 17);
  display.println(line2);
  display.setCursor(0, 27);
  display.println(line3);
  display.setCursor(0, 37);
  display.println(line4);
  display.setCursor(0, 47);
  display.println(line5);
  display.setCursor(0, 57);
  display.println(line6);  
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1);
  display.display();
  delay(wait);
}