#include "config_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

/* Helper function to find JSON section */
static char* find_json_section(const char* json, const char* section_name) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", section_name);

    char* section_pos = strstr(json, search_pattern);
    if (!section_pos) {
        return NULL;
    }

    /* Find the colon and opening bracket */
    char* colon_pos = strchr(section_pos, ':');
    if (!colon_pos) {
        return NULL;
    }

    /* Skip whitespace after colon */
    char* bracket_pos = colon_pos + 1;
    while (*bracket_pos && isspace(*bracket_pos)) {
        bracket_pos++;
    }

    if (*bracket_pos == '[') {
        return bracket_pos + 1; /* Return position after [ */
    } else if (*bracket_pos == '{') {
        return bracket_pos; /* Return position at { */
    }

    return NULL;
}

/* Helper function to extract string value from JSON object */
static char* extract_json_string(const char* json_obj, const char* key, char* output, size_t max_len) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);

    char* key_pos = strstr(json_obj, search_pattern);
    if (!key_pos) {
        return NULL;
    }

    /* Find the colon after the key */
    char* colon_pos = strchr(key_pos, ':');
    if (!colon_pos) {
        return NULL;
    }

    /* Skip whitespace after colon */
    char* value_start = colon_pos + 1;
    while (*value_start && isspace(*value_start)) {
        value_start++;
    }

    /* Check if value is a string (starts with quote) */
    if (*value_start != '"') {
        return NULL;
    }

    /* Find the start and end of the string value */
    value_start++; /* Skip opening quote */
    char* value_end = strchr(value_start, '"');
    if (!value_end) {
        return NULL;
    }

    /* Copy the string value */
    size_t len = value_end - value_start;
    if (len >= max_len) {
        len = max_len - 1;
    }

    strncpy(output, value_start, len);
    output[len] = '\0';

    return output;
}

/* Helper function to extract integer value from JSON object */
static int extract_json_int(const char* json_obj, const char* key) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);

    char* key_pos = strstr(json_obj, search_pattern);
    if (!key_pos) {
        return -1;
    }

    /* Find the colon after the key */
    char* colon_pos = strchr(key_pos, ':');
    if (!colon_pos) {
        return -1;
    }

    /* Skip whitespace after colon */
    char* value_start = colon_pos + 1;
    while (*value_start && isspace(*value_start)) {
        value_start++;
    }

    /* Parse integer */
    return atoi(value_start);
}

/* Helper function to extract boolean value from JSON object */
static bool extract_json_bool(const char* json_obj, const char* key) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);

    char* key_pos = strstr(json_obj, search_pattern);
    if (!key_pos) {
        return false;
    }

    /* Find the colon after the key */
    char* colon_pos = strchr(key_pos, ':');
    if (!colon_pos) {
        return false;
    }

    /* Skip whitespace after colon */
    char* value_start = colon_pos + 1;
    while (*value_start && isspace(*value_start)) {
        value_start++;
    }

    /* Check for "true" */
    if (strncmp(value_start, "true", 4) == 0) {
        return true;
    }

    return false;
}

/* Helper function to extract IP from array format */
static char* extract_ip_from_array(const char* json_obj, const char* key, char* output, size_t max_len) {
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);

    char* key_pos = strstr(json_obj, search_pattern);
    if (!key_pos) {
        return NULL;
    }

    /* Find the colon and opening bracket */
    char* colon_pos = strchr(key_pos, ':');
    if (!colon_pos) {
        return NULL;
    }

    /* Skip whitespace after colon */
    char* bracket_pos = colon_pos + 1;
    while (*bracket_pos && isspace(*bracket_pos)) {
        bracket_pos++;
    }

    if (*bracket_pos != '[') {
        return NULL;
    }

    /* Skip to first element */
    char* value_start = bracket_pos + 1;
    while (*value_start && isspace(*value_start)) {
        value_start++;
    }

    if (*value_start != '"') {
        return NULL;
    }

    /* Extract the first IP address */
    value_start++; /* Skip opening quote */
    char* value_end = strchr(value_start, '"');
    if (!value_end) {
        return NULL;
    }

    size_t len = value_end - value_start;
    if (len >= max_len) {
        len = max_len - 1;
    }

    strncpy(output, value_start, len);
    output[len] = '\0';

    return output;
}

