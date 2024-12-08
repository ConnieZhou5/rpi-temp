#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pigpio.h>
#include <curl/curl.h>
#include <unistd.h> 
#include <signal.h>

// run: sudo adxl345spi 

// from adxl345 data sheet
#define DATA_FORMAT   0x31  
#define DATA_FORMAT_B 0x0B  
#define READ_BIT      0x80
#define MULTI_BIT     0x40
#define BW_RATE       0x2C
#define POWER_CTL     0x2D
#define DATAX0        0x32

const char codeVersion[4] = "0.3";  
const int timeDefault = 100;  
const int freqDefault = 2;  // default sampling rate of data stream, Hz
const int freqMax = 3200;  // maximal allowed cmdline arg sampling rate of data stream, Hz
const int speedSPI = 2000000;  // SPI communication speed, bps
const int freqMaxSPI = 100000;  // maximal possible communication sampling rate through SPI, Hz 
const int coldStartSamples = 2;  // number of samples to be read before outputting data to console (cold start delays)
const double coldStartDelay = 0.1;  // time delay between cold start reads
const double accConversion = 2 * 16.0 / 8192.0;  // +/- 16g range, 13-bit resolution
const double tStatusReport = 1;  // time period of status report if data read to file, seconds
volatile int keepRunning = 1;

// Low-pass filter function
double applyLowPassFilter(double currentValue, double previousValue, double alpha) {
    return alpha * currentValue + (1 - alpha) * previousValue;
}

int readBytes(int handle, char *data, int count) {
    data[0] |= READ_BIT;
    if (count > 1) data[0] |= MULTI_BIT;
    return spiXfer(handle, data, data, count);
}

int writeBytes(int handle, char *data, int count) {
    if (count > 1) data[0] |= MULTI_BIT;
    return spiWrite(handle, data, count);
}

// Function to send alert via a subprocess
void sendAlertSubprocess(const char *alertMessage) {
    pid_t pid = fork();
    if (pid == -1) {
        // Error handling
        perror("Failed to fork process");
        return;
    }

    if (pid == 0) {
        // Child process: Send alert
        CURL *curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3000/alert");
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // Set timeout to 5 seconds
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, alertMessage);

            // Perform the request
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            } else {
                printf("Alert successfully sent: %s\n", alertMessage);
            }

            curl_easy_cleanup(curl);
        } else {
            fprintf(stderr, "Failed to initialize CURL in subprocess.\n");
        }
        exit(0); // Terminate the child process after sending the alert
    }

    // Parent process continues execution
}

