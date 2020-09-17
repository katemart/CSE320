#include <stdio.h>
#include <stdlib.h>

#include "const.h"
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

int main(int argc, char **argv)
{
    //AUDIO_HEADER header;
    //FILE *fp = fopen("./rsrc/dtmf_0_500ms.au", "r");
    //FILE *fp = fopen("test.au", "r");
    //FILE *fp = fopen("./rsrc/dtmf_0_500ms.txt", "r");
    //FILE *fp = fopen("./rsrc/dtmf_all.txt", "r");
    //int read = audio_read_header(fp, &header);
    //debug("return value of audio header %d", read);
    //debug("magic %d, offset %d, size %d, encoding %d, rate %d, channels %d", header.magic_number,
    //    header.data_offset, header.data_size, header.encoding, header.sample_rate, header.channels);
    //fputc('a', stdout);

   /*int write = audio_write_header(stdout, &header);
   debug("return value of audio write %d", write);

   int16_t sample;
   int read_sample = audio_read_sample(fp, &sample);
   debug("return value of sample read %d and sample %d", read_sample, sample);

   int write_sample = audio_write_sample(stdout, sample/2);
   debug("return value of sample write %d and sample %d", write_sample, sample);*/

    if(validargs(argc, argv)) {
    	debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
    	USAGE(*argv, EXIT_FAILURE);
    }
    if(global_options & 1) {
    	debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
        USAGE(*argv, EXIT_SUCCESS);
    }
    // TO BE IMPLEMENTED
    if(global_options & GENERATE_OPTION) {
      int generate = dtmf_generate(stdin, stdout, audio_samples);
      debug("return value of generate %d", generate);
      debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
    }
    if(global_options & DETECT_OPTION) {
      int detect = dtmf_detect(stdin, stdout);
      debug("return value of detect %d", detect);
      debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
    }
    debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
