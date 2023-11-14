#!/usr/bin/python

"""Topology with 10 switches and 10 hosts
"""

from mininet.cli import CLI
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.log import setLogLevel

class CSLRTopo( Topo ):

        def __init__( self ):
                "Create Topology"

                # Initialize topology
                Topo.__init__( self )

                # Add hosts
                alice = self.addHost( 'Alice' )
                bob = self.addHost( 'Bob' )
                david = self.addHost( 'David' )
                carol = self.addHost( 'Carol' )

                # Add switches
                s1 = self.addSwitch( 's1', listenPort=6635 )
                s2 = self.addSwitch( 's2', listenPort=6636 )
                s3 = self.addSwitch( 's3', listenPort=6637 )
                r1 = self.addSwitch( 'r1', listenPort=6638 )
                r2 = self.addSwitch( 'r2', listenPort=6639 )

                # Add links between hosts and switches
                self.addLink( alice, s1 )
                self.addLink( bob, s2 )
                self.addLink( david, s2 )
                self.addLink( carol, s3 )

                # Add links between switches, with bandwidth 100Mbps
                self.addLink( s1, r1, bw=100 )
                self.addLink( r1, s2, bw=100 )
                self.addLink( s2, r2, bw=100 )
                self.addLink( r2, s3, bw=100 )

def run():
        "Create and configure network"
        topo = CSLRTopo()
        net = Mininet( topo=topo, link=TCLink, controller=None )

        # Set interface IP and MAC addresses for hosts
        alice = net.get( 'Alice' )
        alice.intf( 'Alice-eth0' ).setIP( '10.1.1.17', 24 )
        alice.intf( 'Alice-eth0' ).setMAC( 'AA:AA:AA:AA:AA:AA' )

        bob = net.get( 'Bob' )
        bob.intf( 'Bob-eth0' ).setIP( '10.4.4.48', 24 )
        bob.intf( 'Bob-eth0' ).setMAC( 'B0:B0:B0:B0:B0:B0' )
        
        david = net.get( 'David' )
        david.intf( 'David-eth0' ).setIP( '10.4.4.96', 24 )
        david.intf( 'David-eth0' ).setMAC( 'D0:D0:D0:D0:D0:D0' )

        carol = net.get( 'Carol' )
        carol.intf( 'Carol-eth0' ).setIP( '10.6.6.69', 24 )
        carol.intf( 'Carol-eth0' ).setMAC( 'CC:CC:CC:CC:CC:CC' )

        # Set interface MAC address for switches (NOTE: IP
        # addresses are not assigned to switch interfaces)

        s1 = net.get( 's1' )
        s1.intf( 's1-eth1' ).setMAC( 'AA:AA:AA:AA:AA:00' )
        s1.intf( 's1-eth2' ).setMAC( 'AA:AA:AA:AA:00:00' )

        s2 = net.get( 's2' )
        s2.intf( 's2-eth2' ).setMAC( 'DD:DD:DD:DD:DD:00' )
        s2.intf( 's2-eth3' ).setMAC( 'BB:BB:BB:BB:00:00' )
        s2.intf( 's2-eth4' ).setMAC( 'DD:DD:DD:DD:00:00' )
        s2.intf( 's2-eth1' ).setMAC( 'BB:BB:BB:BB:BB:00' )

        s3 = net.get( 's3' )
        s3.intf( 's3-eth1' ).setMAC( 'CC:CC:CC:CC:CC:00' )
        s3.intf( 's3-eth2' ).setMAC( 'CC:CC:CC:CC:00:00' )

        r1 = net.get( 'r1' )
        r1.intf( 'r1-eth1' ).setMAC( 'AA:AA:AA:00:00:00' )
        r1.intf( 'r1-eth2' ).setMAC( 'BB:BB:BB:00:00:00' )

        r2 = net.get( 'r2' )
        r2.intf( 'r2-eth2' ).setMAC( 'CC:CC:CC:00:00:00' )
        r2.intf( 'r2-eth1' ).setMAC( 'DD:DD:DD:00:00:00' )

        net.start()

        # Add routing table entries for hosts (NOTE: The gateway
		# IPs 10.0.X.1 are not assigned to switch interfaces)
        alice.cmd( 'route add default gw 10.1.1.14 dev Alice-eth0' )
        bob.cmd( 'route add default gw 10.4.4.14 dev Bob-eth0' )
        david.cmd( 'route add default gw 10.4.4.28 dev David-eth0' )
        carol.cmd( 'route add default gw 10.6.6.46 dev Carol-eth0' )

        # Add arp cache entries for hosts
        alice.cmd( 'arp -s 10.1.1.14 AA:AA:AA:AA:AA:00 -i Alice-eth0' )
        bob.cmd( 'arp -s 10.4.4.14 BB:BB:BB:BB:BB:00 -i Bob-eth0' )
        david.cmd( 'arp -s 10.4.4.28 DD:DD:DD:DD:DD:00 -i David-eth0' )
        carol.cmd( 'arp -s 10.6.6.46 CC:CC:CC:CC:CC:00 -i Carol-eth0' )

        # Open Mininet Command Line Interface
        CLI(net)

        # Teardown and cleanup
        net.stop()

if __name__ == '__main__':
    setLogLevel('info')
    run()
