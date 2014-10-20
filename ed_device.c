/* The data flow of the devices is:
 *
 *          neted
 *     _______|___________
 *     |                  |
 *     |                  |
 *   ed_rec            ed_tx
 *   (recieve)          (transmit)
 *  
 * neted:  pseodu network device
 * ed_rec: character device
 * ed_tx:  character device
 */
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h> 
#include <linux/string.h>
#include <linux/errno.h>  
#include <linux/types.h>  
#include <linux/in.h>
#include <linux/netdevice.h>  
#include <linux/etherdevice.h> 
#include <linux/ip.h>          
#include <linux/skbuff.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include "ed_device.h"
#include "ed_ioctl.h"
//Added by Tuzhi
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>

#define _DEBUG

/* We must define the htons() function here, for the kernel has no
 * this API if you do not make source code dep
 */ 
//#define htons(x) ((x>>8) | (x<<8))

char ed_names[16];
struct ed_device ed[2];//实例化字符设备对象
struct net_device *ednet_dev; //实例化网络设备对象

static int timeout = ED_TIMEOUT;
void ednet_rx(struct net_device *dev,int len,unsigned char *buf);
void ednet_tx_timeout (struct net_device *dev);
      
/* Initialize the ed_rec and ed_tx device,the two devices
   are allocate the initial buffer to store the incoming and
   outgoing data. If the TCP/IP handshake need change the
   MTU,we must reallocte the buffer using the new MTU value.
 */
static int device_init(void){
    
    int i;
    int err;
    err = -ENOBUFS;
    strcpy(ed[ED_REC_DEVICE].name, ED_REC_DEVICE_NAME);
    strcpy(ed[ED_TX_DEVICE].name, ED_TX_DEVICE_NAME);
    for (i = 0 ;i < 2; i++ )
    {   
    	ed[i].buffer_size = BUFFER_SIZE;     	
        ed[i].buffer = kmalloc(ed[i].buffer_size + 4 , GFP_KERNEL);
        ed[i].magic = ED_MAGIC;
        ed[i].mtu = ED_MTU;
        ed[i].busy = 0;
        init_waitqueue_head(&ed[i].rwait);
        
	if (ed[i].buffer == NULL)
	    goto err_exit;	    	
	spin_lock_init(&ed[i].lock);//加锁  
    }  
    err = 0;
    return err;
    
err_exit:
    printk("There is no enongh memory for buffer allocation. \n");
    return err;		
}


/*Added by Tuzhi
 */
static struct ednet_priv *GetPrivate(struct net_device *dev)
{
	return netdev_priv(dev);
}

static int ed_realloc(int new_mtu){
    int err;
    int i;
    char *local_buffer[2];
    int size;
    err = -ENOBUFS;
    for (i=0;i<2;i++){
        local_buffer[i] = kmalloc(new_mtu + 4,GFP_KERNEL);
        size = min(new_mtu,ed[i].buffer_size);

        memcpy(local_buffer[i],ed[i].buffer,size);
        kfree(ed[i].buffer);

        ed[i].buffer = kmalloc(new_mtu + 4,GFP_KERNEL);
        if( ed[i].buffer < 0){
            printk("Can not realloc the buffer from kernel when change mtu.\n");
            return err;
        }
           
    }
    return 0;
	
}

/* Open the two character devices,and let the ed_device's private pointer 
 * point to the file struct */
//让edp私有指针指向file结构体
static int device_open(struct inode *inode,struct file *file)
{
    int Device_Major;
    struct ed_device *edp;
    Device_Major = imajor(inode); //设备Major号
    
    #ifdef _DEBUG
    printk("Get the Device Major Number is %d\n",Device_Major);
    #endif
    if (Device_Major == MAJOR_NUM_REC ) //如果是接受字符设备
    {
        file->private_data = &ed[ED_REC_DEVICE];
        ed[ED_REC_DEVICE].file = file;
    }    
    else    
    if (Device_Major == MAJOR_NUM_TX){ //如果是发送字符设备
        file->private_data = &ed[ED_TX_DEVICE];
        ed[ED_TX_DEVICE].file = file;
    }    
    else
        return -1;
    edp = (struct ed_device *)file->private_data;
                    
    if(edp->busy != 0){
       printk("The device is open!\n");	
       return -EBUSY;
    }
    
    edp->busy++; //拥有信号量
 
    return 0;
       
}