/* Helper function to find first object in array */
static char* find_first_array_object(const char* json_array) {
    const char* pos = json_array;

    /* Skip whitespace */
    while (*pos && isspace(*pos)) {
        pos++;
    }

    /* Find first { */
    while (*pos && *pos != '{') {
        pos++;
    }

    return (char*)pos;
}

/* Parse RX configuration from JSON file */
int parse_rx_config(const char* config_file, struct rx_config* config) {
    if (!config_file || !config) {
        return -1;
    }

    /* Initialize config structure */
    memset(config, 0, sizeof(struct rx_config));

    /* Read JSON file */
    FILE* file = fopen(config_file, "r");
    if (!file) {
        printf("Error: Cannot open config file %s\n", config_file);
        return -1;
    }

    /* Get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Allocate buffer and read file */
    char* json_content = malloc(file_size + 1);
    if (!json_content) {
        fclose(file);
        return -1;
    }

    size_t read_size = fread(json_content, 1, file_size, file);
    json_content[read_size] = '\0';
    fclose(file);

    /* Parse interfaces section */
    char* interfaces_section = find_json_section(json_content, "interfaces");
    if (interfaces_section) {
        char* first_interface = find_first_array_object(interfaces_section);
        if (first_interface) {
            extract_json_string(first_interface, "name", config->interface.name, sizeof(config->interface.name));
            extract_json_string(first_interface, "ip", config->interface.ip, sizeof(config->interface.ip));
        }
    }

    /* Parse rx_sessions section */
    char* rx_sessions_section = find_json_section(json_content, "rx_sessions");
    if (rx_sessions_section) {
        char* first_session = find_first_array_object(rx_sessions_section);
        if (first_session) {
            /* Extract IP array */
            extract_ip_from_array(first_session, "ip", config->session.ip, sizeof(config->session.ip));

            /* Extract interface index (from array) */
            config->session.interface_index = extract_json_int(first_session, "interface");

            /* Parse st20p section if present */
            char* st20p_section = find_json_section(first_session, "st20p");
            if (st20p_section) {
                char* first_st20p = find_first_array_object(st20p_section);
                if (first_st20p) {
                    config->session.has_st20p = true;

                    config->session.st20p.replicas = extract_json_int(first_st20p, "replicas");
                    config->session.st20p.start_port = extract_json_int(first_st20p, "start_port");
                    config->session.st20p.payload_type = extract_json_int(first_st20p, "payload_type");
                    config->session.st20p.width = extract_json_int(first_st20p, "width");
                    config->session.st20p.height = extract_json_int(first_st20p, "height");

                    extract_json_string(first_st20p, "fps", config->session.st20p.fps, sizeof(config->session.st20p.fps));
                    extract_json_string(first_st20p, "device", config->session.st20p.device, sizeof(config->session.st20p.device));
                    extract_json_string(first_st20p, "output_format", config->session.st20p.output_format, sizeof(config->session.st20p.output_format));
                    extract_json_string(first_st20p, "transport_format", config->session.st20p.transport_format, sizeof(config->session.st20p.transport_format));

                    config->session.st20p.display = extract_json_bool(first_st20p, "display");
                    config->session.st20p.measure_latency = extract_json_bool(first_st20p, "measure_latency");
                }
            }

            /* Parse st30p section if present */
            char* st30p_section = find_json_section(first_session, "st30p");
            if (st30p_section) {
                char* first_st30p = find_first_array_object(st30p_section);
                if (first_st30p) {
                    config->session.has_st30p = true;

                    config->session.st30p.replicas = extract_json_int(first_st30p, "replicas");
                    config->session.st30p.start_port = extract_json_int(first_st30p, "start_port");
                    config->session.st30p.payload_type = extract_json_int(first_st30p, "payload_type");

                    extract_json_string(first_st30p, "audio_format", config->session.st30p.audio_format, sizeof(config->session.st30p.audio_format));
                    extract_json_string(first_st30p, "audio_sampling", config->session.st30p.audio_sampling, sizeof(config->session.st30p.audio_sampling));
                    extract_json_string(first_st30p, "audio_ptime", config->session.st30p.audio_ptime, sizeof(config->session.st30p.audio_ptime));
                }
            }
        }
    }

    free(json_content);
    return 0;
}

