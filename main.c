#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
#include "cjson/cJSON.h"
#include <string.h>
#include "email_sender.h"

// Data structure to store daily averages
struct DailyAverages {
    double temperature;
    double humidity;
    double wind_kph;
    double wind_mph;
    double feels_like;
    int count;  // Number of data points for the day
};

// Function to initialize daily averages
void initDailyAverages(struct DailyAverages *daily) {
    daily->temperature = 0.0;
    daily->humidity = 0.0;
    daily->wind_kph = 0.0;
    daily->wind_mph = 0.0;
    daily->feels_like = 0.0;
    daily->count = 0;
}

// Function to update daily averages
void updateDailyAverages(struct DailyAverages *daily, double temp, double hum, double wind_kph, double wind_mph, double feels_like) {
    daily->temperature += temp;
    daily->humidity += hum;
    daily->wind_kph += wind_kph;
    daily->wind_mph += wind_mph;
    daily->feels_like += feels_like;
    daily->count++;
}

// Function to calculate and print daily averages
void printDailyAverages(const struct DailyAverages *daily, const char *date) {
    if (daily->count > 0) {
        printf("Date: %s\n", date);
        printf("Average Temperature: %.2fC\n", daily->temperature / daily->count);
        printf("Average Humidity: %.2f\n", daily->humidity / daily->count);
        printf("Average Wind_kph: %.2f\n", daily->wind_kph / daily->count);
        printf("Average Wind_mph: %.2f\n", daily->wind_mph / daily->count);
        printf("Average Feels Like: %.2fC\n", daily->feels_like / daily->count);
        printf("\n");
        FILE *finalize = fopen("Report_File", "ab");
    	if (!finalize) {
        	fprintf(stderr, "Failed to open file for writing.\n");
        	return;
    }
    	fprintf(finalize,"Date: %s\n", date);
        fprintf(finalize,"Average Temperature: %.2fC\n", daily->temperature / daily->count);
        fprintf(finalize,"Average Humidity: %.2f\n", daily->humidity / daily->count);
        fprintf(finalize,"Average Wind_kph: %.2f\n", daily->wind_kph / daily->count);
        fprintf(finalize,"Average Wind_mph: %.2f\n", daily->wind_mph / daily->count);
        fprintf(finalize,"Average Feels Like: %.2fC\n", daily->feels_like / daily->count);
        fprintf(finalize,"\n");
         

}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

void parse_hourly_data(cJSON *hourArray, const char *city_name) {
    
    cJSON *hour = NULL;
    struct DailyAverages daily;
    char current_date[11] = "";  // Format: YYYY-MM-DD

    initDailyAverages(&daily);

    cJSON_ArrayForEach(hour, hourArray) {
        const char *time = cJSON_GetObjectItem(hour, "time")->valuestring;
        const char *date = time ? time : "N/A";
        if (strncmp(date, current_date, 10) != 0) {
            // Print previous day's averages
            printDailyAverages(&daily, current_date);
            
            // Start calculating averages for the new day
            initDailyAverages(&daily);
            strncpy(current_date, date, 10);
        }

        double temp_c = cJSON_GetObjectItem(hour, "temp_c")->valuedouble;
        double humidity = cJSON_GetObjectItem(hour, "humidity")->valuedouble;
        double wind_mph = cJSON_GetObjectItem(hour, "wind_mph")->valuedouble;
        double wind_kph = cJSON_GetObjectItem(hour, "wind_kph")->valuedouble;
        double feelslike_c = cJSON_GetObjectItem(hour, "feelslike_c")->valuedouble;
           
           
           
         // comparing and saving in report file
        

        // Update daily averages
        updateDailyAverages(&daily, temp_c, humidity, wind_kph, wind_mph, feelslike_c);

        // Save formatted data into a file to my_data.txt
        char file_name[256];
    	snprintf(file_name, sizeof(file_name), "my_data2.txt");
    	FILE *forg = fopen(file_name, "ab");
    	if (!forg) {
        	fprintf(stderr, "Failed to open file for writing.\n");
        	return;
    }

        fprintf(forg, "Time: %s\n Temperature (C): %.2fC\n Humidity: %.2f\n Wind_mph: %.3f\n Wind_kph: %.3f\n Feels Like: %.2fC\n\n", time, temp_c, humidity, wind_mph, wind_kph, feelslike_c);
        fclose(forg);
    }

    // Print averages for the last day
     FILE *finalize = fopen("Report_File", "wb");
    	if (!finalize) {
        	fprintf(stderr, "Failed to open file for writing.\n");
        	return;
    }
    printDailyAverages(&daily, current_date);
fclose(finalize);
   
}

void parse_forecast_data(cJSON *forecast, const char *city_name) {
    cJSON *forecastday = cJSON_GetObjectItem(forecast, "forecastday");
    cJSON *day = NULL;
    cJSON_ArrayForEach(day, forecastday) {
        cJSON *hourArray = cJSON_GetObjectItem(day, "hour");
        if (hourArray) {
            // Parse and print hourly data for each day
            parse_hourly_data(hourArray, city_name);
        }
    }
}




int main() {
    // Ask the user to input the city name
    char city_name[50];  // Adjust the size based on your needs
    printf("Enter your city name: ");
    scanf("%s", city_name);

    // Ask the user to input the number of days
    int number_of_days;
    printf("Enter the number of days: ");
    scanf("%d", &number_of_days);

    // Construct the API URL
    char url[256];  // Adjust the size based on your needs
    snprintf(url, sizeof(url), "https://api.weatherapi.com/v1/forecast.json?key=e6ff049c207841f883e205341232412&q=%s&days=%d&aqi=no&alerts=no", city_name, number_of_days);

    CURL *hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(hnd, CURLOPT_URL, url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "X-RapidAPI-Key: e6ff049c207841f883e205341232412");
    headers = curl_slist_append(headers, "X-RapidAPI-Host: api.weatherapi.com");

    FILE *fp = fopen("api_response.txt", "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file for writing.\n");
        return 1;
    }
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, fp);

    CURLcode ret = curl_easy_perform(hnd);

    // Clean up resources
    curl_slist_free_all(headers);
    curl_easy_cleanup(hnd);
    fclose(fp);

    if (ret != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));
        return 1;
    }
    // To reset the file
    char file_name[256];
    snprintf(file_name, sizeof(file_name), "my_data.txt");
    FILE *forg = fopen(file_name, "wb");
    if (!forg) {
        fprintf(stderr, "Failed to open file for writing.\n");
        return 1;
    }

    // Parse JSON response
    fp = fopen("api_response.txt", "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open file for reading.\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *json_data = (char *)malloc(file_size + 1);
    fread(json_data, 1, file_size, fp);
    json_data[file_size] = '\0';

    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }

        // Clean up cJSON and close the file
        cJSON_Delete(root);
        fclose(fp);
        free(json_data);

        return 1;
    }

    cJSON *forecast = cJSON_GetObjectItem(root, "forecast");
    if (!forecast) {
        fprintf(stderr, "Failed to get 'forecast' object.\n");

        // Clean up cJSON and close the file
        cJSON_Delete(root);
        fclose(fp);
        free(json_data);

        return 1;
    }

    // Parse and print the complete forecast data
    parse_forecast_data(forecast, city_name);

    // Clean up cJSON and close the file
    cJSON_Delete(root);
    fclose(fp);
    free(json_data);

   send_email();

    return 0;
}