/* release the devices */
int device_release(struct inode *inode,struct file *file)
{
	
    struct ed_device *edp;
    edp = (struct ed_device *)file->private_data;
    edp->busy = 0; //释放信号量
    
    return 0;	
}

/* read data from ed_tx device */
ssize_t device_read(struct file *file,char *buffer,size_t length, loff_t *offset)
{
    #ifdef _DEBUG
    int i;
    #endif
    struct ed_device *edp;
    DECLARE_WAITQUEUE(wait,current);
    edp = (struct ed_device *)file->private_data;
    add_wait_queue(&edp->rwait,&wait);//休眠，由kernel_write唤醒
    for(;;){//内核中轮询，是否有数据可以读取，没有就阻塞用户进程。
        
        set_current_state(TASK_INTERRUPTIBLE);
        if ( file->f_flags & O_NONBLOCK)
            break;
        if ( edp->tx_len > 0)
            break;
        
        if ( signal_pending(current))
            break;
        schedule();
        
    }
    set_current_state(TASK_RUNNING);
    remove_wait_queue(&edp->rwait,&wait);//有数据时唤醒用户进程
    
    spin_lock(&edp->lock);
    
    if(edp->tx_len == 0) {
         spin_unlock(&edp->lock);
         return 0;

        
    }else
    {
   
        copy_to_user(buffer,edp->buffer,edp->tx_len);//buffer复制到用户空间
        memset(edp->buffer,0,edp->buffer_size); //置空内核buffer
        
        #ifdef _DEBUG //printk打印出信息
        printk("\n read data from ed_tx \n");
        for(i=0;i<edp->tx_len;i++)
            printk(" %02x",edp->buffer[i]&0xff);
        printk("\n");
        #endif
        
        length = edp->tx_len;
        edp->tx_len = 0;
    }
    spin_unlock(&edp->lock);
    return length;
    
}

/* This function is called by ednet device to write the network data 
 * into the ed_tx character device.
 */
ssize_t kernel_write(const char *buffer,size_t length,int buffer_size)
{
          
    if(length > buffer_size )
        length = buffer_size;
    memset(ed[ED_TX_DEVICE].buffer,0,buffer_size);
    memcpy(ed[ED_TX_DEVICE].buffer,buffer,buffer_size);
    ed[ED_TX_DEVICE].tx_len = length;
    wake_up_interruptible(&ed[ED_TX_DEVICE].rwait);	
    
    return length;
}

/* Device write is called by server program, to put the user space
 * network data into ed_rec device.
 * 从串口收到的数据写入ed_rec，加上TCP/IP包，被上层API调用。
 */
ssize_t device_write(struct file *file,const char *buffer, size_t length,loff_t *offset)
{
    #ifdef _DEBUG
    int i;
    #endif
    struct ed_device *edp;
    edp = (struct ed_device *)file->private_data;//edp为void*强制类型转换，指向file的私有数据
    
    spin_lock(&ed[ED_REC_DEVICE].lock);
    if(length > edp->buffer_size)
        length =  edp->buffer_size;
        
    copy_from_user( ed[ED_REC_DEVICE].buffer,buffer, length);
    ednet_rx(ednet_dev,length,ed[ED_REC_DEVICE].buffer);
    //ed[ED_REC_DEVICE]中的数据发送到ednet_dev的sk_buff中
    //ednet_dev是一个device类型的全局变量
    
    #ifdef _DEBUG
    printk("\nNetwork Device Recieve buffer:\n");
    for(i =0;i< length;i++)
       printk(" %02x",ed[ED_REC_DEVICE].buffer[i]&0xff);
    printk("\n");
    #endif
    spin_unlock(&ed[ED_REC_DEVICE].lock);     
    return length;   
}        

