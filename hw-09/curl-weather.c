#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <curl/curl.h>
#include "jsmn.h"


char JSON[256 * 1024];
size_t JSON_location = 0;

struct weather {
	int feelslike;
	int humidity;
	double precip;
	int pressure;
	int temperature;
	int max_temperature;
	int min_temperature;
	int wind_speed;
	char wind_dir[128];
	char cityname[128];
};

int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
	    strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 1;
	}
	
	return 0;
}


//parse JSON to get weather and cityname (to check input)
int parse_json(struct weather *wth)
{
	jsmn_parser p;
	jsmntok_t t[4096];
	char tmp_str[128];

	jsmn_init(&p);
	int r = jsmn_parse(&p, JSON, strlen(JSON), t, 4096);
	
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		
		return -1;
	}
	
	for (int i = 0; i < 64; i++) {
		//printf("%d - type = %d\t", i, t[i].type);

		if (jsoneq(JSON, &t[i], "FeelsLikeC")) {
			strncpy(tmp_str, JSON + t[i+1].start, t[i+1].end - t[i+1].start);
			tmp_str[t[i+1].end - t[i+1].start] = '\0';
			wth->feelslike = atoi(tmp_str);
		}
		else if (jsoneq(JSON, &t[i], "humidity")) {
			strncpy(tmp_str, JSON + t[i+1].start, t[i+1].end - t[i+1].start);
			tmp_str[t[i+1].end - t[i+1].start] = '\0';
			wth->humidity = atoi(tmp_str);
		}
		else if (jsoneq(JSON, &t[i], "precipMM")) {
			strncpy(tmp_str, JSON + t[i+1].start, t[i+1].end - t[i+1].start);
			tmp_str[t[i+1].end - t[i+1].start] = '\0';
			wth->precip = atof(tmp_str);
		}
		else if (jsoneq(JSON, &t[i], "pressureInches")) {
			strncpy(tmp_str, JSON + t[i+1].start, t[i+1].end - t[i+1].start);
			tmp_str[t[i+1].end - t[i+1].start] = '\0';
			wth->pressure = (int) (atof(tmp_str) * 25.4);
		}
		else if (jsoneq(JSON, &t[i], "temp_C")) {
			strncpy(tmp_str, JSON + t[i+1].start, t[i+1].end - t[i+1].start);
			tmp_str[t[i+1].end - t[i+1].start] = '\0';
			wth->temperature = atoi(tmp_str);
		}
		else if (jsoneq(JSON, &t[i], "windspeedKmph")) {
			strncpy(tmp_str, JSON + t[i+1].start, t[i+1].end - t[i+1].start);
			tmp_str[t[i+1].end - t[i+1].start] = '\0';
			wth->wind_speed = atoi(tmp_str);
		}
		else if (jsoneq(JSON, &t[i], "winddir16Point")) {
			strncpy(wth->wind_dir, JSON + t[i+1].start,
				(t[i+1].end - t[i+1].start) < 128 ? t[i+1].end - t[i+1].start : 128);
			if ((t[i+4].end - t[i+1].start) < 128) {
				wth->wind_dir[t[i+1].end - t[i+1].start] = '\0';
			}
			else {
				wth->wind_dir[127] = '\0';
			}
		}
		else if (jsoneq(JSON, &t[i], "areaName")) {
			strncpy(wth->cityname, JSON + t[i+4].start,
				(t[i+4].end - t[i+4].start) < 128 ? t[i+4].end - t[i+4].start : 128);
			if ((t[i+4].end - t[i+4].start) < 128) {
				wth->cityname[t[i+4].end - t[i+4].start] = '\0';
			}
			else {
				wth->cityname[127] = '\0';
			}
		}
	}

	for (int i = r; i > r-20; i--) {
		if (jsoneq(JSON, &t[i], "maxtempC")) {
			strncpy(tmp_str, JSON + t[i+1].start, t[i+1].end - t[i+1].start);
			tmp_str[t[i+1].end - t[i+1].start] = '\0';
			wth->max_temperature = atoi(tmp_str);
		}
		else if (jsoneq(JSON, &t[i], "mintempC")) {
			strncpy(tmp_str, JSON + t[i+1].start, t[i+1].end - t[i+1].start);
			tmp_str[t[i+1].end - t[i+1].start] = '\0';
			wth->min_temperature = atoi(tmp_str);
		}
	}
	
	return 0;
}

