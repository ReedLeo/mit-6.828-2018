#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	envid_t from_envid;
	int r;
	while (1) {
		if ((r = ipc_recv(&from_envid, &nsipcbuf, 0)) < 0)
			panic("in output, ipc_recv %e", r);
		if (r != NSREQ_OUTPUT || from_envid != ns_envid)
			continue;
		while ((r = sys_net_try_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) < 0) {
			if (r == E_TX_RETRY) {
				sys_yield();
				continue;
			} else if (r == E_TX_PKTOF) {
				cprintf("in output, packet is too large in %d bytes.\n", nsipcbuf.pkt.jp_len);
			} else {
				panic("in output, unexpected returned value %d\n", r);
			}
			break;
		}
	}
}
