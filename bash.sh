module="netdev"

function load() {
   sudo insmod netdev.ko
}

function set_ip(){
   sudo ifconfig sn0 down
   sudo ifconfig sn1 down
   sudo ifconfig sn0 172.16.100.0/24 up
   sudo ifconfig sn1 172.16.101.1/24 up
}
function unload() {
   sudo ifconfig sn0 down
   sudo ifconfig sn1 down
   sudo rmmod $module
}
arg=$1
case $arg in
    load)
        load ;;
    set_ip)
        set_ip ;;
    unload)
        unload ;;
esac
