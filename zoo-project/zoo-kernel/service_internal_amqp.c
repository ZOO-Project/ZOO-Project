/**
 * Author : David Saggiorato
 *
 *  Copyright 2015-2024 GeoLabs SARL. All rights reserved.
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
#include "service_json.h"
#include <amqp_tcp_socket.h>
#include <amqp_framing.h>
#include "json.h"
#include <cmath>
#include <errno.h>

/**
 * AMQP connection state
 */
amqp_connection_state_t conn = NULL;
/**
 * AMQP socket
 */
amqp_socket_t *_socket = NULL;
/**
 * AMQP exchange
 */
char * amqp_exchange = NULL;
/**
 * AMQP routing key
 */
char * amqp_routingkey = NULL;
/**
 * AMQP hostname
 */
char * amqp_hostname = NULL;
/**
 * AMQP port
 */
int amqp_port = 0;
/**
 * AMQP user name
 */
char *amqp_user = NULL;
/**
 * AMQP password
 */
char *amqp_passwd = NULL;
/**
 * Queue's name
 */
char * amqp_queue = NULL;
/**
 * Queue's name
 */
amqp_bytes_t amqp_queuename;

/**
 * Log AMQP error
 *
 * @param context the context of the error
 * @param reply the AMQP reply
 */
static void log_amqp_error(const char *pccContext, amqp_rpc_reply_t reply) {
  ZOO_ERROR("AMQP ERROR in %s reply_type=%d", pccContext, reply.reply_type);

  switch (reply.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      ZOO_ERROR("AMQP_RESPONSE_NORMAL");
      break;

    case AMQP_RESPONSE_NONE:
      ZOO_ERROR("AMQP_RESPONSE_NONE: no RPC reply received");
      break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      {
        ZOO_ERROR("AMQP_RESPONSE_LIBRARY_EXCEPTION: library_error=%d: %s errno=%d errno_text=%s",
                  reply.library_error,
                  amqp_error_string2(reply.library_error),
                  errno,
                  strerror(errno));
      }
      break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      ZOO_ERROR("AMQP_RESPONSE_SERVER_EXCEPTION");

      if (reply.reply.id == AMQP_CONNECTION_CLOSE_METHOD) {
        amqp_connection_close_t *m =
          (amqp_connection_close_t *) reply.reply.decoded;

        ZOO_ERROR("connection.close reply_code=%d reply_text=%.*s class_id=%d method_id=%d",
                  m->reply_code,
                  (int)m->reply_text.len,
                  (char *)m->reply_text.bytes,
                  m->class_id,
                  m->method_id);
      }
      else if (reply.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
        amqp_channel_close_t *m =
          (amqp_channel_close_t *) reply.reply.decoded;
        ZOO_ERROR("channel.close reply_code=%d reply_text=%.*s class_id=%d method_id=%d",
                  m->reply_code,
                  (int)m->reply_text.len,
                  (char *)m->reply_text.bytes,
                  m->class_id,
                  m->method_id);
      }
      else {
        ZOO_ERROR("unknown server exception method id=%d", reply.reply.id);
      }
      break;

    default:
      {
        ZOO_ERROR("unknown AMQP reply_type=%d", reply.reply_type);
      }
      break;
  }
}

/**
 * Set a string value for a configuration parameter.
 *
 * @param ppcTarget Pointer to the target string pointer.
 * @param pccValue The value to set.
 * @param pccName The name of the configuration parameter.
 * @return 0 on success, 2 on error.
 */
