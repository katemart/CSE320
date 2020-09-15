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
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!/
 */

//string to number helper function -- accounts for negative numbers; returns 0 or 1 along with converted number
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
        else return -1;
    }
  *number = n * negative;
  return 0;
}

//string comparator helper function
int str_comp(char *str1, char *str2) {
    for(; *str1 != '\0' || *str2 != '\0'; str1++, str2++) {
        if(*str1 != *str2)
          return *str1 - *str2;
    }
  return 0;
}

//function to look up given symbol in 2D mapping
int find_symbol(char symbol, int *fr, int *fc) {
    //mapping[i][j] = mapping[rows][columns]
    for(int i = 0; i < NUM_DTMF_ROW_FREQS; i++) {
        for(int j =  0; j < NUM_DTMF_COL_FREQS; j++) {
            if(symbol == *(*(dtmf_symbol_names +i) +j)) {
                //dtmf_freqs[fr], dtmf_freqs[fc + 4]
                *fr = *(dtmf_freqs+i);
                *fc = *(dtmf_freqs+(j+4));
                return 0;
            }
        }
    }
    return -1;
}

//function to get start index, end index, symbol
int get_event_fields(int *s_index, int *e_index, char *symbol) {
    //REMINDER: *(line_buf + i) is the same as line_buf[i] and line_buf = addr
    //process one line at a time (i.e., line_buf until end of file)
    //get start index
    char *buf_p = line_buf;
    for(char *current_p = line_buf; *current_p != '\0'; current_p++) {
        if(*current_p == '\t') {
            *current_p = '\0';
            str_to_num(buf_p, s_index);
            buf_p = current_p + 1;
            break;
        }
    }
    //get end index
    for(char *current_p = buf_p; *current_p != '\0'; current_p++) {
        if(*current_p == '\t') {
            *current_p = '\0';
            str_to_num(buf_p, e_index);
            buf_p = current_p + 1;
            break;
        }
    }
    //get symbol and check that it is valid
    *symbol = *buf_p;
    return 0;
}

