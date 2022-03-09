/**
 * Author : David Saggiorato
 *
 *  Copyright 2015-2022 GeoLabs SARL. All rights reserved.
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

#include "service.h"
#include "service_internal_amqp.h"
#include <amqp_tcp_socket.h>
#include <amqp_framing.h>

/**
 * AMQP connection state
 */
amqp_connection_state_t conn;
/**
 * AMQP socket
 */
amqp_socket_t *_socket = NULL;
/**
 * AMQP exchange
 */
char * amqp_exchange;
/**
 * AMQP routing key
 */
char * amqp_routingkey;
/**
 * AMQP hostname
 */
char * amqp_hostname;
/**
 * AMQP port
 */
int amqp_port;
/**
 * AMQP user name
 */
char *amqp_user;
/**
 * AMQP password
 */
char *amqp_passwd;
/**
 * Queue's name
 */
char * amqp_queue;
/**
 * Queue's name
 */
amqp_bytes_t amqp_queuename;

/**
 * Initialize amqp parameters:
 * 
 * @param conf the main configuration maps
 * @return 0 on success or > 0 in case of missing parameters
 * @see amqp_hostname
 * @see amqp_port
 * @see amqp_user
 * @see amqp_passwd
 * @see amqp_exchange
 * @see amqp_routingkey
 * @see amqp_queue
 * @see amqp_queuename
 */
int init_amqp(maps* conf){
  //char * amqp_host;
  map * m_amqp_host = getMapFromMaps (conf, "rabbitmq", "host");
  if (m_amqp_host == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] host");
    return 2;
  }
  else {
    amqp_hostname = (char *)malloc((strlen(m_amqp_host->value) +1)*sizeof(char*));
    strncpy(amqp_hostname,m_amqp_host->value,strlen(m_amqp_host->value));
    amqp_hostname[strlen(m_amqp_host->value)] = '\0';
  }

  //int amqp_port;
  map *m_amqp_port = getMapFromMaps (conf, "rabbitmq", "port");
  if (m_amqp_port == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] port");
    return 2;
  }
  else {
    amqp_port = atoi(m_amqp_port->value);
    if (amqp_port == 0){
      fprintf(stderr,"Configuration error: [rabbitmq] port");
      return 2;
    }
  }

  //char * amqp_user;
  map * m_amqp_user = getMapFromMaps (conf, "rabbitmq", "user");
  if (m_amqp_user == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] user");
    return 2;
  }
  else {
    amqp_user = (char *)malloc((strlen(m_amqp_user->value) +1)*sizeof(char*));
    strncpy(amqp_user,m_amqp_user->value,strlen(m_amqp_user->value));
    amqp_user[strlen(m_amqp_user->value)] = '\0';
  }

  //char * amqp_passwd;
  map * m_amqp_passwd = getMapFromMaps (conf, "rabbitmq", "passwd");
  if (m_amqp_passwd == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] passwd");
    return 2;
  }
  else {
    amqp_passwd = (char *)malloc((strlen(m_amqp_passwd->value) +1)*sizeof(char*));
    strncpy(amqp_passwd,m_amqp_passwd->value,strlen(m_amqp_passwd->value));
    amqp_passwd[strlen(m_amqp_passwd->value)] = '\0';
  }

  //char * amqp_exchange;
  map * m_amqp_exchange = getMapFromMaps (conf, "rabbitmq", "exchange");
  if (m_amqp_exchange == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] exchange");
    return 2;
  }
  else {
    amqp_exchange = (char *)malloc((strlen(m_amqp_exchange->value) +1)*sizeof(char*));
    strncpy(amqp_exchange,m_amqp_exchange->value,strlen(m_amqp_exchange->value));
    amqp_exchange[strlen(m_amqp_exchange->value)] = '\0';
  }

  //char * amqp_routingkey;
  map * m_amqp_routingkey = getMapFromMaps (conf, "rabbitmq", "routingkey");
  if (m_amqp_routingkey == NULL){
    fprintf(stderr,"Configuration error: [amqp] routingkey");
    return 2;
  }
  else {
    amqp_routingkey = (char *)malloc((strlen(m_amqp_routingkey->value) +1)*sizeof(char*));
    strncpy(amqp_routingkey,m_amqp_routingkey->value,strlen(m_amqp_routingkey->value));
    amqp_routingkey[strlen(m_amqp_routingkey->value)] = '\0';
  }

  //char * amqp_queue;
  map * m_amqp_queue = getMapFromMaps (conf, "rabbitmq", "queue");
  if (m_amqp_queue == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] queue");
    return 2;
  }
  else {
    amqp_queue = (char *)malloc((strlen(m_amqp_queue->value) +1)*sizeof(char*));
    strncpy(amqp_queue,m_amqp_queue->value,strlen(m_amqp_queue->value));
    amqp_queue[strlen(m_amqp_queue->value)] = '\0';
    amqp_queuename=amqp_cstring_bytes(amqp_queue);
  }
  return 0;
}

/**
 * Initialize a consumer
 *
 */
