#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h> /*eth_type_trans*/
#include <linux/ip.h>          /*struct iphdr*/
#include <net/ip.h>
#include "main.h"


static void (*ldd_interrupt)(int, void *, struct pt_regs *);
static int timeout = NETDEV_TIMEOUT;
module_param(timeout, int, 0);

int pool_size = 8;
module_param(pool_size, int, 0);

static int lockup = 0;
module_param(lockup, int, 0);

static int use_napi = 0;
module_param(use_napi, int, 0);

static struct net_device *net_devs[2]; 
// nhan du lieu
void snull_priv_rx(struct net_device *dev, struct snull_packet *pkt) //module snull khai bao cau truc snull_priv
{
	struct sk_buff *skb;
	struct snull_priv *snull_priv = netdev_priv(dev); 

	skb = dev_alloc_skb(pkt->datalen + 2); // cap phat bo nho cho skb
	if (!skb) { // neu cap phat khong thanh cong
		if (printk_ratelimit()) // tranh viec spam msg tren dmesg
			printk(KERN_NOTICE "snull rx: low on mem - packet dropped\n");
		snull_priv->stats.rx_dropped++;
		goto out;
	}
	skb_reserve(skb, 2); /* align IP on 16B boundary */  
	memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen); // skb_put cap nhat con tro o cuoi bo dem, tra ve con tro o bo dem moi 

	/* Write metadata, and then pass to the receive level */
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; //khong kiem tra

	snull_priv->stats.rx_packets++;
	snull_priv->stats.rx_bytes += pkt->datalen;
	pr_debug("======  nhan du lieu ========\n");
	print_hex_dump(KERN_DEBUG, "skb receive: ", DUMP_PREFIX_OFFSET,
		       16, 1, skb->data, skb->len, true);
	netif_rx(skb); // giai phong socket o tren
  out:
	return;
}

// lay lai packet cho vao pool
void ldd_release_buffer(struct snull_packet *pkt)
{
	unsigned long flags; //cac interface flags
	struct snull_priv *snull_priv = netdev_priv(pkt->dev);
	
	spin_lock_irqsave(&snull_priv->lock, flags); // luu trang thai cac co ngat truoc do va vo hieu hoa ngat
	pkt->next = snull_priv->pkt_pool;
	snull_priv->pkt_pool = pkt;
	spin_unlock_irqrestore(&snull_priv->lock, flags); // mo khoa ngat va thuc hien cac trang thai ngat da luu

	if (netif_queue_stopped(pkt->dev) && pkt->next == NULL) // neu hang doi dang bi dung thi bat lai
		netif_wake_queue(pkt->dev);

	pr_debug("release pkt: dev = %s\n", pkt->dev->name);
}

// day pkt vao hang doi cua net_dev
static
void ldd_enqueue_buf(struct net_device *dev, struct snull_packet *pkt)
{
	unsigned long flags;
	struct snull_priv *snull_priv = netdev_priv(dev); // driver truy cap con tro du lieu rieng tu

	spin_lock_irqsave(&snull_priv->lock, flags);
	pkt->next = snull_priv->rx_queue;  /* FIXME - misorders packets */
	snull_priv->rx_queue = pkt;
	spin_unlock_irqrestore(&snull_priv->lock, flags);
}

// lay dau tien cua hang doi
static
struct snull_packet *ldd_dequeue_buf(struct net_device *dev)
{
	struct snull_priv *snull_priv = netdev_priv(dev);
	struct snull_packet *pkt;
	unsigned long flags;

	spin_lock_irqsave(&snull_priv->lock, flags);
	pkt = snull_priv->rx_queue;
	if (pkt != NULL)
		snull_priv->rx_queue = pkt->next;
	spin_unlock_irqrestore(&snull_priv->lock, flags);
	return pkt;
}

// kieu gui nhan data gian doan

