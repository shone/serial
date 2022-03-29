#include <libserialport.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <argp.h>
#include <stdbool.h>

static char doc[] = "An example program for reading/writing on serial ports, written in C.";
static struct argp_option options[] = { 
	{ "device",   'd', "FILEPATH", 0, "The serial device to use. Defaults to /dev/ttyUSB0" },
	{ "baudrate", 'b', "RATE",     0, "The baud rate to use. Defaults to 115200" },
	{ "verbose",  'v', 0,          0, "Print verbose logging information." },
	{ 0 }
};

struct arguments {
	const char* device;
	bool verbose;
	int baudrate;
};

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
	struct arguments* arguments = state->input;
	switch (key) {
	case 'd': arguments->device = arg; break;
	case 'b': arguments->baudrate = strtoumax(arg, NULL, 10); break;
	case 'v': arguments->verbose = true;  break;
	case ARGP_KEY_ARG: return 0;
	default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, NULL, doc, 0, 0, 0 };

char* get_error_string(enum sp_return result);

int main(int argc, char **argv) {
	struct arguments arguments;
	arguments.device = "/dev/ttyUSB0";
	arguments.baudrate = 115200;
	arguments.verbose = false;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	struct sp_port* port;

	int result;

	if (arguments.verbose) {
		printf("Getting port by name '%s'...\n", arguments.device);
	}
	result = sp_get_port_by_name(arguments.device, &port);
	if (result != SP_OK) {
		fprintf(stderr, "Error while getting port by name '%s': %s\n", arguments.device, get_error_string(result));
		return -1;
	}

	if (arguments.verbose) {
		printf("Opening port '%s'...\n", arguments.device);
	}
	result = sp_open(port, SP_MODE_READ_WRITE);
	if (result != SP_OK) {
		fprintf(stderr, "Error while opening port '%s': %s\n", arguments.device, get_error_string(result));
		return -1;
	}

// 	struct sp_port_config* config;
// 	sp_new_config(&config);
	if (arguments.verbose) {
		printf("Setting baud rate to %d...\n", arguments.baudrate);
	}
// 	result = sp_set_config_baudrate(config, arguments.baudrate);
	result = sp_set_baudrate(port, arguments.baudrate);
	if (result != SP_OK) {
		fprintf(stderr, "Error while setting baud rate: %s\n", get_error_string(result));
		return -1;
	}

	if (arguments.verbose) {
		printf("Setting bit count...\n");
	}
// 	result = sp_set_config_bits(config, 8);
	result = sp_set_bits(port, 8);
	if (result != SP_OK) {
		fprintf(stderr, "Error while setting bit count: %s\n", get_error_string(result));
		return -1;
	}

	if (arguments.verbose) {
		printf("Setting parity...\n");
	}
// 	result = sp_set_config_parity(config, SP_PARITY_NONE);
	result = sp_set_parity(port, SP_PARITY_NONE);
	if (result != SP_OK) {
		fprintf(stderr, "Error while setting parity: %s\n", get_error_string(result));
		return -1;
	}

	if (arguments.verbose) {
		printf("Setting stop bits...\n");
	}
// 	result = sp_set_config_stopbits(config, 1);
	result = sp_set_stopbits(port, 1);
	if (result != SP_OK) {
		fprintf(stderr, "Error while setting stop bits: %s\n", get_error_string(result));
		return -1;
	}

	if (arguments.verbose) {
		printf("Setting flow control...\n");
	}
// 	result = sp_set_config_flowcontrol(config, SP_FLOWCONTROL_NONE);
	result = sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
	if (result != SP_OK) {
		fprintf(stderr, "Error while setting flow control: %s\n", get_error_string(result));
		return -1;
	}

// 	printf("Setting config..\n");
// 	result = sp_set_config(port, config);
// 	if (result != SP_OK) {
// 		fprintf(stderr, "Error while setting serial port config: %s\n", get_error_string(result));
// 		return -1;
// 	}

	// Check if there's something to read on stdin
	fd_set select_fds;
	FD_ZERO(&select_fds);
	FD_SET(STDIN_FILENO, &select_fds);
	struct timeval select_timeouts={0};
	bool has_stdin = select(1, &select_fds, NULL, NULL, &select_timeouts) > 0;

	if (has_stdin) {
		if (arguments.verbose) {
			printf("Reading from stdin...\n");
		}
		char write_buffer[4096];
		size_t stdin_byte_count = read(STDIN_FILENO, write_buffer, sizeof(write_buffer));
		if (stdin_byte_count > 0) {
			if (stdin_byte_count == sizeof(write_buffer)) {
				fprintf(stderr, "Stdin exceeded internal buffer (%lu bytes).\n", sizeof(write_buffer));
				return -1;
			}
			if (arguments.verbose) {
				printf("Got %lu bytes from stdin: '%.*s'\n", stdin_byte_count, (int)stdin_byte_count, write_buffer);
			}
			if (arguments.verbose) {
				printf("Writing %lu bytes to port %s: '%.*s'...\n", stdin_byte_count, arguments.device, (int)stdin_byte_count, write_buffer);
			}
			unsigned int write_timeout = 1000;
			result = sp_blocking_write(port, write_buffer, stdin_byte_count, write_timeout);
			if (result < 0) {
				fprintf(stderr, "Error while writing to serial port: %s\n", get_error_string(result));
				return -1;
			}
			if (result < stdin_byte_count) {
				fprintf(stderr, "Timed out while writing to serial port '%s'.\n", arguments.device);
				return -1;
			}
		}
	}

	char read_buffer[4096];
	const unsigned int read_timeout = 0;

	if (arguments.verbose) {
		printf("Reading from serial port...\n");
	}
	while (1) {
		result = sp_blocking_read_next(port, read_buffer, sizeof(read_buffer), read_timeout);
		if (result < 0) {
			fprintf(stderr, "Error while reading from serial port: %s\n", get_error_string(result));
			return -1;
		}
		if (result == 0) {
			fprintf(stderr, "Timed-out while reading from serial port '%s'\n", arguments.device);
			return -1;
		}
		printf("%.*s", result, read_buffer);
		fflush(stdout);
	}

	result = sp_close(port);
	if (result != SP_OK) {
		fprintf(stderr, "Error while closing serial port: %s\n", get_error_string(result));
		return -1;
	}

	sp_free_port(port);

	return 0;
}

char* get_error_string(enum sp_return result) {
	switch (result) {
	case SP_ERR_ARG:  return "Invalid argument";
	case SP_ERR_FAIL: return sp_last_error_message();
	case SP_ERR_SUPP: return "Not supported";
	case SP_ERR_MEM:  return "Couldn't allocate memory";
	case SP_OK:       return "Ok (no error)";
	default:
		return "Unknown error";
	}
}