//用来在APP层改变内核ed_device结构的busy位
int device_ioctl(struct inode *inode,
                 struct file *file,
                 unsigned int ioctl_num,
                 unsigned long ioctl_param){
    struct ed_device *edp;
    edp = (struct ed_device *)file->private_data;
    //file的private_data指针即指向了edp,把ed_device的数据放入到private_data里。
    switch(ioctl_num)
    {
        case IOCTL_SET_BUSY:
           edp->busy = ioctl_param;
           break;      
    }
    return 0;
}

/* 
 * All the ednet_* functions are for the ednet pseudo network device ednet.
 * ednet_open and ednet_release are the two functions which open and release
 * the device.
 */
 
int ednet_open(struct net_device *dev)
{
    //MOD_INC_USE_COUNT;//增加虚拟网卡设备的模块计数
    
    /* Assign the hardware pseudo network hardware address,
     * the MAC address's first octet is 00,for the MAC is
     * used for local net,not for the Internet.
     */
    memcpy(dev->dev_addr, "\0ED000", ETH_ALEN);//硬件地址
	//Revised according to snull.c
	netif_start_queue(dev);//开启网络接口接收和发送数据队列
    return 0;
}
int ednet_release(struct net_device *dev)
{
	netif_stop_queue(dev); //停止网络接口接收和发送数据队列
	//dev->start = 0;
    //dev->tbusy = 1;             
    //MOD_DEC_USE_COUNT;//减少虚拟网卡设备的模块计数
	return 0;
}

/*
 * Used by ifconfig,the io base addr and IRQ can be modified
 * when the net device is not running.
 */
int ednet_config(struct net_device *dev, struct ifmap *map)
{
    if (dev->flags & IFF_UP) 
        return -EBUSY;

    /* change the io_base addr */
    if (map->base_addr != dev->base_addr) {
        printk(KERN_WARNING "ednet: Can't change I/O address\n");
        return -EOPNOTSUPP;
    }

    /* can change the irq */
    if (map->irq != dev->irq) {
        dev->irq = map->irq;
        
    }

    
    return 0;
}


/*
 * ednet_rx,recieves a network packet and put the packet into TCP/IP up
 * layer,netif_rx() is the kernel API to do such thing. The recieving
 * procedure must alloc the sk_buff structure to store the data,
 * and the sk_buff will be freed in the up layer.
 */
void ednet_rx(struct net_device *dev, int len, unsigned char *buf)
{
    struct sk_buff *skb;
    //struct ednet_priv *priv = (struct ednet_priv *) dev->priv;
    //priv 是net_device的void*指针。dev设备的私有priv指针指向ednet_priv。
    struct ednet_priv *priv=GetPrivate(dev);
	skb = dev_alloc_skb(len+2);
    if (!skb) {
        printk("ednet_rx can not allocate more memory to store the packet. drop the packet\n");
        priv->stats.rx_dropped++;
        return;
    }
    skb_reserve(skb, 2);
    memcpy(skb_put(skb, len), buf, len);

    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    /* We need not check the checksum */
    skb->ip_summed = CHECKSUM_UNNECESSARY; 
    priv->stats.rx_packets++;
    netif_rx(skb);//传到TCP/IP层
    return;
}
    
        

/*
 * pseudo network hareware transmit,it just put the data into the 
 * ed_tx device.
 */
void ednet_hw_tx(char *buf, int len, struct net_device *dev)
{
   
    struct ednet_priv *priv;
   
    /* check the ip packet length,it must more then 34 octets */
    if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
        printk("Bad packet! It's size is less then 34!\n");
        
        return;
    }
    /* now push the data into ed_tx device */  
    ed[ED_TX_DEVICE].kernel_write(buf,len,ed[ED_TX_DEVICE].buffer_size); 
    
    /* record the transmitted packet status */
    //priv = (struct ednet_priv *) dev->priv;
    priv=GetPrivate(dev);
    priv->stats.tx_packets++;
    priv->stats.rx_bytes += len;
    
    /* remember to free the sk_buffer allocated in upper layer. */
    dev_kfree_skb(priv->skb);
 
   
}