//function to combine noise file with sample
int combine_noise_file(FILE *fp, FILE *audio_out, int16_t sample) {
    //if header is successfully read, get samples from file
    int16_t noise_sample;
    int read_sample = audio_read_sample(fp, &noise_sample);
    if(read_sample == EOF) {
        noise_sample = 0;
    }
    //if file sample is valid, combine with current sample
    //w = (10^dB/10) / (1 + 10^dB/10)
    double w = (pow(10,(noise_level/10)) / (1 + pow(10, noise_level/10)));
    //debug("w is %f", w);
    //weight noise_sample by w and dtmf_sample by 1-w
    noise_sample = noise_sample * w;
    sample = sample * (1 - w);
    int16_t final_sample = noise_sample + sample;
    int write_sample = audio_write_sample(audio_out, final_sample);
    if(write_sample != 0) {
        return -1;
    }
    return 0;
}

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
    //note: length = audio_samples
    //set other vars
    FILE *fp;
    int file_bool = 0;
    int prev_end = -1;
    int fr, fc, new_end_index;
    uint32_t data_size = length * 2;
    char *str = fgets(line_buf, LINE_BUF_SIZE, events_in);
    //check if a noise file was given, if so mark boolean true
    if(noise_file != NULL){
        //if file cannot be opened return EOF else read samples
        if((fp = fopen(noise_file, "r")) == NULL) {
            return EOF;
        }
        //validate header from file first
        AUDIO_HEADER file_header;
        int read_header = audio_read_header(fp, &file_header);
        if(read_header == EOF) {
            return EOF;
        }
        file_bool = 1;
    }
    //generate audio header
    AUDIO_HEADER header = {AUDIO_MAGIC, AUDIO_DATA_OFFSET, data_size,
        PCM16_ENCODING, AUDIO_FRAME_RATE, AUDIO_CHANNELS};
    int write_header = audio_write_header(audio_out, &header);
    if(write_header != 0) {
        return EOF;
    }
    //generate samples
    while(str != NULL) {
        //get event fields
        char symbol;
        int s_index, e_index;
        get_event_fields(&s_index, &e_index, &symbol);
        //make sure start indices are incrementing and events do not overlap
        if(s_index > e_index || s_index < prev_end) {
            return EOF;
        }
        //if s_index is not zero, pad with zeroes until start index
        if(prev_end == -1 && s_index != 0) {
            for(int i = 0; i < s_index; i++) {
                int16_t sample = 0;
                int write_sample = audio_write_sample(audio_out, sample);
                if(write_sample != 0) {
                    return EOF;
                }
            }
        }
        /* note: to set the zero values (that arent in event), set anything thats
         * between prev index and start index to zero */
        if(prev_end != -1) {
            //if length is less than end index, make length the new end index
            if(length < s_index)
                new_end_index = length;
            else new_end_index = s_index;
            for(int i = prev_end; i < new_end_index; i++) {
                int16_t sample = 0;
                int write_sample = audio_write_sample(audio_out, sample);
                if(write_sample != 0) {
                    return EOF;
                }
            }
        }
        //set current end index to be previous end index
        prev_end = e_index;
        //calculate each i sample
        //if length is less than end index, make length the new end index
        if(length < e_index)
            new_end_index = length;
        else new_end_index = e_index;
        for(int i = s_index; i < new_end_index; i++) {
            //get related i frequecy
            if(find_symbol(symbol, &fr, &fc) < 0) {
                return EOF;
            }
            //do calculations for each index and write to file
            double fr_value = cos(2.0 * M_PI * fr * i / AUDIO_FRAME_RATE) * 0.5;
            double fc_value = cos(2.0 * M_PI * fc * i / AUDIO_FRAME_RATE) * 0.5;
            int16_t dtmf_sample = (int16_t)((fr_value + fc_value) * INT16_MAX);
            //check if noise file was given
            if(file_bool != 0) {
                //if so combine with current dtmf_sample
                int combine = combine_noise_file(fp, audio_out, dtmf_sample);
                if(combine != 0)
                    return EOF;
            } else {
                //if no noise file is given, write dtmf_sample to stdout
                int write_sample = audio_write_sample(audio_out, dtmf_sample);
                if(write_sample != 0) {
                    return EOF;
                }
            }
        }
        //read next line from file
        str = fgets(line_buf, LINE_BUF_SIZE, events_in);
    }
    //pad end of file with zeroes
    if(prev_end != -1) {
        for(int i = prev_end; i < length; i++) {
            int16_t sample = 0;
            int write_sample = audio_write_sample(audio_out, sample);
            if(write_sample != 0) {
                return EOF;
            }
        }
    }
    //close noise file
    if(file_bool != 0) {
        fclose(fp);
    }
    return 0;
}

//helper function for determining frequency strengths
int get_strengths(FILE *fp, int N) {
    //goertzel init
    for(int i = 0; i < NUM_DTMF_FREQS; i++) {
        double k = (double) *(dtmf_freqs + i)/AUDIO_FRAME_RATE * N;
        goertzel_init(goertzel_state + i, N, k);
    }
    //goertzel step
    double x;
    int16_t sample;
    for(int i = 0; i < N-1; i++) {
        int read_sample = audio_read_sample(fp, &sample);
        if(read_sample != 0)
            return -1;
        else {
            x = (double) sample/INT16_MAX;
            for(int j = 0; j < NUM_DTMF_FREQS; j++) {
                goertzel_step(goertzel_state + j, x);
            }
        }
    }
    //goertzel strength
    audio_read_sample(fp, &sample);
    x = (double) sample / INT16_MAX;
    for(int i = 0; i < NUM_DTMF_FREQS; i++) {
        double strength = goertzel_strength(goertzel_state + i, x);
        *(goertzel_strengths + i) = strength;
    }
    for(int i = 0; i < NUM_DTMF_FREQS; i++) {
        debug("%lf", *(goertzel_strengths + i));
    }
    return 0;
}

