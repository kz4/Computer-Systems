//
// Created on 1/19/16.
//

#ifndef MINI_DMTCP2_MYRESTART_H
#define MINI_DMTCP2_MYRESTART_H

void freeCurrentStackAndRestoreMemory(char *ckpt_image);
void freeCurrentStack();
void restore_memory(char * path);

#endif //MINI_DMTCP2_MYRESTART_H
