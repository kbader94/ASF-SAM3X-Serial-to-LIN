#include <asf.h>
#include <stdio.h>
#include <string.h>

/* example LIN frame ID's */
#define LIN_FRAME_ID_11       0x32 //0x32
#define LIN_FRAME_ID_12       0x34 //0xb4
#define LIN_FRAME_ID_13		  0x36 //0x76

/* LIN node number */
#define LIN_SLAVE_NODE_NUM    1

/* Buffer size for UART serial communication */
#define BUFFER_SIZE  10

#define STRING_EOL    "\r"
#define STRING_HEADER "-- LIN-2-USB v0.1a --\r\n" \
"-- "BOARD_NAME" --\r\n" \
"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL


/** Local Buffer for emission */


/* LIN data cache */
uint8_t lin_data[64][8];// lin frame data cache
st_lin_message lin_descriptors[64]; // lin descriptor
volatile uint8_t data_avail = false;
volatile uint8_t num_lin_responses = 0;

/* Pdc transfer buffer */
uint8_t g_uc_pdc_buffer[BUFFER_SIZE];
/* PDC data packet for transfer */
pdc_packet_t g_pdc_uart_packet;
/* Pointer to UART PDC register base */
Pdc *g_p_uart_pdc;

void config_lin(void);
void config_pdc(void);

uint8_t process_lin_data(uint8_t raw_data[10]);// returns id
void register_lin_response(uint8_t id);