void handleInterrupt(int signal) {
    printf("\nInterrupt signal received. Exiting acc program.");
    keepRunning = 0;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handleInterrupt);
    int i;
    int bSave = 0;
    char vSave[256] = "";
    double vTime = timeDefault;
    double vFreq = freqDefault;
    double accelXFiltered = 0.0, accelYFiltered = 0.0, accelZFiltered = 0.0;
    double accelXPrev = 0.0, accelYPrev = 0.0, accelZPrev = 0.0;

    // SPI sensor setup
    int samples = vFreq * vTime;
    int samplesMaxSPI = freqMaxSPI * vTime;
    int success = 1;
    int h, bytes;
    char data[7];
    int16_t x, y, z;
    double tStart, tDuration, t;
    if (gpioInitialise() < 0) {
        printf("Failed to initialize GPIO!");
        return 1;
    }
    h = spiOpen(0, speedSPI, 3);
    data[0] = BW_RATE;
    data[1] = 0x0F;
    writeBytes(h, data, 2);
    data[0] = DATA_FORMAT;
    data[1] = DATA_FORMAT_B;
    writeBytes(h, data, 2);
    data[0] = POWER_CTL;
    data[1] = 0x08;
    writeBytes(h, data, 2);

    double delay = 1.0 / vFreq;


    if (bSave == 0) {
        // fake reads to eliminate cold start timing issues (~0.01 s shift of sampling time after the first reading)
        for (i = 0; i < coldStartSamples; i++) {
            data[0] = DATAX0;
            bytes = readBytes(h, data, 7);
            if (bytes != 7) {
                success = 0;
            }
            time_sleep(coldStartDelay);
        }
        // real reads happen here
        tStart = time_time();
        double nextReadTime = tStart;
        while (keepRunning) {
            // Wait until it's time for the next reading
            while (time_time() < nextReadTime) {
                time_sleep(0.001); // Small sleep to prevent CPU hogging
            }
            
            data[0] = DATAX0;
            bytes = readBytes(h, data, 7);
            if (bytes == 7) {
                x = (data[2]<<8)|data[1];
                y = (data[4]<<8)|data[3];
                z = (data[6]<<8)|data[5];
                t = time_time() - tStart;
                double x_acc = x * accConversion;
                double y_acc = y * accConversion;
                double z_acc = z * accConversion;

                // Apply low-pass filter
                accelXFiltered = applyLowPassFilter(x_acc, accelXPrev, 0.5);
                accelYFiltered = applyLowPassFilter(y_acc, accelYPrev, 0.5);
                accelZFiltered = applyLowPassFilter(z_acc, accelZPrev, 0.5);

                // Update previous values
                accelXPrev = accelXFiltered;
                accelYPrev = accelYFiltered;
                accelZPrev = accelZFiltered;

                printf("time = %.3f, x = %.3f, y = %.3f, z = %.3f\n",
                       t, fabs(x_acc), fabs(y_acc), fabs(z_acc));

                
                if (fabs(x_acc) > 2 || fabs(y_acc) > 2 || fabs(z_acc) > 3) {
                    printf("Fall detected. Sending alert to server...\n");

                    // Prepare the alert message
                    char alertMessage[256];
                    snprintf(alertMessage, sizeof(alertMessage), "{\"message\":\"Fall detected at %.3f seconds\"}", t);

                    // Send alert using a subprocess
                    sendAlertSubprocess(alertMessage);
                }
            }
            else {
                success = 0;
                fprintf(stderr, "Failed to initialze CURL.\n");
            }
            
            // Set the next read time
            nextReadTime += 0.5; // Schedule next reading in 0.5 seconds
        }
        gpioTerminate();
        tDuration = time_time() - tStart;  // need to update current time to give a closer estimate of sampling rate
        printf("%d samples read in %.2f seconds with sampling rate %.1f Hz\n", samples, tDuration, samples/tDuration);
        if (success == 0) {
            printf("Error occurred!");
            return 1;
        }
    }

    else {
        // reserve vectors for file-output arrays: time, x, y, z
        // arrays will not change their lengths, so separate track of the size is not needed
        double *at, *ax, *ay, *az;
        at = malloc(samples * sizeof(double));
        ax = malloc(samples * sizeof(double));
        ay = malloc(samples * sizeof(double));
        az = malloc(samples * sizeof(double));

        // reserve vectors for raw data: time, x, y, z
        // maximal achievable sampling rate depends from the hardware...
        // in my case, for Raspberry Pi 3 at 2 Mbps SPI bus speed sampling rate never exceeded ~30,000 Hz...
        // so to be sure that there is always enough memory allocated, freqMaxSPI is set to 60,000 Hz
        double *rt, *rx, *ry, *rz;
        rt = malloc(samplesMaxSPI * sizeof(double));
        rx = malloc(samplesMaxSPI * sizeof(double));
        ry = malloc(samplesMaxSPI * sizeof(double));
        rz = malloc(samplesMaxSPI * sizeof(double));

        printf("Reading %d samples in %.1f seconds with sampling rate %.1f Hz...\n", samples, vTime, vFreq);
        int statusReportedTimes = 0;
        double tCurrent, tClosest, tError, tLeft;
        int j, jClosest;

        tStart = time_time();
        int samplesRead;
        for (i = 0; i < samplesMaxSPI; i++) {
            data[0] = DATAX0;
            bytes = readBytes(h, data, 7);
            if (bytes == 7) {
                x = (data[2]<<8)|data[1];
                y = (data[4]<<8)|data[3];
                z = (data[6]<<8)|data[5];
                t = time_time();
                rx[i] = x * accConversion;
                ry[i] = y * accConversion;
                rz[i] = z * accConversion;
                rt[i] = t - tStart;
            }
            else {
                gpioTerminate();
                printf("Error occurred!");
                return 1;
            }
            tDuration = t - tStart;
            if (tDuration > tStatusReport * ((float)statusReportedTimes + 1.0)) {
                statusReportedTimes++;
                tLeft = vTime - tDuration;
                if (tLeft < 0) {
                    tLeft = 0.0;
                }
                printf("%.2f seconds left...\n", tLeft);
            }
            if (tDuration > vTime) {  // enough data read
                samplesRead = i;
                break;
            }
        }
        gpioTerminate();
        printf("Writing to the output file...\n");
        for (i = 0; i < samples; i++) {
            if (i == 0) {  // always get the first reading from position 0
                tCurrent = 0.0;
                jClosest = 0;
                tClosest = rt[jClosest];
            }
            else {
                tCurrent = (float)i * delay;
                tError = fabs(tClosest - tCurrent);
                for (j = jClosest; j < samplesRead; j++) {  // lookup starting from previous j value
                    if (fabs(rt[j] - tCurrent) <= tError) {  // in order to save some iterations
                        jClosest = j;
                        tClosest = rt[jClosest];
                        tError = fabs(tClosest - tCurrent);
                    }
                    else {
	                	break;  // break the loop since the minimal error point passed
                    }
                }  // when this loop is ended, jClosest and tClosest keep coordinates of the closest (j, t) point
            }
            ax[i] = rx[jClosest];
            ay[i] = ry[jClosest];
            az[i] = rz[jClosest];
            at[i] = tCurrent;
        }
        FILE *f;
        f = fopen(vSave, "w");
        fprintf(f, "time, x, y, z\n");
        for (i = 0; i < samples; i++) {
            fprintf(f, "%.5f, %.5f, %.5f, %.5f \n", at[i], ax[i], ay[i], az[i]);
        }
        fclose(f);
    }

    printf("Done\n");
    return 0;
}