static int set_string_value(char **ppcTarget, const char *pccValue, const char *pccName) {
  size_t sLength = 0;
  if (pccValue == NULL) {
    ZOO_ERROR("Configuration error: NULL value for %s", pccName);
    return 2;
  }

  sLength = strlen(pccValue);
  if (sLength == 0) {
    ZOO_ERROR("Configuration error: empty value for %s", pccName);
    return 2;
  }

  if (ppcTarget != NULL && *ppcTarget != NULL) {
    free(*ppcTarget);
    *ppcTarget = NULL;
  }
  *ppcTarget = (char *) malloc(sLength + 1);
  if (*ppcTarget == NULL) {
    ZOO_ERROR("Configuration error: malloc failed for %s length=%zu", pccName, sLength);
    return 2;
  }
  memcpy(*ppcTarget, pccValue, sLength);
  (*ppcTarget)[sLength] = '\0';
  if(ppcTarget == &amqp_queue)
    amqp_queuename=amqp_cstring_bytes(*ppcTarget);
  return 0;
}

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
  const char* pccConfigNames[6] = {
    "host",
    "user",
    "passwd",
    "exchange",
    "routingkey",
    "queue"
  };
  char** apcConfigs[6] = {
    &amqp_hostname,
    &amqp_user,
    &amqp_passwd,
    &amqp_exchange,
    &amqp_routingkey,
    &amqp_queue
  };
  for(int i=0;i<6;i++){
    map * m_amqp_config = getMapFromMaps (conf, "rabbitmq", pccConfigNames[i]);
    if (m_amqp_config == NULL){
      ZOO_ERROR("Configuration error: [rabbitmq] %s", pccConfigNames[i]);
      return 2;
    }
    if(set_string_value(apcConfigs[i], m_amqp_config->value, pccConfigNames[i]) != 0)
      return 2;
  }
  
  map *m_amqp_port = getMapFromMaps (conf, "rabbitmq", "port");
  if (m_amqp_port == NULL){
    ZOO_ERROR("Configuration error: [rabbitmq] port");
    return 2;
  }
  else {
    amqp_port = atoi(m_amqp_port->value);
    if (amqp_port == 0){
      ZOO_ERROR("Configuration error: [rabbitmq] port");
      return 2;
    }
  }
  return 0;
}

/**
 * Initialize a consumer
 *
 * @return 0 on success, -1 otherwise
 */
int init_consumer(){
  amqp_rpc_reply_t s;
  amqp_basic_consume(conn, 1, amqp_queuename, amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  s = amqp_get_rpc_reply(conn);
  if (s.reply_type != AMQP_RESPONSE_NORMAL){
    ZOO_ERROR("amqp_basic_consume failed");
    log_amqp_error("init_consumer/amqp_basic_consume", s);
    return -1;
  }
  return 0;
}

/**
 * Consume a message
 *
 * @return envelope.delivery_tag
 */
uint64_t consumer_amqp(char **msg){
  amqp_rpc_reply_t res;
  amqp_envelope_t envelope;

  while(1){
    // Wait for a message with a timeout of 5 seconds
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    amqp_maybe_release_buffers(conn);
    res = amqp_consume_message(conn, &envelope, &timeout, 0);

    if (AMQP_RESPONSE_NORMAL == res.reply_type) {
      break;
    }

    if (res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION &&
        res.library_error == AMQP_STATUS_TIMEOUT) {
      ZOO_TRACE("Timeout waiting for message");
      continue;
    }

    ZOO_ERROR("amqp_consume_message failed");
    log_amqp_error("consumer_amqp/amqp_consume_message", res);
    return 0;
  }

  unsigned char *buf = (unsigned char *) envelope.message.body.bytes;
  size_t i;
  char * tmp = (char *) malloc(envelope.message.body.len * sizeof(char*));
  if (tmp == NULL) {
    ZOO_ERROR("malloc failed for message size %zu", envelope.message.body.len);
    amqp_destroy_envelope(&envelope);
    return 0;
  }
  memcpy(tmp, envelope.message.body.bytes, envelope.message.body.len);
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
  int ret = amqp_basic_ack(conn, 1, delivery_tag, 0);
  if(ret < 0){
    ZOO_ERROR("ERROR consumer_ack_amqp: ack failed");
  }
  return ret;
}

/**
 * Bind AMQP
 * 
 * @return 0 on success, -2 in case of login failure, -3 in case we cannot open
 * a channel
 */
int bind_amqp(){
  if(conn != NULL) {
    ZOO_WARNING("existing conn=%p found, closing before reconnect", conn);
    close_amqp();
  }
  conn = amqp_new_connection();
  if(conn == NULL){
    ZOO_ERROR("amqp_new_connection failed");
    return -1;
  }
  _socket = amqp_tcp_socket_new(conn);
  if (_socket == NULL) {
    ZOO_ERROR("amqp_tcp_socket_new failed");
    return -1;
  }
  int  status;
  status = amqp_socket_open(_socket, amqp_hostname, amqp_port);
  if (status != 0){
    ZOO_ERROR("amqp_socket_open failed host=%s port=%d status=%d error=%s errno=%d errno_text=%s",
            amqp_hostname,
            amqp_port,
            status,
            amqp_error_string2(status),
            errno,
            strerror(errno));
    amqp_destroy_connection(conn);
    conn = NULL;
    _socket = NULL;
    return status;
  }
  amqp_rpc_reply_t s = amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, amqp_user, amqp_passwd);
  if (s.reply_type != AMQP_RESPONSE_NORMAL){
    ZOO_ERROR("amqp_login failed");
    log_amqp_error("bind_amqp/amqp_login", s);
    amqp_destroy_connection(conn);
    conn = NULL;
    _socket = NULL;
    return -2;
  }
  amqp_channel_open(conn, 1);
  s = amqp_get_rpc_reply(conn);
  if (s.reply_type != AMQP_RESPONSE_NORMAL){
    ZOO_ERROR("channel open failed");
    log_amqp_error("bind_amqp/amqp_channel_open", s);
    close_amqp();
    return -3;
  }
  // Set basic QOS
  amqp_basic_qos(conn, 1, 0, 1, 0);
  s = amqp_get_rpc_reply(conn);
  if (s.reply_type != AMQP_RESPONSE_NORMAL) {
    ZOO_ERROR("basic_qos failed");
    log_amqp_error("bind_amqp/amqp_basic_qos", s);
    close_amqp();
    return -4;
  }
  return 0;
}