/* Convert fps string to MTL enum value */
int fps_string_to_value(const char* fps_str) {
    if (!fps_str) return 0;

    if (strcmp(fps_str, "p59") == 0 || strcmp(fps_str, "p60") == 0) {
        return 60;
    } else if (strcmp(fps_str, "p50") == 0) {
        return 50;
    } else if (strcmp(fps_str, "p30") == 0 || strcmp(fps_str, "p29") == 0) {
        return 30;
    } else if (strcmp(fps_str, "p25") == 0) {
        return 25;
    }

    return 25; /* Default */
}

/* Validate RX configuration */
int validate_rx_config(const struct rx_config* config) {
    if (!config) {
        return -1;
    }

    /* Check interface configuration */
    if (strlen(config->interface.name) == 0 || strlen(config->interface.ip) == 0) {
        printf("Error: Invalid interface configuration\n");
        return -1;
    }

    /* Check session configuration */
    if (strlen(config->session.ip) == 0) {
        printf("Error: Invalid session IP configuration\n");
        return -1;
    }

    /* Check ST20P configuration if present */
    if (config->session.has_st20p) {
        if (config->session.st20p.width <= 0 || config->session.st20p.height <= 0) {
            printf("Error: Invalid ST20P video dimensions\n");
            return -1;
        }

        if (config->session.st20p.start_port <= 0) {
            printf("Error: Invalid ST20P start port\n");
            return -1;
        }
    }

    printf("Configuration validation passed\n");
    return 0;
}

/* Load and validate configuration */
int load_and_validate_rx_config(const char* config_file, struct rx_config* config) {
    if (!config_file || !config || strlen(config_file) == 0) {
        return -1;
    }

    /* Parse and validate configuration */
    if (parse_rx_config(config_file, config) != 0 || validate_rx_config(config) != 0) {
        printf("Warning: Failed to load config file %s, using defaults\n", config_file);
        return -1;
    }

    printf("Loaded configuration from: %s\n", config_file);
    return 0;
}

/* Apply configuration to application parameters */
int apply_config_to_app(const struct rx_config* config,
                       uint32_t* width, uint32_t* height, int* fps,
                       char* dip_addr_str, size_t dip_size, uint8_t* dip_addr,
                       uint16_t* udp_port, bool* enable_display,
                       char* port, size_t port_size,
                       char* sip_addr_str, size_t sip_size, uint8_t* sip_addr) {

    if (!config) return -1;

    /* Override defaults with JSON config values for ST20P if present */
    if (config->session.has_st20p) {
        *width = config->session.st20p.width;
        *height = config->session.st20p.height;
        *fps = fps_string_to_value(config->session.st20p.fps);

        /* Update network configuration */
        inet_pton(AF_INET, config->session.ip, dip_addr);
        strncpy(dip_addr_str, config->session.ip, dip_size - 1);
        *udp_port = config->session.st20p.start_port;

        /* Apply display setting from config if not overridden by command line */
        if (config->session.st20p.display) {
            *enable_display = true;
            printf("Display enabled via config file\n");
        }

        printf("Using config values - Resolution: %dx%d, FPS: %s, Multicast: %s:%d\n",
               config->session.st20p.width, config->session.st20p.height, config->session.st20p.fps,
               config->session.ip, config->session.st20p.start_port);
    }

    /* Update interface configuration */
    if (strlen(config->interface.name) > 0) {
        strncpy(port, config->interface.name, port_size - 1);
        inet_pton(AF_INET, config->interface.ip, sip_addr);
        strncpy(sip_addr_str, config->interface.ip, sip_size - 1);
        printf("Using interface: %s (%s)\n", config->interface.name, config->interface.ip);
    }

    return 0;
}