#include <includes.h>

extern DATA_TO_CONTROL dataToControl;

void setup()
{
    Serial.begin(115200);

    ios.setup();
    protocol_setup();
}

void loop()
{
    ios.writeDataBag(&dataToControl);

    delay(WRITE_DATA_BAG_INTERVAL_MS);
    // Serial.println("First");
    // ios.setAuswahlX(HIGH);
    // delay(1000);
    // Serial.println("Second");
    // ios.setAuswahlX(LOW);
    // delay(1000);

    // for (int i = 0; i < 1023; i++)
    // {
    //   uint64_t timeBefore = micros();
    //   dac_set_all_channel(i);
    //   uint64_t timeAfter = micros();

    //   Serial.print("Time in micros taken: ");
    //   Serial.println(timeAfter-timeBefore);

    //   delay(100);
    // }
    // for (int i = 0; i < 1023; i++)
    // {
    //   dac_set_all_channel(1023-i);
    //   delay(1);
    // }
}