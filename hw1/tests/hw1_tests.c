#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>  // You may use this here in the test cases, but not elsewhere.
#include <math.h>
#include "const.h"

Test(basecode_tests_suite, validargs_help_test) {
    int argc = 2;
    char *argv[] = {"bin/dtmf", "-h", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x1;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit (0x1) not set for -h. Got: %x", opt);
}

Test(basecode_tests_suite, validargs_generate_test) {
    char *nfile = "noise.au";
    int argc = 4;
    char *argv[] = {"bin/dtmf", "-g", "-n", nfile, NULL};
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x2;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert(opt & flag, "Generate mode bit wasn't set. Got: %x", opt);
    cr_assert_eq(strcmp(noise_file, nfile), 0,
		 "Variable 'noise_file' was not properly set.  Got: %s | Expected: %s",
		 noise_file, nfile);
}

Test(basecode_tests_suite, validargs_detect_test) {
    int argc = 4;
    char *argv[] = {"bin/dtmf", "-d", "-b", "10", NULL};
    int exp_ret = 0;
    int exp_size = 10;
    int ret = validargs(argc, argv);
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(block_size, exp_size, "Block size not properly set. Got: %d | Expected: %d",
		 block_size, exp_size);
}

Test(basecode_tests_suite, validargs_error_test) {
    int argc = 4;
    char *argv[] = {"bin/dtmf", "-g", "-b", "10", NULL};
    int exp_ret = -1;
    int ret = validargs(argc, argv);
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
		 ret, exp_ret);
}

Test(basecode_tests_suite, help_system_test) {
    char *cmd = "bin/dtmf -h";

    // system is a syscall defined in stdlib.h
    // it takes a shell command as a string and runs it
    // we use WEXITSTATUS to get the return code from the run
    // use 'man 3 system' to find out more
    int return_code = WEXITSTATUS(system(cmd));

    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);
}

Test(basecode_tests_suite, goertzel_basic_test) {
    int N = 100;
    int k = 1;
    GOERTZEL_STATE g0, g1, g2;
    goertzel_init(&g0, N, 0);
    goertzel_init(&g1, N, 1);
    goertzel_init(&g2, N, 2);
    double x;
    for(int i = 0; i < N-1; i++) {
	x = cos(2 * M_PI * i / N);
	goertzel_step(&g0, x);
	goertzel_step(&g1, x);
	goertzel_step(&g2, x);
    }
    x = cos(2 * M_PI * (N-1) / N);
    double r0 = goertzel_strength(&g0, x);
    double r1 = goertzel_strength(&g1, x);
    double r2 = goertzel_strength(&g2, x);
    double eps = 1e-6;
    cr_assert((fabs(r0) < eps), "r0 was %f, should be 0.0", r0);
    cr_assert((fabs(r1-0.5) < eps), "r1 was %f, should be 0.5", r1);
    cr_assert((fabs(r2) < eps), "r2 was %f, should be 0.0", r2);
}

/************************************KATHY'S UNIT TESTS************************************/
Test(basecode_tests_suite, validargs_default_g) {
    int argc = 2;
    char *argv[] = {"bin/dtmf", "-g"};
    int ret = validargs(argc, argv);
    cr_assert_eq(ret, 0, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, 0);
    cr_assert(global_options & 0x02, "Generate mode bit wasn't set. Got: %x", global_options);
    cr_assert_eq(audio_samples, 8000, "Audio samples value not properly set. Got: %d | Expected: %d",
         audio_samples, 1000);
    // cr_assert_eq(noise_file, NULL,
    //      "Variable 'noise_file' was not properly set.  Got: %s | Expected: %s",
    //      noise_file, NULL);
    cr_assert_eq(noise_level, 0, "Correct noise level not set. Got: %d | Expected: %d",
         noise_level, 0);
    cr_assert_eq(block_size, 0, "Block size not properly set. Got: %d | Expected: %d",
         block_size, 0);
}

