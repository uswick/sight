/****************************************************************************
 * Copyright � 2003-2012 Dorian C. Arnold, Philip C. Roth, Barton P. Miller *
 *                  Detailed MRNet usage rights in "LICENSE" file.          *
 ****************************************************************************/

#if !defined(integer_addition_h )
#define integer_addition_h 1

#include "mrnet/Types.h"
#include <unistd.h>
#include <mrnet/Stream.h>

typedef enum { PROT_EXIT=FirstApplicationTag, PROT_END_PHASE,
    PROT_CONCAT
} Protocol;

size_t populateBuffer(char *sendAr, FILE* f);

class DataPckt{
private:
    std::vector<char> char_array;
    bool final_packet;

public:
    DataPckt(char* data_ar, int length, bool final): final_packet(final){
        insertData(data_ar, length);
    };

    void insertData(char* data_ar, int length){
        for (int j = 0; j < length; j++) {
//            printf("%c",data_ar[j]);
            this->char_array.push_back(data_ar[j]);
        }
    };

    std::vector<char>& getData(){
        return  char_array;
    }

    void printData(){
        std::vector<char>::iterator it = char_array.begin();
        printf("[PRINT METHOD !!...pid : %d arr:size : %d ] \n", getpid(), char_array.size());
        while(it != char_array.end()){
            printf("%c",*it);
            it++;
        }
        printf("\n");
    }

    bool isFinal(){
        return final_packet;
    }
};

#define TOTAL_PACKET_SIZE 10000
//#define TOTAL_PACKET_SIZE 20
#define TOTAL_STREAM_SIZE 10000

//#define DEBUG_ON

#endif /* integer_addition_h */
