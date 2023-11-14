#!/usr/bin/env bash

# Sets bridge s1 to use OpenFlow 1.3
ovs-vsctl set bridge s1 protocols=OpenFlow13 

# Sets bridge s2 to use OpenFlow 1.3
ovs-vsctl set bridge s2 protocols=OpenFlow13 

# Sets bridge s3 to use OpenFlow 1.3
ovs-vsctl set bridge s3 protocols=OpenFlow13 

# Sets bridge r1 to use OpenFlow 1.3
ovs-vsctl set bridge r1 protocols=OpenFlow13 

# Sets bridge r2 to use OpenFlow 1.3
ovs-vsctl set bridge r2 protocols=OpenFlow13 

# Print the protocols that each switch supports
for switch in s1 s2 s3 r1 r2;
do
    protos=$(ovs-vsctl get bridge $switch protocols)
    echo "Switch $switch supports $protos"
done

# Avoid having to write "-O OpenFlow13" before all of your ovs-ofctl commands.
ofctl='ovs-ofctl -O OpenFlow13'

# OVS rules for switch 1
$ofctl add-flow s1 \
    in_port=1,actions=mod_dl_src:AA:AA:AA:AA:00:00,mod_dl_dst:AA:AA:AA:00:00:00,output=2 

$ofctl add-flow s1 \
    in_port=2,actions=mod_dl_src:AA:AA:AA:AA:AA:00,mod_dl_dst:AA:AA:AA:AA:AA:AA,output=1 

# OVS rules for switch 2
$ofctl add-flow s2 \
    in_port=2,actions=mod_dl_src:DD:DD:DD:DD:00:00,mod_dl_dst:DD:DD:DD:00:00:00,output=4 

$ofctl add-flow s2 \
    in_port=4,actions=mod_dl_src:DD:DD:DD:DD:DD:00,mod_dl_dst:D0:D0:D0:D0:D0:D0,output=2 

$ofctl add-flow s2 \
    in_port=1,actions=mod_dl_src:BB:BB:BB:BB:00:00,mod_dl_dst:BB:BB:BB:00:00:00,output=3 

$ofctl add-flow s2 \
    in_port=3,actions=mod_dl_src:BB:BB:BB:BB:BB:00,mod_dl_dst:B0:B0:B0:B0:B0:B0,output=1

# OVS rules for switch 3
$ofctl add-flow s3 \
    in_port=2,actions=mod_dl_src:DD:DD:DD:00:00:00,mod_dl_dst:DD:DD:DD:DD:00:00,output=1 

$ofctl add-flow s3 \
    in_port=1,actions=mod_dl_src:CC:CC:CC:CC:00:00,mod_dl_dst:CC:CC:CC:CC:CC:00,output=2

# OVS rules for R1
$ofctl add-flow r1 \
    in_port=2,ip,nw_src=10.4.4.48,nw_dst=10.1.1.17,actions=mod_dl_src:AA:AA:AA:00:00:00,mod_dl_dst:AA:AA:AA:AA:00:00,output=1 

$ofctl add-flow r1 \
    in_port=1,ip,nw_src=10.1.1.17,nw_dst=10.4.4.48,actions=mod_dl_src:BB:BB:BB:00:00:00,mod_dl_dst:BB:BB:BB:BB:00:00,output=2 

# OVS rules for R2
$ofctl add-flow r2 \
    in_port=1,ip,nw_src=10.4.4.96,nw_dst=10.6.6.69,actions=mod_dl_src:CC:CC:CC:00:00:00,mod_dl_dst:CC:CC:CC:CC:00:00,output=2 

$ofctl add-flow r2 \
    in_port=2,ip,nw_src=10.6.6.69,nw_dst=10.4.4.96,actions=mod_dl_src:DD:DD:DD:00:00:00,mod_dl_dst:DD:DD:DD:DD:00:00,output=1 

# Print the flows installed in each switch
for switch in s1 s2 s3 r1 r2;
do
    echo "Flows installed in $switch:"
    $ofctl dump-flows $switch
    echo ""
done
