curl-weather: curl-weather.c jsmn.h wttr-in-chain.pem
	gcc -o curl-weather curl-weather.c -lcurl -Wall -Wextra -Wpedantic -std=c11 

sanitize: curl-weather.c
	gcc -o curl-weather curl-weather.c -lcurl -fsanitize=undefined

clean:
	rm -rf curl-weather
