/* File : thread.h
          functions for starting a new thread
*/

#ifndef THREAD_H
#define THREAD_H

long thread(char *newname, int (*func)(), long arg);

#endif
