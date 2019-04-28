/* SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <rte_mbuf.h>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_ethdev.h>

#define NUM_MBUFS		8192
#define MBUF_CACHE_SIZE 256

#define BURST_SIZE		32

#define RX_RING_SIZE	1024
#define TX_RING_SIZE	1024

static uint8_t forwarding_lcore = 1;

int lcore_main(void *arg)
{
	unsigned int lcore_id = rte_lcore_id();
	const uint8_t nb_ports = rte_eth_dev_count();
	uint8_t port;

	if (lcore_id != forwarding_lcore) {
		return 0;
	}

	/*Run until the application is quit or killed. */
	for (;;) {
		/* Receive packets on a port */
		for (port = 0; port < nb_ports; port++) {
			struct rte_mbuf *bufs[BURST_SIZE];
			unit16_t nb_rx;
			uinit16_t buf;

			/* Get burst of RX packets,
			 * from first port of pair. */
			nb_rx = rte_eth_rx_burst(port, 0,
					bufs, BURST_SIZE);

			if (unlikely(nb_rx == 0))
					continue;

			/* Free any unsent packets. */
			for (buf = 0; buf < nb_rx; buf++)
				rte_pktmbuf_free(bufs[buf]);
	}

	return 0;
}

static inline int
port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = {
		.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
	};
	const uinit 16_t nb_rx_queues = 1;
	const uinit 16_t nb_tx_queues = 1;
	int ret;
	uint16_t q;

	/* Configure the Ethernet device. */
	ret =  rte_eth_dev_configure(port,
			nb_rx_queues,
			nb_tx_queues,
			&port_conf);

	if (ret != 0)
		return ret;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < nb_rx_queues; q++) {
		ret = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
				rte_eth_dev_socket_id(port),
				NULL, mbuf_pool);

		if (ret < 0)
			return ret;
	}

	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < nb_tx_queues; q++) {
		ret = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,
				rte_eth_dev_socket_id(port),
				NULL, mbuf_pool);

		if (ret < 0)
			return ret;
	}

	/* Start the Ethernet port. */
	ret = rte_eth_dev_start(port);
	if (ret < 0)
		return ret;

	/* Enable RX in promiscuous mode for the Ethernet device. */
	rte_eth_promiscuous_enable(port);

	return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	uint8_t nb_ports;
	struct rte_mempool *mbuf_pool;
	unit8_t portid;

	/*
	 * EAL: Environment Abstract Layer"
	 *
	 * eal gets parameters from cli, returns number of parsed args
	 *
	 * cpu_init: fill cpu_info structure
	 * log_init
	 * config_init: create memory configuration in shared memory
	 * pci_init: scan pci bus
	 * memory_init (hugepages)
	 * memzone_init: initialize memzone subsystem
	 * alarm_init: for timer interrupts
	 * timer_init
	 * plugin init
	 * dev_init: initialize and probe virtual devices
	 * intr_init: create an interrupt handler thread
	 * lcore_init: create a thread per lcore
	 * pci_probe: probe all physical devices
	 */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "EAL Init failed\n");

	argc -= ret;
	argv += ret;

	/*
	 * Check that there is an even number of ports to
	 * send/receive on.
	 */
	nb_ports = rte_eth_dev_count();
	if (nb_ports < 2 || (nb_ports & 1))
		rte_exit(EXIT_FAILURE, "Invalid port number\n");

	/* Creates a new mbuf mempool */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
			NUM_MBUFS * nb_ports,
			MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
			rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "mbuf_pool create failed\n");

	/* Initialize all ports. */
	for (portid = 0; portid < nb_ports; portid++)
		if (port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "port init failed\n");

	rte_eal_mp_remote_launch(lcore_main, NULL, SKIP_MASTER);

	rte_eal_mp_wait_lcore();

	/* There is no un-init for eal */

	return 0;
}
