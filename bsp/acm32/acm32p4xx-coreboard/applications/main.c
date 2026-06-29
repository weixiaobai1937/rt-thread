#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/dev_pin.h>
#include "board.h"

#define LED_PIN  PA0

int main(void)
{
    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);
    while (1)
    {
        rt_pin_write(LED_PIN, PIN_LOW);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
    }
    return 0;
}
