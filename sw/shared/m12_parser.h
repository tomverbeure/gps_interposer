#ifndef M12_PARSER_H
#define M12_PARSER_H

#include <stdio.h>
#include <string.h>
#include <time.h>

int bytes_to_int16(const char data[])
{
    return (data[1] + (data[0] << 8));
}

int bytes_to_int32(const char data[])
{
    return (data[3] + (data[2] << 8) + (data[1] << 16)+ (data[0] << 24));
}

class HaRespData
{
public:
    int         month; 
    int         day; 
    int         year; 

    int         hours;
    int         minutes;
    int         seconds;
    int         frac_seconds;

    int         lat;
    int         lon;
    int         height;
    int         msl_height;

    int         lat_unfilt;
    int         lon_unfilt;
    int         height_unfilt;
    int         msl_height_unfilt;

    int         speed3d;
    int         speed2d;
    int         heading;

    int         geometry;

    int         sats_visible;
    int         sats_tracked;

    void decode(const char data[])
    {
        int offset      = 4;

        month               = data[offset];
        day                 = data[offset+1];
        year                = bytes_to_int16(&data[offset+2]);
        offset += 4;

        hours               = data[offset];
        minutes             = data[offset+1];
        seconds             = data[offset+2];
        frac_seconds        = bytes_to_int32(&data[offset+3]);
        offset += 7;

        lat                 = bytes_to_int32(&data[offset+0]);
        lon                 = bytes_to_int32(&data[offset+4]);
        height              = bytes_to_int32(&data[offset+8]);
        msl_height          = bytes_to_int32(&data[offset+12]);
        offset += 16;

        lat_unfilt          = bytes_to_int32(&data[offset+0]);
        lon_unfilt          = bytes_to_int32(&data[offset+4]);
        height_unfilt       = bytes_to_int32(&data[offset+8]);
        msl_height_unfilt   = bytes_to_int32(&data[offset+12]);
        offset += 16;

        speed3d             = bytes_to_int16(&data[offset+0]);
        speed2d             = bytes_to_int16(&data[offset+2]);
        heading             = bytes_to_int16(&data[offset+4]);
        offset += 6;

        geometry            = bytes_to_int16(&data[offset]);
        offset += 2;

        sats_visible        = data[offset+0];
        sats_tracked        = data[offset+1];
        offset += 2;
    }
};

class M12Parser
{
public:
    bool    req_mode;

    char    buf[1024];
    char    msg_id[10] = "\0\0\0";

    char    checksum;
    int     offset;
    int     exp_msg_len;
    
    enum eState {
        WAIT_FIRST_0X40     = 0,
        WAIT_SECOND_0X40    = 1,
        MESSAGE_ID0         = 2,
        MESSAGE_ID1         = 3,
        MESSAGE_DATA        = 4,
        MESSAGE_CHECKSUM    = 5,
        MESSAGE_TERMINATOR0 = 6,
        MESSAGE_TERMINATOR1 = 7
    };

    enum eState state = WAIT_FIRST_0X40;
    

public:
    M12Parser(bool _req_mode)
    {
        req_mode    = _req_mode;

        offset      = 0;
        exp_msg_len = 0;
        checksum    = 0;
    }

    const char *get_msg_id()
    {
        return msg_id;
    }

    char calc_checksum(char msg[], int start, int end)
    {
        char checksum   = 0;
    
        for(int i=start;i<=end;++i){
            checksum ^= msg[i];
        }
    
        return checksum;
    }

    int get_msg_len(const char *msg_id)
    {
        if (strcmp(msg_id, "Aw") == 0){
            return req_mode ? 8 : 8;
        }
        if (strcmp(msg_id, "Bp") == 0){
            return req_mode ? 8 : -1;
        }
        if (strcmp(msg_id, "Bj") == 0){
            return req_mode ? 8 : 8;
        }
        if (strcmp(msg_id, "Cf") == 0){
            return req_mode ? 7 : 7;
        }
        if (strcmp(msg_id, "Co") == 0){
            return req_mode ? -1 : 29;
        }
        if (strcmp(msg_id, "Gd") == 0){
            return req_mode ? 8 : 8;
        }
        if (strcmp(msg_id, "Ge") == 0){
            return req_mode ? 8 : 8;
        }
        if (strcmp(msg_id, "Gf") == 0){
            return req_mode ? 9 : 9;
        }
        if (strcmp(msg_id, "Gj") == 0){
            return req_mode ? 7 : 21;
        }
        if (strcmp(msg_id, "Ha") == 0){
            return req_mode ? 8 : 154;
        }
        if (strcmp(msg_id, "Hn") == 0){
            return req_mode ? 8 : 78;
        }
        return -1;
    }
    

    bool parse(char c)
    {
        buf[offset++] = c;

        switch(state){
            case WAIT_FIRST_0X40:{
                offset = 0;

                if (c == 0x40){
                    buf[offset++] = c;
                    state = WAIT_SECOND_0X40;
                }
                break;
            }

            case WAIT_SECOND_0X40:{
                if (c == 0x40){
                    checksum    = 0x00;
                    state = MESSAGE_ID0;
                }
                else{
                    state = WAIT_FIRST_0X40;
                }
                break;
            }

            case MESSAGE_ID0:{
                msg_id[0]           = c;
                checksum            ^= c;
                state               = MESSAGE_ID1;
                break;
            }

            case MESSAGE_ID1:{
                msg_id[1]           = c;
                checksum            ^= c;
                exp_msg_len         = get_msg_len(msg_id);

                if (exp_msg_len == -1){
                    state       = WAIT_FIRST_0X40;
                }
                else if (exp_msg_len == 7){
                    state       = MESSAGE_CHECKSUM;
                }
                else{
                    state       = MESSAGE_DATA;
                }
                break;
            }
            case MESSAGE_DATA:{
                checksum            ^= c;

                if (offset == exp_msg_len-3){
                    state       = MESSAGE_CHECKSUM;
                }
                break;
            }
            case MESSAGE_CHECKSUM:{
                if (c == checksum){
                    state               = MESSAGE_TERMINATOR0;
                }
                else{
                    state               = WAIT_FIRST_0X40;
                }
                break;
            }

            case MESSAGE_TERMINATOR0:{
                if (c == 0x0d){
                    state               = MESSAGE_TERMINATOR1;
                }
                else{
                    state               = WAIT_FIRST_0X40;
                }
                break;
            }

            case MESSAGE_TERMINATOR1:{
                if (c == 0x0a){
                    state               = WAIT_FIRST_0X40;
                    return true;
                }
                else{
                    state               = WAIT_FIRST_0X40;
                }
                break;
            }
        }

        return false;
    }
};


#endif
