#include "logger.h"
#include "button.h"
#include "variant.h"
#include "OneButton.h"

extern logging::Logger logger;
OneButton btn;
static bool buttonPressed = false;

void IRAM_ATTR checkTicks() {
  // keep watching the push button:
  btn.tick();
}

void setup_Button()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  btn = OneButton(
  BUTTON_PIN, // Input pin for the button
  LOW,       // Button is active LOW
  true       // Enable internal pull-up resistor
  );
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), checkTicks, CHANGE);
  btn.attachClick(pressStart);
}

void pressStart() {
    buttonPressed = true;
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "BUTTON", "Button pressed");
} // pressStart

bool isButtonPressed(){
    if(buttonPressed) {
       buttonPressed = false; 
       return true; 
    } else {
       return false;
    }    
}

