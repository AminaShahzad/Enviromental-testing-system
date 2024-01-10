#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
#include "cjson/cJSON.h"
#include <string.h>

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

void parse_hourly_data(cJSON *hourArray, const char *city_name) {
    cJSON *hour = NULL;
    cJSON_ArrayForEach(hour, hourArray) {
         const char *time = cJSON_GetObjectItem(hour, "time")->valuestring;
        double temp_c = cJSON_GetObjectItem(hour, "temp_c")->valuedouble;
        double temp_f = cJSON_GetObjectItem(hour, "temp_f")->valuedouble;
        double humidity = cJSON_GetObjectItem(hour, "humidity")->valuedouble;
        double wind_mph = cJSON_GetObjectItem(hour, "wind_mph")->valuedouble;
        double wind_kph = cJSON_GetObjectItem(hour, "wind_kph")->valuedouble;
        const char *wind_dir = cJSON_GetObjectItem(hour, "wind_dir")->valuestring;
        double feelslike_c = cJSON_GetObjectItem(hour, "feelslike_c")->valuedouble;

        // Print or use the extracted data as needed
        printf("Time: %s\n Temperature (C): %.2fC\n Temperature (F): %.2fF\n Humidity: %.2f\n Wind_mph: %.3f\n Wind_kph: %.3f\n Wind_dir: %s\n", time, temp_c, temp_f, humidity, wind_mph, wind_kph,  	wind_dir);
        printf(" Feels Like: %.2fC\n", feelslike_c);
        
	// Save formatted data into a file based on the city name
        char formatted_data[512];
        snprintf(formatted_data, sizeof(formatted_data), "Time: %s\n Temperature (C): %.2fC\n Temperature (F): %.2fF\n Humidity: %.2f\n Wind_mph: %.3f\n Wind_kph: %.3f\n Wind_dir: %s\n", time, temp_c, temp_f, humidity, wind_mph, wind_kph, wind_dir);
        snprintf(formatted_data + strlen(formatted_data), sizeof(formatted_data) - strlen(formatted_data), " Feels Like: %.2fC\n\n\n\n", feelslike_c);
        
        
	char file_name[256];
    	snprintf(file_name, sizeof(file_name), "%s_data.txt", city_name);
        FILE *forg = fopen(file_name, "ab");
        if (!forg) {
            fprintf(stderr, "Failed to open file for writing.\n");
            return;
        }

        fprintf(forg, "%s", formatted_data);
        fclose(forg); 
    }
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
    char *city_name = "karachi";
    CURL *hnd = curl_easy_init();

    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(hnd, CURLOPT_URL, "http://api.weatherapi.com/v1/forecast.json?key=e6ff049c207841f883e205341232412&q=karachi&days=3&aqi=no&alerts=no");

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

    // Parse JSON response
    FILE *fo = fopen("api_response.txt", "rb");
    if (!fo) {
        fprintf(stderr, "Failed to open file for reading.\n");
        return 1;
    }

    fseek(fo, 0, SEEK_END);
    long file_size = ftell(fo);
    fseek(fo, 0, SEEK_SET);

    char *json_data = (char *)malloc(file_size + 1);
    fread(json_data, 1, file_size, fo);
    json_data[file_size] = '\0';

    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return 1;
    }

    cJSON *forecast = cJSON_GetObjectItem(root, "forecast");
    if (!forecast) {
        fprintf(stderr, "Failed to get 'forecast' object.\n");
        cJSON_Delete(root);
        return 1;
    }

    // Parse and print the complete forecast data
    parse_forecast_data(forecast, city_name);

    // Clean up cJSON and close the file
    cJSON_Delete(root);
    fclose(fo);
    free(json_data);

    return 0;
}