Test(basecode_tests_suite, validargs_invalid_flag) {
    int argc = 5;
    char *argv[] = {"bin/dtmf", "-g", "-t", "50", "-f"};
    int ret = validargs(argc, argv);
    cr_assert_eq(ret, -1, "Invalid return for valid args.  Got: %d | Expected: %d",
         ret, -1);
    cr_assert_eq(global_options, 0, "Correct bit not set. Got: %d | Expected: %d",
         global_options, 0);
    cr_assert_eq(audio_samples, 0, "MSEC value not properly set. Got: %d | Expected: %d",
         audio_samples, 0);
    // cr_assert_eq(noise_file, NULL,
    //      "Variable 'noise_file' was not properly set.  Got: %s | Expected: %s",
    //      noise_file, NULL);
    cr_assert_eq(noise_level, 0, "Correct noise level not set. Got: %d | Expected: %d",
         noise_level, 0);
    cr_assert_eq(block_size, 0, "Block size not properly set. Got: %d | Expected: %d",
         block_size, 0);
}

Test(basecode_tests_suite, goertzel_advanced_test) {
    //read header and set pointer to sample data start
    AUDIO_HEADER header;
    FILE *fp = fopen("./rsrc/dtmf_0_500ms.au", "r");
    audio_read_header(fp, &header);
    //goertzel init
    int N = 1000;
    double k0 = (double) 697*N / 8000;
    double k1 = (double) 770*N / 8000;
    double k2 = (double) 852*N / 8000;
    double k3 = (double) 941*N / 8000;
    double k4 = (double) 1209*N / 8000;
    double k5 = (double) 1336*N / 8000;
    double k6 = (double) 1477*N / 8000;
    double k7 = (double) 1633*N / 8000;
    GOERTZEL_STATE g0, g1, g2, g3, g4, g5, g6, g7;
    goertzel_init(&g0, N, k0);
    goertzel_init(&g1, N, k1);
    goertzel_init(&g2, N, k2);
    goertzel_init(&g3, N, k3);
    goertzel_init(&g4, N, k4);
    goertzel_init(&g5, N, k5);
    goertzel_init(&g6, N, k6);
    goertzel_init(&g7, N, k7);
    //goertzel step
    double x;
    int16_t sample;
    for(int i = 0; i < N-1; i++) {
        audio_read_sample(fp, &sample);
        x = (double) sample / INT16_MAX;
        goertzel_step(&g0, x);
        goertzel_step(&g1, x);
        goertzel_step(&g2, x);
        goertzel_step(&g3, x);
        goertzel_step(&g4, x);
        goertzel_step(&g5, x);
        goertzel_step(&g6, x);
        goertzel_step(&g7, x);
    }
    //goertzel strength
    audio_read_sample(fp, &sample);
    x = (double) sample / INT16_MAX;
    double r0 = goertzel_strength(&g0, x);
    double r1 = goertzel_strength(&g1, x);
    double r2 = goertzel_strength(&g2, x);
    double r3 = goertzel_strength(&g3, x);
    double r4 = goertzel_strength(&g4, x);
    double r5 = goertzel_strength(&g5, x);
    double r6 = goertzel_strength(&g6, x);
    double r7 = goertzel_strength(&g7, x);
    cr_assert(r0, "r0 was %f, should be 0.000003", r0);
    cr_assert(r1, "r1 was %f, should be 0.000007", r1);
    cr_assert(r2, "r2 was %f, should be 0.000003", r2);
    cr_assert(r3, "r3 was %f, should be 0.031544", r3);
    cr_assert(r4, "r4 was %f, should be 0.000002", r4);
    cr_assert(r5, "r5 was %f, should be 0.031479", r5);
    cr_assert(r6, "r6 was %f, should be 0.000009", r6);
    cr_assert(r7, "r7 was %f, should be 0.000001", r7);
}