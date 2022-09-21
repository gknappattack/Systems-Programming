#include<stdio.h>
#include<string.h>

#define HTTP_REQUEST_MAX_SIZE 4096
#define HOSTNAME_MAX_SIZE 512
#define PORT_MAX_SIZE 6
#define URI_MAX_SIZE 4096
#define METHOD_SIZE 32

/* 
 * Determine whether or not the HTTP request contained in the request argument
 * is complete in the sense that the request has been received in its entirety.
 * Hint: this can be as simple as seeing if the request ends with the
 * end-of-headers character sequence.
 *
 * Input: request, a string containing an HTTP request.  Note that the way this
 *        function is currently defined, request must be a null-terminated
 *        string. As such, you can use string functions (strcp(), strstr(),
 *        strlen()) on request.  Alternatively (i.e., not null-terminated), you
 *        would need to modify the function definition to additionally pass in
 *        a length.
 * Output: 1 if request is a complete HTTP request, 0 otherwise
 * */
int is_complete_request(const char *request) {
	return 0;
}

/* Parse an HTTP request, and copy each parsed value into the
 * appropriate string argument as a null-terminated string (remember the null
 * terminator!). Call is_complete_request() first to make sure the client has
 * sent the full request.
 * Input: method, hostname, port, uri - strings to which the
 *        corresponding parts parsed from the request should be
 *        copied, as strings.  The uri is the "path" part of the requested URL,
 *        including query string. If no port is specified, the port string
 *        should be populated with a string specifying the default HTTP port,
 *        i.e., "80".
 * Output: 1 if request is a complete HTTP request, 0 otherwise
 * */
int parse_request(const char *request, char *method,
		char *hostname, char *port, char *uri) {
	return 0;
}

int main() {
	char *req1;
	char *req2;
	char *req3;

	char method[METHOD_SIZE];
	char hostname[HOSTNAME_MAX_SIZE];
	char port[PORT_MAX_SIZE];
	char uri[URI_MAX_SIZE];

       	req1 = "GET http://www.example.com/index.html HTTP/1.0\r\n"
		"Host: www.example.com\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n";

       	req2 = "GET http://www.example.com:8080/index.html?foo=1&bar=2 HTTP/1.0\r\n"
		"Host: www.example.com:8080\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n";

       	req3 = "GET http://www.example.com:8080/index.html HTTP/1.0\r\n";
	
	if (parse_request(req1, method, hostname, port, uri)) {
		printf("req1 method: %s\n", method);
		printf("req1 hostname: %s\n", hostname);
		printf("req1 port: %s\n", port);
	} else {
		printf("req1 is incomplete\n");
	}

	if (parse_request(req2, method, hostname, port, uri)) {
		printf("req2 method: %s\n", method);
		printf("req2 hostname: %s\n", hostname);
		printf("req2 port: %s\n", port);
	} else {
		printf("req2 is incomplete\n");
	}

	if (parse_request(req3, method, hostname, port, uri)) {
		printf("req3 method: %s\n", method);
		printf("req3 hostname: %s\n", hostname);
		printf("req3 port: %s\n", port);
	} else {
		printf("req3 is incomplete\n");
	}

}