size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{	
	strncpy(JSON + JSON_location, ptr, size * nmemb);
	JSON_location += size * nmemb;
	
	return size * nmemb;
}

void print_weather(struct weather *wth)
{
	setlocale(LC_CTYPE, "");
	wint_t deg = 0x00b0;
	
	printf("air temperature is %d%lc C (min %d%lc C, max %d%lc C)",
	       wth->temperature, deg,
	       wth->min_temperature, deg,
	       wth->max_temperature, deg);
	printf(", feels like %d%lc C. ", wth->feelslike, deg);
	printf("Air humidity is %d %%. \n", wth->humidity);
	printf("Pressure is %d mmHg. ", wth->pressure);
	printf("Wind speed is %d km/h, direction %s. ", wth->wind_speed, wth->wind_dir);
	if (wth->precip < 0.01) {
		printf("Precipitations are not expected.\n");
	}
	else if(wth->precip < 1.0) {
		printf("Light precipitations are expected.\n");
	}
	else {
		printf("Heavy precipitations are expected.\n");
	}
}


int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("You should specify city name as second parameter for the program: \n");
		printf("curl-weather CITYNAME\n");
		
		return EXIT_SUCCESS;
	}
	
	CURL *curl = NULL;
	CURLcode res;
	char url[128];  
	snprintf(url, 128, "https://wttr.in/%s?format=j1", argv[1]); 
	
	res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res) {
		fprintf(stderr, "Error in global init curl library\n");
		
		return EXIT_FAILURE;
	}
 
	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "Error in easy init curl library\n");
		curl_global_cleanup();
		
		return EXIT_FAILURE;
	}
	
	if(curl) {
		res = curl_easy_setopt(curl, CURLOPT_CAINFO, "./wttr-in-chain.pem");
		if (res != CURLE_OK) {
			fprintf(stderr, "Error in set sertificate\n");
			
			curl_easy_cleanup(curl);
			curl_global_cleanup();
		}
		
		res = curl_easy_setopt(curl, CURLOPT_URL, url);
		if (res != CURLE_OK) {
			fprintf(stderr, "Error in set url\n");
			
			curl_easy_cleanup(curl);
			curl_global_cleanup();
		}

		res = curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
		if (res != CURLE_OK) {
			fprintf(stderr, "Error in set http get request\n");
			
			curl_easy_cleanup(curl);
			curl_global_cleanup();
		}
		
		res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
		if (res != CURLE_OK) {
			fprintf(stderr, "Error in set write callback function\n");
			
			curl_easy_cleanup(curl);
			curl_global_cleanup();
		}

		res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
		if (res != CURLE_OK) {
			fprintf(stderr, "Error in write data\n");
			
			curl_easy_cleanup(curl);
			curl_global_cleanup();
		}

#ifdef DEBUG
		res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
 
		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		if(res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		}
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	if (res != CURLE_OK) {
		return EXIT_FAILURE;
	}
	
	JSON[JSON_location+1] = '\0';
	struct weather wth;
	if (parse_json(&wth)) {
		return EXIT_FAILURE;
	}

	if (strncmp(argv[1], wth.cityname, strlen(argv[1]))) {
		printf("Error in cityname. Did you mean %s?\n", wth.cityname);

		return EXIT_SUCCESS;
	}
	
	printf("The weather in %s: ", argv[1]);
	print_weather(&wth);
	
	return EXIT_SUCCESS;
}