/**
 * Initialize confirmation
 * 
 */
int init_confirmation(){
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
    ZOO_ERROR("Unable to activate publication confirmation");
    log_amqp_error("init_confirmation/amqp_confirm_select", s);
    return -1;
  }
  return 0;
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
  if (s.reply_type != AMQP_RESPONSE_NORMAL){
    ZOO_ERROR("queue_bind failed queue=%s exchange=%s routingkey=%s",
              amqp_queue,
              amqp_exchange,
              amqp_routingkey);
    log_amqp_error("bind_queue/amqp_queue_bind", s);
    return -3;
  }
  return 0;
}

/**
 * Wait for a frame and check for errors
 *
 * @param pccContext Contextual information for logging
 * @param frame Pointer to the frame structure
 * @return 0 in case of success, negative value otherwise
 */
int wait_frame_checked(const char *pccContext, amqp_frame_t *frame) {
  int wait_status = amqp_simple_wait_frame(conn, frame);
  if (wait_status < 0) {
    ZOO_ERROR("%s: amqp_simple_wait_frame failed ret=%d error=%s errno=%d errno_text=%s",
              pccContext,
              wait_status,
              amqp_error_string2(wait_status),
              errno,
              strerror(errno));
    return wait_status;
  }
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
  int ret = amqp_basic_publish(conn,
                               1,
                               amqp_cstring_bytes(amqp_exchange),
                               amqp_cstring_bytes(amqp_routingkey),
                               0,
                               0,
                               &props,
                               amqp_cstring_bytes(msg));
  if (ret < 0){
    fprintf(stderr, " %s\n", amqp_error_string2(ret));
    ZOO_ERROR("send_msg: amqp_basic_publish failed");
    return ret;
  }
  // You then need to wait for either a basic.ack or then a basic.return
  // followed by a message, followed by basic.ack in the case of a delivery
  // failure:
  amqp_frame_t frame;
  int wait_status;
  if( (wait_status = wait_frame_checked("send_msg", &frame)) < 0)
    return wait_status;
  // Check that frame is on channel 1
  if (frame.frame_type == AMQP_FRAME_METHOD && 
      frame.payload.method.id == AMQP_BASIC_ACK_METHOD)
    {
      // Message successfully delivered
      ZOO_SUCCESS("Message successfully delivered.");
      return 0;
    }
  else
    {
      // Read properties
      if( (wait_status = wait_frame_checked("send_msg", &frame)) < 0)
        return wait_status;
      // Check that frame is on channel 1, and that its a properties type
      uint64_t body_size = frame.payload.properties.body_size;

      uint64_t body_read = 0;
      // Read body frame
      while (body_read < body_size)
        {
          if( (wait_status = wait_frame_checked("send_msg", &frame)) < 0)
            return wait_status;
          // Check frame is on channel 1, and that is a body frame
          if(frame.frame_type == AMQP_FRAME_BODY){
            body_read += frame.payload.body_fragment.len;  
          }else{
            ZOO_ERROR("send_msg: expected body frame while reading returned"
                      " message, got frame_type=%d", frame.frame_type);
            break;
          }
        }

      // Read basic.ack
      if( (wait_status = wait_frame_checked("send_msg", &frame)) < 0)
        return wait_status;
      // Check frame is on channel 1, and that its a basic.ack
      if (frame.frame_type == AMQP_FRAME_METHOD &&
          frame.payload.method.id == AMQP_BASIC_ACK_METHOD)
        {
          // Message successfully delivered
          ZOO_SUCCESS("Message successfully delivered.");
          return 0;
        }
    }
  ZOO_ERROR("publisher confirm did not end with basic.ack frame_type=%d method=%d",
            frame.frame_type,
            frame.frame_type == AMQP_FRAME_METHOD ? frame.payload.method.id : 0);
  return ret;
}

