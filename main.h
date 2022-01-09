#ifndef _MAIN_H
#define _MAIN_H

#define MODULE_NAME			"netdev"

#define NETDEV_RX_INTR			0x0001
#define NETDEV_TX_INTR			0x0002
#define NETDEV_TIMEOUT			5

struct snull_packet {
	struct snull_packet *next;
	struct net_device *dev;
	int	datalen;
	u8 data[ETH_DATA_LEN];
};

struct snull_priv {
	struct net_device *net_dev;   //con tro net_dev
	struct net_device_stats stats; // thong so cua net_dev
	struct snull_packet *pkt_pool; // danh sach packet trong net_dev 
	struct snull_packet *pkt_arr;
	struct snull_packet *rx_queue; // danh sach packet trong hang doi nhan 
	int tx_packetlen;  
	u8 *tx_packetdata;
	int status;
	spinlock_t lock;
	struct sk_buff *skb; // danh sach socket buffer
	bool rx_int_enabled;
	struct napi_struct napi;
};



//sadfuhasdhasdoipasdiasjdipasdiosi
#endif
