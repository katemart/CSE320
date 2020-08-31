#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "const.h"
#include "audio.h"
#include "dtmf.h"
#include "dtmf_static.h"
#include "goertzel.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 */

/**
 * DTMF generation main function.
 * DTMF events are read (in textual tab-separated format) from the specified
 * input stream and audio data of a specified duration is written to the specified
 * output stream.  The DTMF events must be non-overlapping, in increasing order of
 * start index, and must lie completely within the specified duration.
 * The sample produced at a particular index will either be zero, if the index
 * does not lie between the start and end index of one of the DTMF events, or else
 * it will be a synthesized sample of the DTMF tone corresponding to the event in
 * which the index lies.
 *
 *  @param events_in  Stream from which to read DTMF events.
 *  @param audio_out  Stream to which to write audio header and sample data.
 *  @param length  Number of audio samples to be written.
 *  @return 0 if the header and specified number of samples are written successfully,
 *  EOF otherwise.
 */
int dtmf_generate(FILE *events_in, FILE *audio_out, uint32_t length) {
    // TO BE IMPLEMENTED
    return EOF;
}

/**
 * DTMF detection main function.
 * This function first reads and validates an audio header from the specified input stream.
 * The value in the data size field of the header is ignored, as is any annotation data that
 * might occur after the header.
 *
 * This function then reads audio sample data from the input stream, partititions the audio samples
 * into successive blocks of block_size samples, and for each block determines whether or not
 * a DTMF tone is present in that block.  When a DTMF tone is detected in a block, the starting index
 * of that block is recorded as the beginning of a "DTMF event".  As long as the same DTMF tone is
 * present in subsequent blocks, the duration of the current DTMF event is extended.  As soon as a
 * block is encountered in which the same DTMF tone is not present, either because no DTMF tone is
 * present in that block or a different tone is present, then the starting index of that block
 * is recorded as the ending index of the current DTMF event.  If the duration of the now-completed
 * DTMF event is greater than or equal to MIN_DTMF_DURATION, then a line of text representing
 * this DTMF event in tab-separated format is emitted to the output stream. If the duration of the
 * DTMF event is less that MIN_DTMF_DURATION, then the event is discarded and nothing is emitted
 * to the output stream.  When the end of audio input is reached, then the total number of samples
 * read is used as the ending index of any current DTMF event and this final event is emitted
 * if its length is at least MIN_DTMF_DURATION.
 *
 *   @param audio_in  Input stream from which to read audio header and sample data.
 *   @param events_out  Output stream to which DTMF events are to be written.
 *   @return 0  If reading of audio and writing of DTMF events is sucessful, EOF otherwise.
 */
int dtmf_detect(FILE *audio_in, FILE *events_out) {
    // TO BE IMPLEMENTED
    return EOF;
}

int str_comp(char *str1, char *str2) {
    for(; *str1 != '\0' || *str2 != '\0'; str1++, str2++) {
        if(*str1 != *str2)
          return *str1 - *str2;
    }
  return 0;
}

