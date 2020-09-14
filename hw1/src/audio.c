#include <stdio.h>

#include "audio.h"
#include "debug.h"

int read_bytes(FILE *in, int *result) {     // result = pointer
    int ret_val = 0;
    //traverse 4 bytes at a time
    for(int i = 0; i < 4; i++) {
        //get current byte
        int current = fgetc(in);
        //if current byte is not EOF continue
        if(current != EOF) {
            //OR current value
            ret_val = ret_val | current;
            //and shift left each time
            if(i < 3) {
                ret_val = ret_val << 8;
            }
        }
        //if EOF return -i
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
    //skip data size
    int d_size;
    if(read_bytes(in, &d_size) < 0) {
        return EOF;
    }
    //hp -> data_size = d_size;
    //hp -> data_size = 0xffffffff;
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
    if(m_number == AUDIO_MAGIC && encode == PCM16_ENCODING && s_rate == AUDIO_FRAME_RATE && chan == AUDIO_CHANNELS
        && d_offset >= 24) {
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
    int number = field;
    //shift from letfmost (to the right) then decrement each time until zero
    for(int i = 24; i >= 0;) {
        current = number >> i;
        //put current byte in file
        int output = fputc(current, out);
        //debug("%d",output);
        //if EOF then end
        if(output == EOF)
            return -1;
        //decrement i by one byte
        i = i - 8;
    }
    return 0;
}

int audio_write_header(FILE *out, AUDIO_HEADER *hp) {
    // TO BE IMPLEMENTED
    //check each header element (if EOF) else return 0
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
    //get msb
    int msb = fgetc(in);
    if(msb == EOF)
        return EOF;
    //get lsb
    int lsb = fgetc(in);
    if(lsb == EOF)
        return EOF;
    //combine together into samplep
    *samplep = (msb << 8) | lsb;
    //debug("%x", *samplep);
    return 0;
}

int audio_write_sample(FILE *out, int16_t sample) {
    // TO BE IMPLEMENTED
    int msb = fputc(sample >> 8, out);
    if(msb == EOF)
        return EOF;
    int lsb = fputc(sample, out);
    if(lsb == EOF)
        return EOF;
    //debug("%d", sample);
    return 0;
}