/*
 * Transmit the packet,called by the kernel when there is an
 * application wants to transmit a packet.
 */
int ednet_tx(struct sk_buff *skb, struct net_device *dev)
{
    int len;
    char *data;
    //struct ednet_priv *priv = (struct ednet_priv *) dev->priv;
    struct ednet_priv *priv=GetPrivate(dev);
    if( ed[ED_TX_DEVICE].busy ==1){
     
        return -EBUSY;
    }
   // if (dev->tbusy || skb == NULL) {
	//这里的tbusy对应到2.6内核中应该怎么表示？
    if (skb == NULL) {
  
        ednet_tx_timeout (dev);
        if (skb == NULL)
            return 0;
    }

    len = skb->len < ETH_ZLEN ? ETH_ZLEN : skb->len;
    //对于以太网，如果有效数据的长度小于以太网冲突检测所要求数据帧的最小长度ETH_ZLEN，填0
    data = skb->data;
    /* stamp the time stamp */
    dev->trans_start = jiffies;

    /* remember the skb and free it in ednet_hw_tx */
    priv->skb = skb;
    
    /* pseudo transmit the packet,hehe */
    ednet_hw_tx(data, len, dev);//调用字符设备的操作例程kernel_write()

    return 0; 
}

/*
 * Deal with a transmit timeout.
 */
void ednet_tx_timeout (struct net_device *dev)
{
    //struct ednet_priv *priv = (struct ednet_priv *) dev->priv;
    struct ednet_priv *priv=GetPrivate(dev);
    priv->stats.tx_errors++;
    netif_wake_queue(dev);
    return;
}

/*
 * When we need some ioctls.
 */
int ednet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
 
    return 0;
}

/*
 * ifconfig to get the packet transmitting status.
 */
struct net_device_stats *ednet_stats(struct net_device *dev)
{
   // struct ednet_priv *priv = (struct ednet_priv *) dev->priv;
    struct ednet_priv *priv=GetPrivate(dev);
    return &priv->stats;
}

/*
 * TCP/IP handshake will call this function, if it need.
 */
int ednet_change_mtu(struct net_device *dev, int new_mtu)
{
    int err;
    unsigned long flags;
    //spinlock_t *lock = &((struct ednet_priv *) dev->priv)->lock;
	spinlock_t *lock = &(GetPrivate(dev))->lock;
    
    /* en, the mtu CANNOT LESS THEN 68 OR MORE THEN 1500. */
    if (new_mtu < 68)
        return -EINVAL;
   
    
    spin_lock_irqsave(lock, flags);
    dev->mtu = new_mtu;
    /* realloc the new buffer */
    
    err = ed_realloc(new_mtu); //重新分配缓存区
    spin_unlock_irqrestore(lock, flags);
    return err; 
}

int ednet_header(struct sk_buff *skb,
                 struct net_device *dev,
                 unsigned short type,
                 const void *daddr,
                 const void *saddr,
                 unsigned int len)
{
    struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);
    eth->h_proto = htons(type);
    memcpy(eth->h_source,saddr? saddr : dev->dev_addr,dev->addr_len);
    memcpy(eth->h_dest,   daddr? daddr : dev->dev_addr, dev->addr_len);
    return (dev->hard_header_len);

}

int ednet_rebuild_header(struct sk_buff *skb)
{
    struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);
    struct net_device *dev = skb->dev;

    memcpy(eth->h_source, dev->dev_addr ,dev->addr_len);
    memcpy(eth->h_dest,   dev->dev_addr , dev->addr_len);
    return 0;

}

//Tuzhi:Struct dev donot have open stop mem, they are in netdev_ops
static const struct net_device_ops pse_netdev_ops=
{
    .ndo_open            = ednet_open,
    .ndo_stop            = ednet_release,
    .ndo_set_config      = ednet_config,//Used
    .ndo_start_xmit      = ednet_tx, //数据包写入ed_tx设备
    .ndo_do_ioctl        = ednet_ioctl,//Used
    .ndo_get_stats       = ednet_stats,
    .ndo_change_mtu      = ednet_change_mtu,  
    //.ndo_hard_header     = ednet_header, 定义在header_ops中
    //.ndo_rebuild_header  = ednet_rebuild_header,
    .ndo_tx_timeout     = ednet_tx_timeout
};

