#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Interface configuration */
struct interface_config {
    char name[64];            /* Interface PCI name */
    char ip[32];              /* Interface IP */
};

/* ST20P video session configuration */
struct st20p_config {
    int replicas;             /* Number of session copies */
    int start_port;           /* Start UDP port */
    int payload_type;         /* RTP payload type */
    int width;                /* Video width */
    int height;               /* Video height */
    char fps[8];              /* Frame rate (e.g., "p59", "p50") */
    char device[16];          /* Device type */
    char output_format[32];   /* Output format */
    char transport_format[32]; /* Transport format */
    bool display;             /* Display enable */
    bool measure_latency;     /* Latency measurement */
};

/* ST30P audio session configuration */
struct st30p_config {
    int replicas;             /* Number of session copies */
    int start_port;           /* Start UDP port */
    int payload_type;         /* RTP payload type */
    char audio_format[16];    /* Audio format */
    char audio_channel[16];   /* Audio channel */
    char audio_sampling[8];   /* Sample rate */
    char audio_ptime[8];      /* Packet time */
};

/* RX session group configuration */
struct rx_session_config {
    char ip[32];              /* Multicast IP */
    int interface_index;      /* Interface array index */
    struct st20p_config st20p; /* ST20P video configuration */
    struct st30p_config st30p; /* ST30P audio configuration */
    bool has_st20p;           /* ST20P session present */
    bool has_st30p;           /* ST30P session present */
};

/* Main configuration structure */
struct rx_config {
    struct interface_config interface; /* Interface configuration */
    struct rx_session_config session;  /* RX session configuration */
};

// Forward declaration for rx_app_context
struct rx_app_context;

// Function to parse JSON configuration file
int parse_rx_config(const char* config_file, struct rx_config* config);

// Helper function to convert fps string to enum value
int fps_string_to_value(const char* fps_str);

// Helper function to validate configuration
int validate_rx_config(const struct rx_config* config);

// Load and return configuration structure
int load_and_validate_rx_config(const char* config_file, struct rx_config* config);

// Apply configuration to application parameters (avoids struct dependency)
int apply_config_to_app(const struct rx_config* config,
                       uint32_t* width, uint32_t* height, int* fps,
                       char* dip_addr_str, size_t dip_size, uint8_t* dip_addr,
                       uint16_t* udp_port, bool* enable_display,
                       char* port, size_t port_size,
                       char* sip_addr_str, size_t sip_size, uint8_t* sip_addr);