//helper function for finding greatest strengths


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
    //read and validate header from input stream
    AUDIO_HEADER header;
    int read_header = audio_read_header(audio_in, &header);
        if(read_header == EOF) {
            return EOF;
        }
    //read data in
    get_strengths(audio_in, block_size);

    return 0;
}

//helper function for validation
int extract_int_arg(char *arg, int *num_out) {
    //if -1, no int arg so functionss fails
    if(arg == NULL)
        return -1;
    else
        return str_to_num(arg, num_out);
}

//generate args helper function
int validate_generate_args(int argc, char **argv) {
    //vars used for globals -- set to zero each time (from global vars)
    int msec_arg = 1000 * 8;
    int level_arg = 0;
    char *noisefile_arg = noise_file;
    //vars used to keep track of selections (to avoid repeated flags)
    int t_flag = 0;
    int n_flag = 0;
    int l_flag = 0;
    for(int i = 2; i < argc; i++) {
        char *current = *(argv + i);
        if(str_comp(current, "-t") == 0) {
            //check if t_flag has been previously set, if not set to "true"
            if(t_flag == 0) {
                t_flag = 1;
                //go to next arg (after -t)
                current = *(argv + (i + 1));
                if(extract_int_arg(current, &msec_arg) < 0)
                    return -1;
                else {
                    //divide UINT32_MAX by 8 to avoid overflow
                    if(msec_arg >= 0 && msec_arg <= UINT32_MAX)
                        msec_arg = msec_arg * 8;
                    else return -1;
                }
                i++;                    //increment index to go to next value
            }
        } else if(str_comp(current, "-n") == 0) {
            //check if n_flag has been previously set, if not set to "true"
            if(n_flag == 0) {
                n_flag = 1;
                current = *(argv + (i + 1));
                if(current != NULL)
                    noisefile_arg = current;
                i++;                 //increment index to go to next flag
            }
        } else if(str_comp(current, "-l") == 0) {
            //check if l_flag has been previously set, if not set to "true"
            if(l_flag == 0) {
                l_flag = 1;
                current = *(argv + (i + 1));
                if(extract_int_arg(current, &level_arg) < 0)
                    return -1;
                else if(level_arg < -30 && level_arg > 30)
                    return -1;
                i++;                 //increment index to go to next flag
            }
        } else {
            return -1;
        }
    }
    global_options = GENERATE_OPTION;
    audio_samples = msec_arg;
    noise_file = noisefile_arg;
    noise_level = level_arg;
    return 0;
}

//detect args helper function
int validate_detect_args(int argc, char **argv) {
    int blocksize_arg;
    if(argc > 4)
        return -1;
    else if(argc == 2)
        blocksize_arg = DEFAULT_BLOCK_SIZE;
    else {
        char *current = *(argv + 2);
        if(str_comp(current, "-b") == 0) {
            current = *(argv + 3);
            if(extract_int_arg(current, &blocksize_arg) < 0)
                return -1;
            else if(blocksize_arg < 10 && blocksize_arg > 1000)
                return -1;
        } else return -1;
    }
    global_options = DETECT_OPTION;
    block_size = blocksize_arg;
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
int validargs(int argc, char **argv)
{
    char *cmd = *(argv + 1);
    // TO BE IMPLEMENTED
    if(str_comp(cmd, "-h") == 0) {
        global_options = HELP_OPTION;
        return 0;
    } else if(str_comp(cmd, "-g") == 0) {
        if(validate_generate_args(argc, argv) != 0)
            return -1;
    } else if(str_comp(cmd, "-d") == 0) {
        if(validate_detect_args(argc, argv) != 0)
            return -1;
    } else {
        return -1;
    }
    return 0;
}

