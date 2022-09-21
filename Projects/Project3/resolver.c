#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <unistd.h>
#include <netdb.h>

typedef unsigned int dns_rr_ttl;
typedef unsigned short dns_rr_type;
typedef unsigned short dns_rr_class;
typedef unsigned short dns_rdata_len;
typedef unsigned short dns_rr_count;
typedef unsigned short dns_query_id;
typedef unsigned short dns_flags;

typedef struct {
	char *name;
	dns_rr_type type;
	dns_rr_class class;
	dns_rr_ttl ttl;
	dns_rdata_len rdata_len;
	unsigned char *rdata;
} dns_rr;

typedef struct {
    unsigned short indentification;
    unsigned short flags;
    unsigned short questions;
    unsigned short arr;
    unsigned short authorityr1;
    unsigned short authorityr2;
} Dns_Query_Header;

struct dns_answer_entry;
struct dns_answer_entry {
	char *value;
	struct dns_answer_entry *next;
};
typedef struct dns_answer_entry dns_answer_entry;

void free_answer_entries(dns_answer_entry *ans) {
	dns_answer_entry *next;
	while (ans != NULL) {
		next = ans->next;
		free(ans->value);
		free(ans);
		ans = next;
	}
}

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;
	unsigned char c;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
}

void canonicalize_name(char *name) {
	/*
	 * Canonicalize name in place.  Change all upper-case characters to
	 * lower case and remove the trailing dot if there is any.  If the name
	 * passed is a single dot, "." (representing the root zone), then it
	 * should stay the same.
	 *
	 * INPUT:  name: the domain name that should be canonicalized in place
	 */
	
	int namelen, i;

	// leave the root zone alone
	if (strcmp(name, ".") == 0) {
		return;
	}

	namelen = strlen(name);
	// remove the trailing dot, if any
	if (name[namelen - 1] == '.') {
		name[namelen - 1] = '\0';
	}

	// make all upper-case letters lower case
	for (i = 0; i < namelen; i++) {
		if (name[i] >= 'A' && name[i] <= 'Z') {
			name[i] += 32;
		}
	}
}

int name_ascii_to_wire(char *name, unsigned char *wire) {
	/* 
	 * Convert a DNS name from string representation (dot-separated labels)
	 * to DNS wire format, using the provided byte array (wire).  Return
	 * the number of bytes used by the name in wire format.
	 *
	 * INPUT:  name: the string containing the domain name
	 * INPUT:  wire: a pointer to the array of bytes where the
	 *              wire-formatted name should be constructed
	 * OUTPUT: the length of the wire-formatted name.
	 */
}

char *name_ascii_from_wire(unsigned char *wire, int *indexp) {
	/* 
	 * Extract the wire-formatted DNS name at the offset specified by
	 * *indexp in the array of bytes provided (wire) and return its string
	 * representation (dot-separated labels) in a char array allocated for
	 * that purpose.  Update the value pointed to by indexp to the next
	 * value beyond the name.
	 *
	 * INPUT:  wire: a pointer to an array of bytes
	 * INPUT:  indexp, a pointer to the index in the wire where the
	 *              wire-formatted name begins
	 * OUTPUT: a string containing the string representation of the name,
	 *              allocated on the heap.
	 */
}

dns_rr rr_from_wire(unsigned char *wire, int *indexp, int query_only) {
	/* 
	 * Extract the wire-formatted resource record at the offset specified by
	 * *indexp in the array of bytes provided (wire) and return a 
	 * dns_rr (struct) populated with its contents. Update the value
	 * pointed to by indexp to the next value beyond the resource record.
	 *
	 * INPUT:  wire: a pointer to an array of bytes
	 * INPUT:  indexp: a pointer to the index in the wire where the
	 *              wire-formatted resource record begins
	 * INPUT:  query_only: a boolean value (1 or 0) which indicates whether
	 *              we are extracting a full resource record or only a
	 *              query (i.e., in the question section of the DNS
	 *              message).  In the case of the latter, the ttl,
	 *              rdata_len, and rdata are skipped.
	 * OUTPUT: the resource record (struct)
	 */
}


