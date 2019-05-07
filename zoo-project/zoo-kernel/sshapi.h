/*
 *
 * Author : Gérald FENOY
 *
 *  Copyright 2017 GeoLabs SARL. All rights reserved.
 *
 * This work was supported by public funds received in the framework of GEOSUD,
 * a project (ANR-10-EQPX-20) of the program "Investissements d'Avenir" managed 
 * by the French National Research Agency
 *
 * permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef SSHAPI_H
#define SSHAPI_H 1

#pragma once 

#include <libssh2.h>
#include <libssh2_sftp.h>
 
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PARALLEL_SSH_CON 128  

  static int nb_sessions;

  typedef struct {
    int sock_id;
    int index;
    LIBSSH2_SESSION *session;
    LIBSSH2_SFTP *sftp_session;
  } SSHCON;

  
  ZOO_DLL_EXPORT SSHCON *ssh_connect(maps*);
  ZOO_DLL_EXPORT bool ssh_copy(maps*,const char*,const char*,int);
  ZOO_DLL_EXPORT int ssh_get_cnt(maps*);
  ZOO_DLL_EXPORT int ssh_fetch(maps*,const char*,const char*,int);
  ZOO_DLL_EXPORT int ssh_exec(maps*,const char*,int);
  ZOO_DLL_EXPORT bool ssh_close_session(maps*,SSHCON*);
  ZOO_DLL_EXPORT bool ssh_close(maps*);
  ZOO_DLL_EXPORT bool addToUploadQueue(maps**,maps*);
  ZOO_DLL_EXPORT bool runUpload(maps**);
  
#ifdef __cplusplus
}
#endif

#endif