void init_consumer(){
  amqp_rpc_reply_t s;
  amqp_basic_consume(conn, 1, amqp_queuename, amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  amqp_get_rpc_reply(conn);
  if (s.reply_type != AMQP_RESPONSE_NORMAL){
    fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
    fprintf(stderr,"# %d + Error occured %s %d\n",getpid(),__FILE__,__LINE__);
    fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
    fflush(stderr);
  }
}

/**
 * Consume a message
 *
 * @return envelope.delivery_tag
 */
uint64_t consumer_amqp(char **msg){
    amqp_rpc_reply_t res;
    amqp_envelope_t envelope;
    amqp_maybe_release_buffers(conn);
    res = amqp_consume_message(conn, &envelope, NULL, 0);
    if (AMQP_RESPONSE_NORMAL != res.reply_type) {
      fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
      fprintf(stderr,"# %d ERROR +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
      fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
      fflush(stderr);
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


/**
 * Consumer ack AMQP
 * @return amqp_basic_ack
 */
int consumer_ack_amqp(uint64_t delivery_tag){
  return amqp_basic_ack(conn,1,delivery_tag,0);
}

/**
 * Bind AMQP
 * 
 * @return 0 on success, -2 in case of login failure, -3 in case we cannot open
 * a channel
 */
int bind_amqp(){
  conn = amqp_new_connection();
  _socket = amqp_tcp_socket_new(conn);
  int  status;
  if (!_socket)
    return -1;
  status = amqp_socket_open(_socket, amqp_hostname, amqp_port);
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

/**
 * Initialize confirmation
 * 
 */
void init_confirmation(){
  amqp_rpc_reply_t s;
  amqp_confirm_select_t req;
  req.nowait = 0;
  amqp_simple_rpc_decoded(conn,
			  1,
			  AMQP_CONFIRM_SELECT_METHOD,
			  AMQP_CONFIRM_SELECT_OK_METHOD,
			  &req);
  s = amqp_get_rpc_reply(conn);
  if(s.reply_type != AMQP_RESPONSE_NORMAL){
    fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
    fprintf(stderr,"**** - !!!! - Unable to activate publication confirmation! %s %d\n",__FILE__,__LINE__);
    fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
    fflush(stderr);
  }
}

/**
 * Bind AMQP queue
 *
 * @return 0 in case of success, -3 otherwise
 */
int bind_queue(){
  amqp_rpc_reply_t s;
  amqp_queue_bind(conn, 1, amqp_queuename, amqp_cstring_bytes(amqp_exchange),
                  amqp_cstring_bytes(amqp_routingkey), amqp_empty_table);
  s = amqp_get_rpc_reply(conn);
  if (s.reply_type != AMQP_RESPONSE_NORMAL)
    return -3;  
  return 0;
}

/**
 * Send a message
 *
 * @return 0 in case of success
 */
int send_msg(const char * msg, const char * content_type){
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes(content_type);
    props.delivery_mode = 2; /* persistent delivery mode */
    int ret = amqp_basic_publish(conn,1,amqp_cstring_bytes(amqp_exchange),amqp_cstring_bytes(amqp_routingkey), 0, 0, &props, amqp_cstring_bytes(msg));
    if (ret < 0)
        fprintf(stderr, " %s\n", amqp_error_string2(ret));
    // You then need to wait for either a basic.ack or then a basic.return followed by a message, followed by basic.ack in the case of a delivery failure:
    amqp_frame_t frame;
    amqp_simple_wait_frame(conn, &frame);
    // Check that frame is on channel 1
    if (frame.frame_type == AMQP_FRAME_METHOD && frame.payload.method.id == AMQP_BASIC_ACK_METHOD)
      {
	fprintf(stderr," *-* Message successfully delivered (%s %d)\n",__FILE__,__LINE__ );
	fflush(stderr);
	return 0;
	// Message successfully delivered
      }
    else
      {
	// Read properties
	amqp_simple_wait_frame(conn, &frame);
	// Check that frame is on channel 1, and that its a properties type
	uint64_t body_size = frame.payload.properties.body_size;
	
	uint64_t body_read = 0;
	// Read body frame
	while (body_read < body_size)
	  {
	    amqp_simple_wait_frame(conn, &frame);
	    // Check frame is on channel 1, and that is a body frame
	    body_read += frame.payload.body_fragment.len;
	  }   
	
	// Read basic.ack
	amqp_simple_wait_frame(conn, &frame);
	// Check frame is on channel 1, and that its a basic.ack
	if (frame.frame_type == AMQP_FRAME_METHOD && frame.payload.method.id == AMQP_BASIC_ACK_METHOD)
	  {
	    fprintf(stderr," *-* Message successfully delivered (%s %d)\n",__FILE__,__LINE__ );
	    fflush(stderr);
	    return 0;
	    // Message successfully delivered
	  }
      }
    
    return ret;
}

/**
 * Close the AMQP connection
 *
 * @return 0 in case of success
 */
int close_amqp(){
    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
    return 0;
}

