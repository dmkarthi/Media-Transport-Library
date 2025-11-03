#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "mtl_api.h"
#include "st_pipeline_api.h"

/* Application context for TX sessions */
struct tx_app_context {
  /* MTL library handle */
  mtl_handle mtl;

  /* Configuration */
  char port[MTL_PORT_MAX_LEN];
  char tx_url[256];
  char config_file[256];
  char sip_addr_str[INET_ADDRSTRLEN];
  uint8_t sip_addr[MTL_IP_ADDR_LEN];
  char dip_addr_str[INET_ADDRSTRLEN];
  uint8_t dip_addr[MTL_IP_ADDR_LEN];
  uint16_t udp_port;

  /* Video parameters */
  uint32_t width;
  uint32_t height;
  enum st_fps fps;
  enum st_frame_fmt fmt;

  /* Session controls */
  int st20p_sessions;
  int st30p_sessions;
  bool exit;
  bool force_dhcp;
  int test_time_s;
};