static
void ldd_regular_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int statusword;
	unsigned long flags;
	struct snull_priv *snull_priv;
	struct snull_packet *pkt = NULL;
	struct net_device *dev = (struct net_device *)dev_id;

	if (!dev)
		return;

	snull_priv = netdev_priv(dev);
	spin_lock_irqsave(&snull_priv->lock, flags);

	statusword = snull_priv->status;
	snull_priv->status = 0;

	if (statusword & NETDEV_RX_INTR) {
		pkt = snull_priv->rx_queue;
		if (pkt) {
			snull_priv->rx_queue = pkt->next;
			pr_debug("====== nhan du lieu tu hang doi ========");
			snull_priv_rx(dev, pkt);
		}
	}

	if (statusword & NETDEV_TX_INTR) {
		snull_priv->stats.tx_packets++;
		snull_priv->stats.tx_bytes += snull_priv->tx_packetlen;
		dev_kfree_skb(snull_priv->skb);
	}

	spin_unlock_irqrestore(&snull_priv->lock, flags);

	if (pkt)
		ldd_release_buffer(pkt); /* Do this outside the lock! */
}

//kieu gui nhan data polled
static
int snull_priv_poll(struct napi_struct *napi, int budget)  // RX NAPI
{
	int rv, npackets = 0;
	unsigned long flags;
	struct sk_buff *skb;
	struct snull_priv *snull_priv = container_of(napi, struct snull_priv, napi);
	struct net_device *dev = snull_priv->net_dev;
	struct snull_packet *pkt;

	pr_debug("========= budget: %d, dev = %s\n", budget, dev->name);

	while (npackets < budget && snull_priv->rx_queue) { // nhận 1 lần nhiều pkt
		pr_debug("------ deque %s!\n", dev->name);
		pkt = ldd_dequeue_buf(dev);
		skb = dev_alloc_skb(pkt->datalen + 2);
		if (!skb) {
			if (printk_ratelimit())
				pr_err("snull: packet dropped\n");
			snull_priv->stats.rx_dropped++;
			npackets++;
			ldd_release_buffer(pkt);
			continue;
		}
		skb_reserve(skb, 2);
		memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);
		skb->dev = dev;
		skb->protocol = eth_type_trans(skb, dev);
		skb->ip_summed = CHECKSUM_UNNECESSARY;

		print_hex_dump(KERN_DEBUG, "skb poll raw: ", DUMP_PREFIX_OFFSET,
			       16, 1, skb->data, skb->len, true);

		rv = netif_receive_skb(skb); // thong bao la goi duoc nhan va duoc dong goi vao skb
		pr_debug("rv = %s\n", rv == NET_RX_SUCCESS ? "suc" : "drop");

		npackets++;
		snull_priv->stats.rx_packets++;
		snull_priv->stats.rx_bytes += pkt->datalen;
		ldd_release_buffer(pkt);
	}

	if (npackets < budget) {
		spin_lock_irqsave(&snull_priv->lock, flags);
		if (napi_complete_done(napi, npackets)) {
			pr_debug("openirq: %s, npackets = %d\n", dev->name, npackets);
			snull_priv->rx_int_enabled = true;
		}
		spin_unlock_irqrestore(&snull_priv->lock, flags);
	}
	return npackets;
}

//kieu dung new api
static
void ldd_napi_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int statusword;
	//unsigned long flags;
	struct snull_priv *snull_priv;

	/*
	 * As usual, check the "device" pointer for shared handlers.
	 * Then assign "struct device *dev"
	 */
	struct net_device *dev = (struct net_device *)dev_id;
	/* ... and check with hw if it's really ours */

	/* paranoid */
	if (!dev)
		return;

	/* Lock the device */
	snull_priv = netdev_priv(dev);
	// spin_lock_irqsave(&snull_priv->lock, flags);
	spin_lock(&snull_priv->lock);

	/* retrieve statusword: real netdevices use I/O instructions */
	statusword = snull_priv->status;
	snull_priv->status = 0;
	if (statusword & NETDEV_RX_INTR) {
		pr_debug("schedule: %s, %s, napi=%px\n", dev->name,
			 snull_priv->net_dev->name, &snull_priv->napi);
			snull_priv->rx_int_enabled = false; /* Disable further interrupts */ // không ngắt liên tục
			napi_schedule(&snull_priv->napi);
	}
	if (statusword & NETDEV_TX_INTR) { // nhận thì vẫn giống old API
        	/* a transmission is over: free the skb */
		snull_priv->stats.tx_packets++;
		snull_priv->stats.tx_bytes += snull_priv->tx_packetlen;
		if(snull_priv->skb) {
			dev_kfree_skb(snull_priv->skb);
			snull_priv->skb = 0;
		}
	}

	/* Unlock the device and we are done */
	// spin_unlock_irqrestore(&snull_priv->lock, flags);
	spin_unlock(&snull_priv->lock);
}


