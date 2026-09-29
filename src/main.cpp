#include <Arduino.h>
#include "App.h"

App app;

void setup()
{
    Serial.begin(115200);
    delay(500);

    app.begin();
}

void loop()
{
    app.tick();
}