int rr_to_wire(dns_rr rr, unsigned char *wire, int query_only) {
	/* 
	 * Convert a DNS resource record struct to DNS wire format, using the
	 * provided byte array (wire).  Return the number of bytes used by the
	 * name in wire format.
	 *
	 * INPUT:  rr: the dns_rr struct containing the rr record
	 * INPUT:  wire: a pointer to the array of bytes where the
	 *             wire-formatted resource record should be constructed
	 * INPUT:  query_only: a boolean value (1 or 0) which indicates whether
	 *              we are constructing a full resource record or only a
	 *              query (i.e., in the question section of the DNS
	 *              message).  In the case of the latter, the ttl,
	 *              rdata_len, and rdata are skipped.
	 * OUTPUT: the length of the wire-formatted resource record.
	 *
	 */
}

unsigned short create_dns_query(char *qname, dns_rr_type qtype, unsigned char *wire) {
	/* 
	 * Create a wire-formatted DNS (query) message using the provided byte
	 * array (wire).  Create the header and question sections, including
	 * the qname and qtype.
	 *
	 * INPUT:  qname: the string containing the name to be queried
	 * INPUT:  qtype: the integer representation of type of the query (type A == 1)
	 * INPUT:  wire: the pointer to the array of bytes where the DNS wire
	 *               message should be constructed
	 * OUTPUT: the length of the DNS wire message
	 */
}

dns_answer_entry *get_answer_address(char *qname, dns_rr_type qtype, unsigned char *wire) {
	/* 
	 * Extract the IPv4 address from the answer section, following any
	 * aliases that might be found, and return the string representation of
	 * the IP address.  If no address is found, then return NULL.
	 *
	 * INPUT:  qname: the string containing the name that was queried
	 * INPUT:  qtype: the integer representation of type of the query (type A == 1)
	 * INPUT:  wire: the pointer to the array of bytes representing the DNS wire message
	 * OUTPUT: a linked list of dns_answer_entrys the value member of each
	 * reflecting either the name or IP address.  If
	 */
}

