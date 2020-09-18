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
    if(validargs(argc, argv)) {
      //debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
      USAGE(*argv, EXIT_FAILURE);
    }
    if(global_options & 1) {
      //debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
      USAGE(*argv, EXIT_SUCCESS);
    }
    // TO BE IMPLEMENTED
    else if(global_options & GENERATE_OPTION) {
      int generate = dtmf_generate(stdin, stdout, audio_samples);
      //debug("%d", generate);
      //debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
      if(generate == 0)
        return EXIT_SUCCESS;
      else return EXIT_FAILURE;
    }
    else if(global_options & DETECT_OPTION) {
      int detect = dtmf_detect(stdin, stdout);
      //debug("%d", detect);
      //debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
      if(detect == 0)
        return EXIT_SUCCESS;
      else return EXIT_FAILURE;
    }
    //debug("g %d, t %d, file %s, l %d, b %d", global_options, audio_samples, noise_file, noise_level, block_size);
    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
