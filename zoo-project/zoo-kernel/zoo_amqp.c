/**
 * Author : David Saggiorato
 *
 *  Copyright 2008-2009 GeoLabs SARL. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>


amqp_connection_state_t conn;
amqp_socket_t *socket = NULL;
char * amqp_exchange;
char * amqp_routingkey;
char * amqp_hostname;
int amqp_port;
char *amqp_user;
char *amqp_passwd;
char * amqp_queue;


void init_amqp(const char * hostname, int port,const char *user, const char *passwd,const char *exchange, const char * routingkey,const char * queue){
    amqp_hostname = strdup(hostname);
    amqp_user = strdup(user);
    amqp_passwd = strdup(passwd);
    amqp_exchange = strdup(exchange);
    amqp_routingkey = strdup(routingkey);
    amqp_queue = strdup(queue);
    amqp_port = port;
}

void init_consumer(){
    amqp_basic_consume(conn, 1, amqp_cstring_bytes(amqp_queue), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    amqp_get_rpc_reply(conn);
}

uint64_t consumer_amqp(char **msg){
    amqp_rpc_reply_t res;
    amqp_envelope_t envelope;
    amqp_maybe_release_buffers(conn);
    res = amqp_consume_message(conn, &envelope, NULL, 0);
    if (AMQP_RESPONSE_NORMAL != res.reply_type) {
        return 0;
    }
    unsigned char *buf = (unsigned char *) envelope.message.body.bytes;
    size_t i;
    char * tmp = (char *) malloc(envelope.message.body.len * sizeof(char*));
    for (i = 0; i < envelope.message.body.len; i++) {
        tmp[i] = (char)buf[i];
    }
    tmp[envelope.message.body.len]='\0';
    *msg = tmp;
    uint64_t r = envelope.delivery_tag;
    amqp_destroy_envelope(&envelope);
    return r;
}


int consumer_ack_amqp(uint64_t delivery_tag){
    return amqp_basic_ack(conn,1,delivery_tag,0);
}



int bind_amqp(){
    conn = amqp_new_connection();
    socket = amqp_tcp_socket_new(conn);
    int  status;
    if (!socket)
        return -1;
    status = amqp_socket_open(socket, amqp_hostname, amqp_port);
    if (status != 0)
        return status;
    amqp_rpc_reply_t s = amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, amqp_user, amqp_passwd);
    if (s.reply_type != AMQP_RESPONSE_NORMAL)
        return -2;
    amqp_channel_open(conn, 1);
    s = amqp_get_rpc_reply(conn);
    if (s.reply_type != AMQP_RESPONSE_NORMAL)
        return -3;
    return 0;

}

int send_msg(const char * msg, const char * content_type){
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes(content_type);
    props.delivery_mode = 2; /* persistent delivery mode */
    int ret = amqp_basic_publish(conn,1,amqp_cstring_bytes(amqp_exchange),amqp_cstring_bytes(amqp_routingkey), 0, 0, &props, amqp_cstring_bytes(msg));
    if (ret < 0)
        fprintf(stderr, " %s\n", amqp_error_string2(ret));
    return ret;
}

int close_amqp(){
    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
    return 0;
}

