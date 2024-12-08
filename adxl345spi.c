#include <unistd.h>  // For fork()

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




if (fabs(x_acc) > 2 || fabs(y_acc) > 2 || fabs(z_acc) > 3) {
    printf("Fall detected. Sending alert to server...\n");

    // Prepare the alert message
    char alertMessage[256];
    snprintf(alertMessage, sizeof(alertMessage), "{\"message\":\"Fall detected at %.3f seconds\"}", t);

    // Send alert using a subprocess
    sendAlertSubprocess(alertMessage);
}
