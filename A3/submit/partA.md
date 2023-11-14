$ofctl add-flow s0\
	in_port=1,ip,nw_src=10.0.0.2,nw_dst=10.0.2.2,actions=mod_dl_src:0A:00:0B:01:00:03,mod_dl_dst:0A:00:0B:FE:00:02,output=3

s0 means the flow rule is add to switch s0
in_port=1 means the packet is enter from port 1
ip means check the ip
nw_src=10.0.0.2 means check whether the srouce ip is 10.0.0.2
nw_dst=10.0.2.2 means check whether the destination ip is 10.0.2.2
actions= ... means if all conditions pass, then do the following thing(s)
mod_dl_src:0A:00:0B:01:00:03 means set the ethernet source based on MAC,
mmod_dl_dst:0A:00:0B:FE:00:02 means set the ethernet destination based on MAC
output=3 means the packet should be pushed into port 3 and send it out