int str_to_num(char *str_number, int *number) {
    int n = 0;
    int negative = 1;
    if(*str_number == '-') {
          negative = -1;
          str_number++;
    }
    for(; *str_number != '\0'; str_number++) {
        if(*str_number >= '0' && *str_number <= '9')
          n = (n * 10) + (*str_number - '0');
        else {
          return -1;
        }
    }
  *number = n * negative;
  return 0;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the operation mode of the program (help, generate,
 * or detect) will be recorded in the global variable `global_options`,
 * where it will be accessible elsewhere in the program.
 * Global variables `audio_samples`, `noise file`, `noise_level`, and `block_size`
 * will also be set, either to values derived from specified `-t`, `-n`, `-l` and `-b`
 * options, or else to their default values.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected program operation mode, and global variables `audio_samples`,
 * `noise file`, `noise_level`, and `block_size` to contain values derived from
 * other option settings.
 */
int validargs(int argc, char **argv) {
    // TO BE IMPLEMENTED
    debug("%d", global_options);
    int ret_value = -1;
    int msec_arg;
    int level_arg;
    char *noisefile_arg;
    int blocksize_arg;
    if(argc == 1)
		return -1;
	for(int i = 1; i < argc; i++) {
        if(global_options == 0) {
            char *current_elem = *(argv + i);        //pointer that points to start of a char in mem
            if(str_comp(current_elem, "-h") == 0) {
            global_options = HELP_OPTION;
                ret_value = 0;
            }
            else if(str_comp(current_elem, "-g") == 0) {
                global_options = GENERATE_OPTION;
                int t_flag = 0;
                int n_flag = 0;
                int l_flag = 0;
                if(argc == 2){
                    audio_samples = 1000;
                    noise_file = NULL;
                    noise_level = 0;
                }
                for(int j = i++; j < argc; j++) {
                    char *current_opt_elem = *(argv + j);
                    debug("current opt elem, %s", current_opt_elem);
                    if(str_comp(current_opt_elem, "-t") == 0) {
                        if(t_flag == 0) {
                            t_flag = 1;
                            current_opt_elem = *(argv + (j+1));
                            if(current_opt_elem != NULL) {
                                int converted_number;
                                if(str_to_num(current_opt_elem, &converted_number) < 0) {
                                    ret_value = -1;
                                } else {
                                    if(converted_number >= 0 && converted_number <= UINT32_MAX) {
                                        msec_arg = converted_number * 8;
                                        ret_value = 0;
                                    }
                                }

                            }
                            else {
                                ret_value = -1;
                            }
                        }
                        else {
                            ret_value = -1;
                        }
                    }
                    else if(str_comp(current_opt_elem, "-n") == 0) {
                        if(n_flag == 0) {
                            n_flag = 1;
                            current_opt_elem = *(argv + (j+1));
                            if(current_opt_elem != NULL) {
                                noisefile_arg = current_opt_elem;
                                ret_value = 0;
                            }
                            else {
                                ret_value = -1;
                            }
                        }
                        else {
                            ret_value = -1;
                        }
                    }
                    else if(str_comp(current_opt_elem, "-l") == 0)  {
                        if(l_flag == 0) {
                            l_flag = 1;
                            current_opt_elem = *(argv + (j+1));
                            if(current_opt_elem != NULL) {
                                int converted_number;
                                if(str_to_num(current_opt_elem, &converted_number) < 0) {
                                    debug("%d", converted_number);
                                    ret_value = -1;
                                }
                                else {
                                    if(converted_number >= -30 && converted_number <= 30) {
                                        debug("%d", converted_number);
                                        level_arg = 10*log10(converted_number);
                                        ret_value = 0;
                                    }
                                }
                            }
                            else {
                                ret_value = -1;
                            }
                        }
                        else {
                            ret_value = 1;
                        }
                    }
                }

            }
            else if(str_comp(current_elem, "-d") == 0) {
                global_options = DETECT_OPTION;
                int b_flag = 0;
                if(argc == 2) {
                    block_size = 100;
                }
                char *current_opt_elem = *(argv + 2);
                debug("current elem, %s", current_opt_elem);
                if(str_comp(current_opt_elem, "-b") == 0) {
                    if(b_flag == 0) {
                        b_flag = 1;
                        current_opt_elem = *(argv + 3);
                        debug("next curr elem, %s",current_opt_elem);
                        if(current_opt_elem != NULL) {
                            int converted_number;
                            if(str_to_num(current_opt_elem, &converted_number) < 0) {
                                ret_value = -1;
                            }
                            else {
                                if(converted_number >= 10 && converted_number <= 1000) {
                                    blocksize_arg = converted_number;
                                    ret_value = 0;
                                }
                            }
                        }
                        else {
                            ret_value = -1;
                        }
                    }
                    else {
                        ret_value = -1;
                    }
                }
            }
        }
    }
    if(ret_value == 0) {
        audio_samples = msec_arg;
        noise_file = noisefile_arg;
        noise_level = level_arg;
        block_size = blocksize_arg;
    }
    return ret_value;
}
