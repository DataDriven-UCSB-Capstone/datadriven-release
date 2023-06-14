# Digital Ocean Setup
sudo ufw allow 65432/udp
iptables -A INPUT -i eth0 -p tcp -m tcp --dport 65432 -j ACCEPT
