#include "../device.h"
#include "../utility.h"
#include "logitech.h"

#include <math.h>
#include <string.h>

static struct device device_gprox_wireless;

static const uint16_t PRODUCT_ID = 0x0aba;

static int gprox_wireless_send_sidetone(hid_device* device_handle, uint8_t num);
static int gprox_wireless_request_battery(hid_device* device_handle);

void gprox_wireless_init(struct device** device)
{
    device_gprox_wireless.idVendor = VENDOR_LOGITECH;
    device_gprox_wireless.idProductsSupported = &PRODUCT_ID;
    device_gprox_wireless.numIdProducts = 1;

    strncpy(device_gprox_wireless.device_name, "Logitech G PRO Series", sizeof(device_gprox_wireless.device_name));

    device_gprox_wireless.capabilities = CAP_SIDETONE | CAP_BATTERY_STATUS;
    device_gprox_wireless.send_sidetone = &gprox_wireless_send_sidetone;
    device_gprox_wireless.request_battery = &gprox_wireless_request_battery;

    *device = &device_gprox_wireless;
}

static int gprox_wireless_send_sidetone(hid_device* device_handle, uint8_t num)
{
    num = map(num, 0, 128, 0, 100);

    uint8_t sidetone_data[HIDPP_LONG_MESSAGE_LENGTH] = { HIDPP_LONG_MESSAGE, HIDPP_DEVICE_RECEIVER, 0x05, 0x1a, num };

    return hid_write(device_handle, sidetone_data, sizeof(sidetone_data) / sizeof(sidetone_data[0]));
}

static int gprox_wireless_request_battery(hid_device* device_handle)
{
    int r = 0;
    // request battery voltage
    uint8_t data_request[HIDPP_LONG_MESSAGE_LENGTH] = { HIDPP_LONG_MESSAGE, HIDPP_DEVICE_RECEIVER, 0x06, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    r = hid_write(device_handle, data_request, sizeof(data_request) / sizeof(data_request[0]));
    if (r < 0)
        return r;

    uint8_t data_read[7];
    r = hid_read(device_handle, data_read, 7);
    if (r < 0)
        return r;

    //6th byte is state; 0x1 for idle, 0x3 for charging
    uint8_t state = data_read[6];
    if (state == 0x03)
        return BATTERY_CHARGING;

    // actual voltage is byte 4 and byte 5 combined together
    const uint16_t voltage = (data_read[4] << 8) | data_read[5];

    if (voltage <= 3525)
        return (int)((0.03 * voltage) - 101);
    if (voltage > 4030)
        return 100;

    // f(x)=3.7268473047*10^(-9)x^(4)-0.00005605626214573775*x^(3)+0.3156051902814949*x^(2)-788.0937250298629*x+736315.3077118985
    return (int)(0.0000000037268473047 * pow(voltage, 4) - 0.00005605626214573775 * pow(voltage, 3) + 0.3156051902814949 * pow(voltage, 2) - 788.0937250298629 * voltage + 736315.3077118985);
}
