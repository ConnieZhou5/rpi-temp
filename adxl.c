#include <stdio.h>
#include <pigpio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <libwebsockets.h>

#define DATA_FORMAT   0x31  // data format register address
#define DATA_FORMAT_B 0x0B  // data format bytes: +/- 16g range, 13-bit resolution
#define READ_BIT      0x80
#define MULTI_BIT     0x40
#define BW_RATE       0x2C
#define POWER_CTL     0x2D
#define DATAX0        0x32
#define ALERT_THRESHOLD 1.5  // Threshold for acceleration alert
#define WEBSOCKET_SERVER_IP "172.20.10.9"  // Replace with your WebSocket server IP
#define WEBSOCKET_SERVER_PORT 3000

const double accConversion = 2 * 16.0 / 8192.0;  // +/- 16g range, 13-bit resolution

// Global WebSocket variables
struct lws_context *context;
struct lws *wsi;

// WebSocket protocols
static const struct lws_protocols protocols[] = {
    { "default", NULL, 0, 0 },
    { NULL, NULL, 0, 0 }
};

// Utility function to send messages via WebSocket
void sendWebSocketAlert(const char *message) {
    if (wsi) {
        unsigned char buf[LWS_PRE + 256];
        size_t len = snprintf((char *)&buf[LWS_PRE], 256, "%s", message);
        lws_write(wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
    } else {
        printf("WebSocket not connected. Unable to send alert.\n");
    }
}

// Function to read bytes from the SPI interface
int readBytes(int handle, char *data, int count) {
    data[0] |= READ_BIT;
    if (count > 1) data[0] |= MULTI_BIT;
    return spiXfer(handle, data, data, count);
}

// Function to write bytes to the SPI interface
int writeBytes(int handle, char *data, int count) {
    if (count > 1) data[0] |= MULTI_BIT;
    return spiWrite(handle, data, count);
}

int main(int argc, char *argv[]) {
    int h, bytes;
    char data[7];
    int16_t x, y, z;
    double tStart, t;
    
    // Initialize GPIO
    if (gpioInitialise() < 0) {
        printf("Failed to initialize GPIO!\n");
        return 1;
    }

    // Initialize SPI
    h = spiOpen(0, 2000000, 3);
    if (h < 0) {
        printf("Failed to open SPI interface!\n");
        gpioTerminate();
        return 1;
    }

    // Initialize ADXL345 sensor
    data[0] = BW_RATE;
    data[1] = 0x0F;
    writeBytes(h, data, 2);
    data[0] = DATA_FORMAT;
    data[1] = DATA_FORMAT_B;
    writeBytes(h, data, 2);
    data[0] = POWER_CTL;
    data[1] = 0x08;
    writeBytes(h, data, 2);

    // Initialize WebSocket connection
    struct lws_context_creation_info info = {0};
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    context = lws_create_context(&info);
    if (!context) {
        printf("Failed to create WebSocket context\n");
        spiClose(h);
        gpioTerminate();
        return 1;
    }

    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = context;
    ccinfo.address = WEBSOCKET_SERVER_IP;
    ccinfo.port = WEBSOCKET_SERVER_PORT;
    ccinfo.path = "/";
    ccinfo.protocol = protocols[0].name;

    wsi = lws_client_connect_via_info(&ccinfo);
    if (!wsi) {
        printf("Failed to connect to WebSocket server\n");
        lws_context_destroy(context);
        spiClose(h);
        gpioTerminate();
        return 1;
    }

    // Reading loop
    printf("Starting accelerometer data reading...\n");
    tStart = time_time();
    while (1) {
        data[0] = DATAX0;
        bytes = readBytes(h, data, 7);
        if (bytes == 7) {
            x = (data[2] << 8) | data[1];
            y = (data[4] << 8) | data[3];
            z = (data[6] << 8) | data[5];

            double accX = x * accConversion;
            double accY = y * accConversion;
            double accZ = z * accConversion;

            t = time_time() - tStart;
            printf("time = %.3f, x = %.3f, y = %.3f, z = %.3f\n", t, accX, accY, accZ);

            // Check for threshold exceedance
            if (fabs(accX) > ALERT_THRESHOLD || fabs(accY) > ALERT_THRESHOLD || fabs(accZ) > ALERT_THRESHOLD) {
                char message[256];
                snprintf(message, sizeof(message), "{\"type\": \"alert\", \"message\": \"Acceleration exceeded: x=%.3f, y=%.3f, z=%.3f\"}", accX, accY, accZ);
                sendWebSocketAlert(message);
            }
        } else {
            printf("Error reading from accelerometer!\n");
        }

        time_sleep(0.1);  // Adjust sleep time as per your desired sampling rate
    }

    // Cleanup
    lws_context_destroy(context);
    spiClose(h);
    gpioTerminate();

    return 0;
}
