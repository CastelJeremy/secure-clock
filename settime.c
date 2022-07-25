#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/capability.h>
#include <sys/time.h>

int main(int argc, char** argv) {
    cap_t caps = cap_get_proc();
    cap_value_t required_caps[1] = { CAP_SYS_TIME };

    // Remove every capabilities except required ones
    cap_clear(caps);
    cap_set_flag(caps, CAP_PERMITTED, 1, required_caps, CAP_SET);
    cap_set_flag(caps, CAP_EFFECTIVE, 1, required_caps, CAP_SET);
    cap_set_proc(caps);
    cap_free(caps);

    if (argc == 2) {
        long long time = strtol(argv[1], NULL, 10);

        if (time > 0 && time <= LONG_MAX) {
            struct timeval tv = { time, 0 };

            if (settimeofday(&tv, NULL) == 0) {
                return 0;
            } else if (errno == EPERM) { // Missing capabilities
                return -2;
            }

            return -3; // Something went wrong
        } else {
            return -4; // Could not parse argument
        }
    }

    return -1; // Invalid arguments
}