static
// cho phep netdev chap nhan cac paket de truyen 	
int ldd_netdev_open(struct net_device *dev)
{
	/* request_region(), request_irq(), ....  (like fops->open) */

	struct snull_priv *snull_priv = netdev_priv(dev);

	pr_debug("========= open %s ====== \n", dev->name);

	memcpy(dev->dev_addr, "\0SNUL0", ETH_ALEN);
	if (dev == net_devs[1])
		dev->dev_addr[ETH_ALEN-1]++; /* \0SNUL1 */

	pr_debug("======== enable napi = %px, napi = %px \n",
		 dev, &snull_priv->napi);
	if (use_napi)
		napi_enable(&snull_priv->napi);

	snull_priv->rx_int_enabled = true;
	netif_start_queue(dev);

	return 0;
}
// giai phong net_dev
static
int ldd_netdev_release(struct net_device *dev)
{
	struct snull_priv *snull_priv = netdev_priv(dev);

	pr_debug("========= release %s  ======= \n", dev->name);

	snull_priv->rx_int_enabled = false;
	netif_stop_queue(dev); // danh dau device khong the truyen them packet nao nua
	pr_debug("====== disable %px \n", &snull_priv->napi);
	if (use_napi)
		napi_disable(&snull_priv->napi);
	return 0;
}

static
int ldd_netdev_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)//cho phep netdev giao tiep vs userspace ma ko co syscall mac dinh
{
	pr_debug("ioctl\n");
	return 0;
}

// lay ra packet tu pool de dong goi
static
struct snull_packet *ldd_get_tx_buffer(struct net_device *dev) // lấy data để gửi ví dụ sn0 -> sn1
{
	unsigned long flags;
	struct snull_priv *snull_priv = netdev_priv(dev);
	struct snull_packet *pkt;
    
	spin_lock_irqsave(&snull_priv->lock, flags);
	pkt = snull_priv->pkt_pool;
	if(!pkt) {
		pr_debug("Out of Pool\n");
		return pkt;
	}
	snull_priv->pkt_pool = pkt->next;
	if (snull_priv->pkt_pool == NULL) {
		pr_info("Pool empty\n");
		netif_stop_queue(dev);
	}
	spin_unlock_irqrestore(&snull_priv->lock, flags);
	return pkt;
}

