/*

  This is the mac driver for a slip interface to work with a network driver.

  In this case, there is no RDC, radio, or framer. The data from the
  slip connection is passed into the NETSTACK.network
  (e.g. sicslowpan.c)

 */

/* don't want slip input to go through tcpip */
/* instead we want to send the data straight through the netstack */

#define SLIP_CONF_TCPIP_INPUT slip_mac_input

#include "contiki.h"
#include "net/netstack.h"
#include "dev/slip.h"
#include "dev/uart1.h"
#include "net/uip.h"
#include "net/rime.h"
#include <string.h>

#define SLIP_END     0300

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static void
slip_input_callback(void)
{

}

static void
init(void)
{
  slip_arch_init(BAUD2UBR(115200));
  process_start(&slip_process, NULL);
  slip_set_input_callback(slip_input_callback);
}

void
slip_mac_input(void)
{
	uint8_t i;
	uint8_t *ptr;
	printf("slip_mac_input\n\r");
	/* the slip-radio sent up  the full 802.15.4 pack so */
	/* that we could re-parse it here */
	packetbuf_set_datalen(uip_len);
	ptr = packetbuf_dataptr();
        memcpy(ptr, &uip_buf[UIP_LLH_LEN], UIP_BUFSIZE);
	
	for(i = 0; i < packetbuf_datalen(); i++)
	{
		printf("%02x ", ptr[i]);
	}

	NETSTACK_FRAMER.parse();
       	NETSTACK_NETWORK.input();
}

void
slip_mac_send(void)
{
	rimeaddr_t addr;
	/* the packet we want is now in the rime buffer */
	/* but slip send will use the uip buffer */
	/* copy it back and call slip_send */
	memcpy(&uip_buf[UIP_LLH_LEN], packetbuf_dataptr(), packetbuf_datalen());
	uip_len = packetbuf_datalen();
	
	/* append PACKETBUF_ADDR_RECEIVER */
	/* this gets stripped by slip radio which re-sets the packetbuf_attr for */
	/* framer802154 */

	rimeaddr_copy(&addr,packetbuf_addr(PACKETBUF_ADDR_RECEIVER));

#if DEBUG
	{
		int i;
		PRINTF("ADDR_RECEIVER ");		
		for (i = 0; i < RIMEADDR_SIZE; i++) {
			PRINTF("%02x:",addr.u8[i]);
		}
		PRINTF("\n\r");

	}
#endif

	memcpy(&uip_buf[UIP_LLH_LEN + uip_len],&addr, sizeof(rimeaddr_t));
	uip_len += sizeof(rimeaddr_t);

	slip_send();
}

static int
on(void)
{
  return NETSTACK_RDC.on();
}

static int
off(int keep_radio_on)
{
  return NETSTACK_RDC.off(keep_radio_on);
}

static unsigned short
channel_check_interval(void)
{
  return 0;
}

const struct mac_driver slipmac_driver = {
  "nullmac",
  init,
  slip_mac_send,
  slip_mac_input,
  on,
  off,
  channel_check_interval,
};