dns_answer_entry *resolve(char *qname, char *server, char *port) {
    //Create wire and header of query
    unsigned char wire[64];
    Dns_Query_Header *header = malloc(sizeof(Dns_Query_Header));

    //Set values of header
    header->indentification = 0xd627;
    header->flags = 0x0001;
    header->questions = 0x0100;
    header->arr = 0x0000;
    header->authorityr1 = 0x00;
    header->authorityr2 = 0x00;

    //Copy values from header to wire
    memcpy(wire, (unsigned char*)header, sizeof(Dns_Query_Header));


    //Tokenize and convert qname to hex and put in wire
    char *token = strtok(qname, ".");
    int offset = sizeof(Dns_Query_Header);

    while (token != NULL) {

        int len = strlen(token);

        unsigned char * length  = malloc(sizeof(unsigned char));
        *length = (unsigned char) len;

        memcpy(&wire[offset], (unsigned char*)length, 1);
        offset += 1;

        memcpy(&wire[offset], (unsigned char*)token, strlen(token));
        offset += len;

        token = strtok(NULL, ".");

        free(length);
    }

    //Set hardcoded values at the end of query
    unsigned char  * null = malloc(sizeof(unsigned char));
    *null = '\0';
    memcpy(&wire[offset], (unsigned char*)null, 1);
    offset+= 1;

    unsigned short *type = malloc(sizeof(unsigned short));
    *type = 0x0100;
    memcpy(&wire[offset], (unsigned char*)type, 2);
    offset+= 2;
    unsigned short *class = malloc(sizeof(unsigned short));
    *class = 0x0100;
    memcpy(&wire[offset], (unsigned char*)class, 2);
    offset+= 2;

    //Free memory used in created query
    free(header);
    free(null);
    free(type);
    free(class);

    //Set up client and connect to dns lookup server
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    size_t len;
    ssize_t nread;
    char buf[500];
    int af;

    af = AF_INET;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = af;    /* Allow IPv4, IPv6, or both, depending on
				    what was specified on the command line. */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(server, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    //Write query to lookup server
    if (write(sfd, wire, offset) != offset) {
        fprintf(stderr, "partial/failed write\n");
        exit(EXIT_FAILURE);
    }

    //Read anser from server
    nread = read(sfd, buf, 500);
    if (nread == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }


    //Get number of responses from answer from server
    unsigned short rr = (unsigned short) buf[7];

    //Initialize pointers used in creating answer list
    struct dns_answer_entry *head;
    struct dns_answer_entry *answer = malloc(sizeof(struct dns_answer_entry));
    answer->value = malloc(64);
    memset(answer->value, 0, 64);
    answer->next = NULL;

    head = answer;

    head->next = answer;

    //Iterate through every response in answer
    while (rr > 0) {
        //Get type of current response
        unsigned short typeAnswer = buf[offset + 3];

        //If type 5, CNAME lookup
        if (typeAnswer == 5) { //CNAME record
            int addr = 0;

            //Get lenght of data to read
            unsigned short datalength = buf[offset + 11];
            offset+=11;

            //Get length of name to be read
            unsigned short namelen = buf[offset + 1];
            offset += 2;


            //Initialize value used to read and copy name
            char temp[4];
            unsigned short strptr = 0;

            int ptrfound = 0;
            int nextvalue = 0;

            //Loop until done reading characters from answer
            while (1) {

                //Iterate over name length, append to string
                for (int i = 0; i < namelen; i++) {
                    sprintf(temp, "%c", buf[offset + i]);
                    answer->value[strptr + i] = temp[0];
                }

                //Check next character, if C0 or 00, break, else, append . and get new name length
                offset += namelen;
                strptr += namelen;


                nextvalue = buf[offset];
                if (nextvalue == 0x00) { //End of CNAME found, break while loop
                    break;
                }
                else if (nextvalue == 0xffffffc0 || nextvalue == 0xc0) { //This part of CNAME done, set flag that pointer is next
                    ptrfound = 1;
                    break;
                }
                else {  //Name not done yet, get new name length, update name length and go again
                    namelen = nextvalue;
                    answer->value[strptr] = '.';
                    offset += 1;
                    strptr += 1;

                }
            }

            if (ptrfound) {  //Next value is pointer, keep adding to the string
                offset += 1;

                //Append period after name
                answer->value[strptr] = '.';
                strptr += 1;

                //Get address that answer is point to
                int ptr = buf[offset];
                namelen = buf[ptr];

                ptr += 1;

                //Loop and read characters from answer at pointer location 
                while (1) {

                    //Read and append characters
                    for (int i = 0; i < namelen; i++) {
                        sprintf(temp, "%c", buf[ptr + i]);
                        answer->value[strptr + i] = temp[0];
                    }

                    //Check next character, if C0 or 00, break, else, append . and get new name length
                    ptr += namelen;
                    strptr += namelen;
                    nextvalue = buf[ptr];
                    
                    if (nextvalue == 0x00) {
                        break;
                    } else if (nextvalue == 0xffffffc0 || nextvalue == 0xc0) {
                        ptrfound = 1;
                        break;
                    } else {  //Get new name length, update name length and go again
                        namelen = nextvalue;
                        answer->value[strptr] = '.';
                        ptr += 1;
                        strptr += 1;

                    }
                }

                //Set offset to next response in answer
                offset += 1;

                //Increment number of responses left to read
                rr -= 1;
            }
            else {  //Next value is part of next response
                //Set off set to first value of next response
                offset += 1;
                //Increment number of responses left
                rr -= 1;
            }
        }
        else {  //Is normal IPv4 address to handle

            //Get value for length of data and data location
            int dataLengthOffset = offset + 11;
            int dataOffset = dataLengthOffset + 1;

            unsigned short dataLength = (unsigned short) buf[dataLengthOffset];

            //Create array to store string of hex converted to ip address
            char ipinput[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &buf[dataOffset], ipinput, INET_ADDRSTRLEN);

            //Copy new string to answer->value
            memcpy(answer->value, ipinput, INET_ADDRSTRLEN);

            //Increment number of responses left
            rr -= 1;
        }

        //Set up next pointer in linked list and initialize array
        struct dns_answer_entry *next = malloc(sizeof(struct dns_answer_entry));
        next->value = malloc(64);
        memset(next->value, 0, 64);
        next->next = NULL;

        answer->next = next;
        answer = next;
    }

    //Set next of final pointer to null
    answer->next = NULL;

    return head;
}

int main(int argc, char *argv[]) {
	char *port;
	dns_answer_entry *ans_list, *ans;
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <domain name> <server> [ <port> ]\n", argv[0]);
		exit(1);
	}
	if (argc > 3) {
		port = argv[3];
	} else {
		port = "53";
	}
	ans = ans_list = resolve(argv[1], argv[2], port);
	while (ans != NULL) {
		printf("%s\n", ans->value);
		ans = ans->next;
	}
	if (ans_list != NULL) {
		free_answer_entries(ans_list);
	}
}