static
int ldd_netdev_hw_tx(struct sk_buff *skb, struct net_device *dev) //gửi dữ liệu từ socket buffer(kernel) ra ngoài card mạng (chỉ để mô phỏng)
	int rv, len;
	struct iphdr *iphdr;
	u32 *saddr, *daddr;
	struct snull_priv *dest_snull_priv, *src_snull_priv;
	struct net_device *dest_dev;
	struct snull_packet *tx_buffer;

	pr_debug("======= Truyen goi ==========\n");


	// print_hex_dump(KERN_DEBUG, "skb tx raw: ", DUMP_PREFIX_OFFSET,
		       // 16, 1, skb->data, skb->len, true);

	iphdr = (struct iphdr*)skb_network_header(skb);
	saddr = &iphdr->saddr;
	daddr = &iphdr->daddr;

	pr_debug("before saddr: 0x%x, daddr: 0x%x\n", *saddr, *daddr);
	((u8 *)saddr)[2] ^= 1; /* change the third octet (class C) */
	((u8 *)daddr)[2] ^= 1;
	pr_debug("after saddr: 0x%x, daddr: 0x%x\n", *saddr, *daddr);

	if (skb->len < ETH_ZLEN) {
		rv = skb_padto(skb, ETH_ZLEN);
		if (rv) {
			pr_err("Pad failed\n");
			return rv;
		}
		len = ETH_ZLEN;
	} else {
		len = skb->len;
	}

	ip_send_check(iphdr);

	/*
	 * Ok, now the packet is ready for transmission: first simulate a
	 * receive interrupt on the twin device, then  a
	 * transmission-done on the transmitting device
	 */
	dest_dev = net_devs[dev == net_devs[0] ? 1 : 0]; // dich khac nguon
	dest_snull_priv = netdev_priv(dest_dev);

	tx_buffer = ldd_get_tx_buffer(dev);

	if(!tx_buffer) {
		pr_debug("Out of tx buffer, len is %i\n",len);
		return -ENOMEM;
	}

	tx_buffer->datalen = len;
	memcpy(tx_buffer->data, skb->data, len);
	pr_debug("data from %s to %s\n", dev->name, dest_dev->name);

	ldd_enqueue_buf(dest_dev, tx_buffer);// vứt tx buffer vào hàng đợi của device đích

	if (dest_snull_priv->rx_int_enabled) {
		dest_snull_priv->status |= NETDEV_RX_INTR;
		ldd_interrupt(0, dest_dev, NULL); // mô phỏng ngắt nhận
	}


	src_snull_priv = netdev_priv(dev);
	src_snull_priv->tx_packetlen = len;
	src_snull_priv->tx_packetdata = skb->data;
	src_snull_priv->status |= NETDEV_TX_INTR;
#if 0
	if (lockup && ((src_snull_priv->stats.tx_packets + 1) % lockup) == 0) {
        	/* Simulate a dropped transmit interrupt */
		netif_stop_queue(dev);
		PDEBUG("Simulate lockup at %ld, txp %ld\n", jiffies,
				(unsigned long) src_snull_priv->stats.tx_packets);
	}
	else
		ldd_interrupt(0, dev, NULL);
#endif
	ldd_interrupt(0, dev, NULL);

	return 0;
}

static
int ldd_netdev_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct snull_priv *snull_priv = netdev_priv(dev);

	netif_trans_update(dev);

	/* Remember the skb, so we can free it at interrupt time */
	snull_priv->skb = skb;

	/* actual deliver of data is device-specific, and not shown here */
	return ldd_netdev_hw_tx(skb, dev);
}


//khi ifconfig thì gọi hàm này để trả về các thông số
static
struct net_device_stats *ldd_netdev_stats(struct net_device *dev)
{
	struct snull_priv *snull_priv = netdev_priv(dev);
	return &snull_priv->stats;
}

int ldd_netdev_header(struct sk_buff *skb, struct net_device *dev,
                unsigned short type, const void *daddr, const void *saddr,
                unsigned len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);

	eth->h_proto = htons(type);
	pr_debug("in-> saddr = %llx, daddr = %llx\n",
		 saddr ? *((u64*)saddr) : 0,
		 daddr ? *((u64*)daddr) : 0);
	memcpy(eth->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest,   daddr ? daddr : dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN - 1]   ^= 0x01;
	return (dev->hard_header_len);
}

static const struct header_ops ldd_header_ops = {
	.create  = ldd_netdev_header,
};

static const struct net_device_ops ldd_netdev_ops = {
	.ndo_open            = ldd_netdev_open,
	.ndo_stop            = ldd_netdev_release,
	.ndo_start_xmit      = ldd_netdev_tx,
	.ndo_do_ioctl        = ldd_netdev_ioctl,
	// .ndo_set_config      = snull_config,
	.ndo_get_stats       = ldd_netdev_stats,
	// .ndo_change_mtu      = snull_change_mtu,
	// .ndo_tx_timeout      = snull_tx_timeout,
};

static
int ldd_setup_pool(struct net_device *dev)
{
	int i;
	struct snull_priv *snull_priv = netdev_priv(dev);
	struct snull_packet *pkt, *next;

	pr_debug("pool_size = %d\n", pool_size);
	snull_priv->pkt_arr = kcalloc(pool_size, sizeof(struct snull_packet), GFP_KERNEL);
	if (!snull_priv->pkt_arr) {
		pr_err("Alloc memeory for pool failed\n");
		return -ENOMEM;
	}

	next = NULL;
	for (i = pool_size - 1; i >= 0; i--) {
		pkt = snull_priv->pkt_arr + i;
		pkt->next = next;
		pkt->dev = dev;
		next = pkt;
	}

	snull_priv->pkt_pool = snull_priv->pkt_arr;

	return 0;
}

