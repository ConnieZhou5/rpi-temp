#include <pthread.h>

// Function to send alert
void *sendAlert(void *arg) {
    char *alertMessage = (char *)arg;
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3000/alert");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // Set timeout to 5 seconds
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, alertMessage);

        // Debug logging for CURL
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("Alert successfully sent to server: %s\n", alertMessage);
        }

        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Failed to initialize CURL.\n");
    }

    free(alertMessage); // Free the dynamically allocated memory
    return NULL;
}

// In the main loop, trigger alert in a non-blocking manner
if (fabs(x_acc) > 2 || fabs(y_acc) > 2 || fabs(z_acc) > 3) {
    printf("Fall detected. Sending alert to server...\n");

    // Prepare the alert message
    char *alertMessage = malloc(256);
    if (alertMessage) {
        snprintf(alertMessage, 256, "{\"time\":\"%.3f\"}", t);
    }

    // Create a thread to send the alert
    pthread_t alertThread;
    pthread_create(&alertThread, NULL, sendAlert, alertMessage);

    // Detach the thread to allow it to clean up after itself
    pthread_detach(alertThread);
}
