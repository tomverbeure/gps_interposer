#ifndef M12_PARSER_H
#define M12_PARSER_H

#include <stdio.h>
#include <string.h>
#include <time.h>

class M12Parser
{
private:
    char    tx_buf[1024];
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
    
    int get_msg_len(char *msg_id, bool req)
    {
        if (strcmp(msg_id, "Aw") == 0){
            return req ? 8 : 8;
        }
        if (strcmp(msg_id, "Bp") == 0){
            return req ? 8 : -1;
        }
        if (strcmp(msg_id, "Bj") == 0){
            return req ? 8 : 8;
        }
        if (strcmp(msg_id, "Cf") == 0){
            return req ? 7 : 7;
        }
        if (strcmp(msg_id, "Co") == 0){
            return req ? -1 : 29;
        }
        if (strcmp(msg_id, "Gd") == 0){
            return req ? 8 : 8;
        }
        if (strcmp(msg_id, "Ge") == 0){
            return req ? 8 : 8;
        }
        if (strcmp(msg_id, "Gf") == 0){
            return req ? 9 : 9;
        }
        if (strcmp(msg_id, "Gj") == 0){
            return req ? 7 : 21;
        }
        if (strcmp(msg_id, "Ha") == 0){
            return req ? 8 : 154;
        }
        if (strcmp(msg_id, "Hn") == 0){
            return req ? 8 : 78;
        }
        return -1;
    }
    
    char calc_checksum(char msg[], int start, int end)
    {
        char checksum   = 0;
    
        for(int i=start;i<=end;++i){
            checksum ^= msg[i];
        }
    
        return checksum;
    }

public:
    M12Parser()
    {
        offset      = 0;
        exp_msg_len = 0;
        checksum    = 0;
    }

    void parse(char c)
    {
        tx_buf[offset++] = c;

        switch(state){
            case WAIT_FIRST_0X40:{
                offset = 0;

                if (c == 0x40){
                    tx_buf[offset++] = c;
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
                exp_msg_len         = get_msg_len(msg_id, false);

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

                    if (strcmp(msg_id, "Ha") == 0){
                        struct tm date  = { 0 };

                        // Extract time from packet
                        date.tm_mday    = tx_buf[4];
                        date.tm_mon     = tx_buf[5] - 1;
                        date.tm_year    = (tx_buf[6] * 256) + tx_buf[7] - 1900; // Year since 1900

                        // Add 1024 weeks by adding the number of days. 
                        date.tm_mday    += 1024 * 7;

                        // Normalize...
                        time_t time     = mktime(&date);
                    
                        // Extract date
                        struct tm new_date; 
                        localtime_r(&time, &new_date);

                        tx_buf[4]       = new_date.tm_mday;
                        tx_buf[5]       = new_date.tm_mon + 1;

                        int new_year    = new_date.tm_year + 1900;
                        tx_buf[6]       = new_year >> 8;
                        tx_buf[7]       = new_year & 255;
                    }

                    char new_checksum = calc_checksum(tx_buf, 2, offset-4);

                    tx_buf[offset-3]    = new_checksum;

                    //xmit_msg(UART1_ID, tx_buf, offset);
                }
                else{
                    state               = WAIT_FIRST_0X40;
                }
                break;
            }
        }

    }
};


#endif