/**
 * Publish a message in RabbitMQ
 *
 * @param pmsConf the maps pointing to the main configuration file
 * @param pmRequest the request as it has been parsed till now
 * @param pmsInpuits the inputs maps
 * @param pmsOutpuits the outputs maps
 */
void publish_amqp_msg(maps* pmsConf,int* eres,map* pmRequest,maps* pmsInputs,maps* pmsOututs){
  init_amqp(pmsConf);
  *eres = SERVICE_ACCEPTED;
  json_object *poMsg = json_object_new_object();

  maps* pmsLenv=getMaps(pmsConf,"lenv");
  json_object *maps1_obj = mapToJson(pmsLenv->content);
  json_object_object_add(poMsg,"main_lenv",maps1_obj);

  pmsLenv=getMaps(pmsConf,"main");
  if(pmsLenv!=NULL){
    json_object *poMain = mapToJson(pmsLenv->content);
    json_object_object_add(poMsg,"main_main",poMain);
  }
  pmsLenv=getMaps(pmsConf,"subscriber");
  if(pmsLenv!=NULL){
    json_object *poSubscriber = mapToJson(pmsLenv->content);
    json_object_object_add(poMsg,"main_subscriber",poSubscriber);
  }
  maps* pmsRequests=getMaps(pmsConf,"http_requests");
  if(pmsRequests!=NULL){
    json_object *poRequests = mapToJson(pmsRequests->content);
    json_object_object_add(poMsg,"main_http_requests",poRequests);
  }

  map* pmListSections=getMapFromMaps(pmsConf,"servicesNamespace","sections_list");
  if(pmListSections!=NULL) {
    char* pcaListSections=zStrdup(pmListSections->value);
    char *saveptr;
    char *tmps = strtok_r (pcaListSections, ",", &saveptr);
    while(tmps!=NULL){
      maps* pmsTmp=getMaps(pmsConf,tmps);
      if(pmsTmp!=NULL){
        json_object *poRequests = mapToJson(pmsTmp->content);
        char* pcaKey=(char*)malloc((strlen(tmps)+6)*sizeof(char));
        sprintf(pcaKey,"main_%s",tmps);
        json_object_object_add(poMsg,pcaKey,poRequests);
      }
      tmps = strtok_r (NULL, ",", &saveptr);
    }
    free(pcaListSections);
  }

  json_object *req_format_jobj = mapsToJson(pmsInputs);
  json_object_object_add(poMsg,"request_input_real_format",req_format_jobj);

  json_object *req_jobj = mapToJson(pmRequest);
  json_object_object_add(poMsg,"request_inputs",req_jobj);

  json_object *outputs_jobj = mapsToJson(pmsOututs);
  json_object_object_add(poMsg,"request_output_real_format",outputs_jobj);

  int iBindStatus = bind_amqp();
  if (iBindStatus != 0) {
    ZOO_ERROR("publish_amqp_msg: bind_amqp failed status=%d", iBindStatus);
    *eres = SERVICE_FAILED;
    json_object_put(poMsg);
    return;
  }

  init_confirmation();
  if ( (send_msg(json_object_to_json_string_ext(poMsg,JSON_C_TO_STRING_PLAIN),
		   "application/json") != 0) ){
    ZOO_ERROR("publish_amqp_msg: send_msg failed");
    *eres = SERVICE_FAILED;
  }
  close_amqp();
  json_object_put(poMsg);
}

/**
 * Close the AMQP connection
 *
 * @return 0 in case of success
 */
int close_amqp(){
  if (conn != NULL) {
    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
    conn = NULL;
  }
  _socket = NULL;
  return 0;
}

