//
// Created on 1/19/16.
//

#ifndef MINI_DMTCP2_CKPT_H
#define MINI_DMTCP2_CKPT_H

__attribute__ ((constructor)) void init_signal();
int checkpointer();
char * readSelfMaps();
int getHeaders();
int initStructs(int header_num);
void saveToDisk(char * file_name, int struct_num);
unsigned long long readhex (char *value);

#endif //MINI_DMTCP2_CKPT_H
