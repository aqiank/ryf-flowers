#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pulse/pulseaudio.h>

// Arduino file descriptor
static int arduino_fd;

// Pulseaudio
static pa_context *context;
static pa_stream *stream;

void handle_signal(int code) {
	if (arduino_fd) {
		close(arduino_fd);
	}

	if (context) {
		pa_context_unref(context);
	}

	exit(EXIT_SUCCESS);
}

void sourceinfo_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata);

void find_microphone(pa_context *c) {
	fprintf(stdout, "Finding audio input..\n");
	fflush(stdout);

	pa_operation *o;

	if (!(o = pa_context_get_source_info_list(c, sourceinfo_cb, NULL))) {
		fprintf(stderr, "context_state_cb: pa_context_subscribe() failed\n");
		fflush(stderr);
		return;
	}

	pa_operation_unref(o);
}

void stream_read_cb(pa_stream *p, size_t nbytes, void *userdata) {
	int average = 0;
	uint8_t *data;
	
	pa_stream_peek(p, (const void **) &data, &nbytes);

	average = 0;
	for (int i = 0; i < nbytes; i++) {
		average += data[i] - 128;
	}
	average /= nbytes;

	if (write(arduino_fd, (void *) &average, 1) == -1) {
		exit(EXIT_FAILURE);
	};

	pa_stream_drop(p);
}

void stream_state_cb(pa_stream *p, void *userdata) {
	pa_stream_state_t state = pa_stream_get_state(p);
	if (state == PA_STREAM_FAILED || state == PA_STREAM_TERMINATED) {
		if (context) {
			pa_stream_unref(stream);
			stream = NULL;
			find_microphone(context);
		}
	}
}

void sourceinfo_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
	if (!stream) {
		if (i && strstr(i->name, "input")/* && strstr(i->name, "GoMic")*/) {
			pa_sample_spec spec;
			spec.channels = 2;
			spec.format = PA_SAMPLE_U8;
			spec.rate = 22050;

			stream = pa_stream_new(context, "peak detect", &spec, NULL);
			pa_stream_set_read_callback(stream, stream_read_cb, NULL);
			pa_stream_set_state_callback(stream, stream_state_cb, NULL);
			pa_stream_connect_record(stream, i->name, NULL, PA_STREAM_PEAK_DETECT);

			fprintf(stdout, "Found audio input: %s\n", i->name);
			fflush(stdout);
		} else if (!i) {
			sleep(1);
			find_microphone(c);
		}
	}
}

void context_state_cb(pa_context *c, void *userdata) {
	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_UNCONNECTED:
	case PA_CONTEXT_CONNECTING:
		fprintf(stdout, "PulseAudio connection is being established\n");
		fflush(stdout);
		break;
	case PA_CONTEXT_AUTHORIZING:
	case PA_CONTEXT_SETTING_NAME:
		break;

	case PA_CONTEXT_READY: {
		find_microphone(c);
		break;
	}
	case PA_CONTEXT_FAILED:
		fprintf(stderr, "context_state_cb: PulseAudio connection failed\n");
		fflush(stderr);
		break;
	case PA_CONTEXT_TERMINATED:
	default:
		return;
	}
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		fprintf(stderr, "main: Please start with %s /dev/ttyACM0 (for example)\n", argv[0]);
		fflush(stderr);
		return EXIT_FAILURE;
	}

	fprintf(stdout, "Found device at %s\n", argv[1]);
	fflush(stdout);

	// Signal handling
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	// PulseAudio stuff
	pa_mainloop *pa_ml;
	pa_mainloop_api *pa_mlapi;
	int err;

	pa_ml = pa_mainloop_new();
	pa_mlapi = pa_mainloop_get_api(pa_ml);
	context = pa_context_new(pa_mlapi, "Flowers");

	err = pa_context_connect(context, NULL, 0, NULL);
	if (err != 0) {
		fprintf(stderr, "main: %s\n", pa_strerror(err));
		fflush(stderr);
		exit(EXIT_FAILURE);
	}
	pa_context_set_state_callback(context, context_state_cb, NULL);

	// Arduino stuff
	struct termios tio;
	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = CS8|CREAD|CLOCAL;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;

	fprintf(stdout, "Opening device at %s\n", argv[1]);
	fflush(stdout);

	arduino_fd = open(argv[1], O_RDWR | O_NONBLOCK);
	if (arduino_fd == -1) {
		fprintf(stderr, "main: Failed to open device at %s\n", argv[1]);
		return EXIT_FAILURE;
	}
	fprintf(stdout, "Opened device at %s\n", argv[1]);
	fflush(stdout);

	sleep(1);

	cfsetospeed(&tio, B9600);
	cfsetispeed(&tio, B9600);
	tcsetattr(arduino_fd, TCSANOW, &tio);

	// Run PulseAudio
	if (pa_mainloop_run(pa_ml, &err) < 0) {
		printf("main: pa_mainloop_run() failed.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
