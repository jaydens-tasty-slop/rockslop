/* The USB HID keyboard emulation of the original is not available here: no
 * USB device on this target, so the class is a no-op (isUSBConfigured() is
 * false and the KEYBOARD setting has no effect). */
#include "FakeKeyboard.h"

FakeKeyboard::FakeKeyboard() {}
void FakeKeyboard::reset(uint8_t, uint8_t, uint8_t) {}
void FakeKeyboard::activated(void) {}
void FakeKeyboard::sendLetter(char) {}