static const struct header_ops pse_header_ops=
{
	.create = ednet_header,
	.rebuild = ednet_rebuild_header,
	.cache = NULL
};

struct file_operations ed_ops ={
    .owner=THIS_MODULE,
	.open=device_open,
    .read=device_read,
    .write=device_write,
    .ioctl=device_ioctl,
    .release=device_release
};

/* initialize the character devices */
int eddev_module_init(void) //注册两个字符设备驱动，一个接受，一个发送。
{
    int err;
    int i;
    if((err=device_init()) != 0)
    {
        printk("Init device error:");
        return err;
    }
    err = register_chrdev(MAJOR_NUM_REC,ed[ED_REC_DEVICE].name,&ed_ops);
    if( err != 0)
        printk("Install the buffer rec device %s fail", ED_REC_DEVICE_NAME);
    for(i=0; i<2;i++)
        ed[i].kernel_write = kernel_write;  
    
    err = register_chrdev(MAJOR_NUM_TX,ed[ED_TX_DEVICE].name,&ed_ops);
    if( err != 0)
        printk("Install the buffer tx device %s fail",ED_TX_DEVICE_NAME);		
    return err;    
    

}
/* clean up the character devices */
void eddev_module_cleanup(void)
{
    //int err;
    int i;
    for (i = 0 ;i < 2; i++){
        kfree(ed[i].buffer);   
    }    

    //err = unregister_chrdev(MAJOR_NUM_REC,ed[ED_REC_DEVICE].name);
    unregister_chrdev(MAJOR_NUM_REC,ed[ED_REC_DEVICE].name);
    //if(err != 0)
    //    printk("UnInstall the buffer recieve device %s fail",ED_REC_DEVICE_NAME);
    unregister_chrdev(MAJOR_NUM_TX,ed[ED_TX_DEVICE].name);
    //if(err != 0)
    //    printk("UnInstall the buffer recieve device %s fail",ED_TX_DEVICE_NAME);
        	
}

void ednet_init(struct net_device *dev)
{
	struct ednet_priv *priv=NULL;
    ether_setup(dev); 
	
	dev->netdev_ops = &pse_netdev_ops;
	dev->header_ops=&pse_header_ops;
	dev->watchdog_timeo = timeout;
    dev->flags          |= IFF_NOARP;
	dev->features       |=NETIF_F_NO_CSUM;

	priv=GetPrivate(dev);
    memset(priv, 0, sizeof(struct ednet_priv));
    spin_lock_init(& ((struct ednet_priv *)priv)->lock);
	return;
}

int ednet_module_init(void) //注册网络设备驱动
{
	int err;
	ednet_dev=alloc_netdev(sizeof(struct ednet_priv),"ed0",ednet_init);
	if(ednet_dev==NULL)
		return -1;
	
	if ( (err = register_netdev(ednet_dev)) )
            printk("ednet: error %i registering pseudo network device \"%s\"\n",err,ednet_dev->name);
	return 0;
}

static __exit void ednet_module_cleanup(void)
{
    //struct ednet_priv *priv=GetPrivate(&ednet_dev);
    //kfree(priv);
    unregister_netdev(ednet_dev);
	free_netdev(ednet_dev);
    return;
}

/* called by the kernel to setup the module kernel2.4*/
int InitModule(void)
{
    int err;
	printk("ed_device is iniitalited\n");
    err = eddev_module_init(); //初始化设备字符驱动
    if(err < 0)
        return err;
    err = ednet_module_init(); //初始化伪驱动设备
    if(err < 0)
        return err;
    return err;	
	
}
void CleanModule(void)
{
    eddev_module_cleanup();
    ednet_module_cleanup();	
	printk("ed_device is removed\n");
}

//For kernel 2.6
module_init(InitModule);
module_exit(CleanModule);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tuzhi");
MODULE_VERSION("NetRouter_V1.0");
