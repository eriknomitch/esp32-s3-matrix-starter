#pragma once

#include <Frame.h>

void displayBegin();

// Push a frame to the LEDs, applying rotation and the brightness cap.
void displayShow(const Frame& f);
