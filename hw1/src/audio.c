#include <stdio.h>

#include "audio.h"
#include "debug.h"

int read_bytes(FILE *in, int *result) {     // result = pointer
    int ret_val = 0;
    for(int i = 0; i < 4; i++) {
        int current = fgetc(in);
        if(current != EOF) {
            //debug("%d current", current);
            ret_val = ret_val | current;
            if(i < 3) {
                ret_val = ret_val << 8;
            }
            //debug("%d, ret_val", ret_val);
            //debug("%d, audio magic", AUDIO_MAGIC);
        }
        else {
            return -1;
        }
    }
    *result = ret_val;
    return 0;
}

int audio_read_header(FILE *in, AUDIO_HEADER *hp) {
    // TO BE IMPLEMENTED
    //get magic number
    int m_number;
    if(read_bytes(in, &m_number) < 0) {
        return EOF;
    }
    hp -> magic_number = m_number;
    //get data offset
    int d_offset;
    if(read_bytes(in, &d_offset) < 0) {
        return EOF;
    }
    hp -> data_offset = d_offset;
    //get data size
    int d_size;
    if(read_bytes(in, &d_size) < 0) {
        return EOF;
    }
    hp -> data_size = d_size;
    //get encoding
    int encode;
    if(read_bytes(in, &encode) < 0) {
        return EOF;
    }
    hp -> encoding = encode;
    //get sample rate
    int s_rate;
    if(read_bytes(in, &s_rate) < 0) {
        return EOF;
    }
    hp -> sample_rate = s_rate;
    //get channels
    int chan;
    if(read_bytes(in, &chan) < 0) {

    }
    hp -> channels = chan;
    //check if the header is valid
    if(m_number == AUDIO_MAGIC && encode == PCM16_ENCODING && chan == AUDIO_CHANNELS) {
        //move pointer to start of data (offset - header = annotation)
        //current pointer is at end of header, now move to after annotations end (which is start of data)
        int annotations = d_offset - 24;
        for(int i = 0; i < annotations; i++) {
            fgetc(in);
        }
        return 0;
    } else return EOF;
}

int write_bytes(FILE *out, int field) {
    int current;
    int m_number = field;
    for(int i = 24; i >= 0;) {
        current = m_number >> i;
        int output = fputc(current, out);
        //debug("%d",output);
        if(output == EOF)
            return -1;
        i = i- 8;
    }
    return 0;
}

int audio_write_header(FILE *out, AUDIO_HEADER *hp) {
    // TO BE IMPLEMENTED
    if(write_bytes(out, hp -> magic_number) < 0)
        return EOF;
    if(write_bytes(out, hp -> data_offset) < 0)
        return EOF;
    if(write_bytes(out, hp -> data_size) < 0)
        return EOF;
    if(write_bytes(out, hp -> encoding) < 0)
        return EOF;
    if(write_bytes(out, hp -> sample_rate) < 0)
        return EOF;
    if(write_bytes(out, hp -> channels) < 0)
        return EOF;
    return 0;
}

int audio_read_sample(FILE *in, int16_t *samplep) {
    // TO BE IMPLEMENTED
    return EOF;
}

int audio_write_sample(FILE *out, int16_t sample) {
    // TO BE IMPLEMENTED
    return EOF;
}