static
void ldd_teardown_pool(struct net_device *dev)
{
	struct snull_priv *snull_priv = netdev_priv(dev);

	kfree(snull_priv->pkt_arr);
}    


static
void snull_priv_init(struct net_device *dev)
{
	struct snull_priv *snull_priv;

	ether_setup(dev);
	dev->watchdog_timeo = timeout;
	dev->netdev_ops = &ldd_netdev_ops;
	dev->header_ops = &ldd_header_ops;
	dev->flags           |= IFF_NOARP;
	dev->features        |= NETIF_F_HW_CSUM;

	//cap phat va khoi tao dev-> priv
	snull_priv = netdev_priv(dev);
	memset(snull_priv, 0, sizeof(struct snull_priv));

	snull_priv->net_dev = dev;
	spin_lock_init(&snull_priv->lock);
	snull_priv->rx_int_enabled = false;
	if (use_napi) {
		netif_napi_add(dev, &snull_priv->napi, snull_priv_poll, 2);
	}

}

static
void ldd_cleanup(void) //Huy dang ky cac interface, thuc hien don dep theo yeu cau va giai phong net_device tro lai he thong
{
	int i;
	for (i = 0; i < 2; i++) {
		if (net_devs[i]) {
			unregister_netdev(net_devs[i]); //xoa interface khoi he thong
			ldd_teardown_pool(net_devs[i]); //don dep
			free_netdev(net_devs[i]);//tra lai net_devie cho he thong
		}
	}
}

static
int __init m_init(void)
{
	int i, rv;
	struct snull_priv *snull_priv;

	ldd_interrupt = use_napi ? ldd_napi_interrupt : ldd_regular_interrupt;

	snull_priv = kzalloc(sizeof(struct snull_priv), GFP_KERNEL);
	if (!snull_priv)
		return -ENOMEM;
	// Cap phat dong cho net_device	
	net_devs[0] = alloc_netdev(sizeof(struct snull_priv), "sn%d",
				   NET_NAME_UNKNOWN, snull_priv_init);
	net_devs[1] = alloc_netdev(sizeof(struct snull_priv), "sn%d",
				   NET_NAME_UNKNOWN, snull_priv_init);
	if (net_devs[0] == NULL || net_devs[1] == NULL) {
		rv = -ENOMEM;
		goto out;
	}

	pr_debug("dev[0] = %px, dev[1] = %px\n", net_devs[0], net_devs[1]);
	snull_priv = netdev_priv(net_devs[0]);
	pr_debug("0 napi = %px\n", &snull_priv->napi);
	snull_priv = netdev_priv(net_devs[1]);
	pr_debug("1 napi = %px\n", &snull_priv->napi);
	// sau khi gọi register_netdev, driver có thể được gọi để vận hành trên thiết bị
	for (i = 0; i < 2;  i++) {
		rv = ldd_setup_pool(net_devs[i]);
		if (rv) {
			pr_err("Setup pool failed for device %d\n", i);
			goto out;
		}

		rv = register_netdev(net_devs[i]); // đăng ký driver
		if (rv) {
			pr_err("registering device %s failed\n",
			       net_devs[i]->name);
			rv = -ENODEV;
			goto out;
		}

	}

#if 0
	for (i = 0; i < 2;  i++) {
		snull_priv = netdev_priv(net_devs[i]);
		pr_debug("====== enable napi dev = %px, napi = %px\n",
			 net_devs[i], &snull_priv->napi);
		if (use_napi)
			napi_enable(&snull_priv->napi);
	}
#endif


	return 0;
out:
	ldd_cleanup();
	return rv;
}

static
void __exit m_exit(void)
{
	ldd_cleanup();
}

module_init(m_init);
module_exit(m_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("d0u9");
MODULE_DESCRIPTION("PCI Driver skel");
