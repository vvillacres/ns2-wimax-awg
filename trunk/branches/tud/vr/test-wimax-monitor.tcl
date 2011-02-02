# Test for 802.16 scheduler.
# @author Xingting
# @date 04/01/2009
# Test file for wimax
# Scenario: Communication between MN and Sink Node with MN attached to BS.
#          In UL scenarios: 
#              Using [grep ^r out_mod_X.res | grep -c "1 0"] to get the number of packet received. 
#              Using [grep ^+ out.res | grep -c AGT] to get the number of packet sent.
#          In DL scenarios:
#              Using [grep ^r out_mod_X.res | grep -c AGT] to get the number of packet received. 
#              Using [grep ^+ out.res | grep -c "0 1"] to get the number of packet sent.
#           
#
# Topology scenario:
#
#
#	        |-----|          
#	        | MN0 |                 ; 1.0.1 
#	        |-----|        
#
#
#		  (^)
#		   |
#	    |--------------|
#           | Base Station | 		; 1.0.0
#           |--------------|
#	    	   |
#	    	   |
#	     |-----------|
#            | Sink node | 		; 0.0.0test-be-ul-arq.tcl
#            |-----------|
#

#check input parameters
if {$argc != 4} {
    puts ""
    puts "Wrong Number of Arguments! No arguments in this topology"
    puts "Syntax: ns test-be.tcl seed diuc dl/ul distance"
    puts ""
    exit (1)
}

# set global variables
#set nb_mn [lindex $argv 0]				;# max number of mobile node
set nb_mn 2


set seed [lindex $argv 0]		;# seed
set diuc [lindex $argv 1]
set direction [lindex $argv 2]
set distance [lindex $argv 3]


global defaultRNG
$defaultRNG seed $seed

set packet_size 1500			;# packet size in bytes at CBR applications 
set output_dir .
set gap_size 0.05 				;#compute gap size between packets
puts "gap size=$gap_size"
set traffic_start 10
set traffic_stop  20
set simulation_stop 21

#define debug values
Mac/802_16 set debug_           1
Mac/802_16 set rtg_             20
Mac/802_16 set ttg_             20
Mac/802_16 set frame_duration_  0.005
Mac/802_16 set ITU_PDP_         2
Mac/802_16/BS set dlratio_      .66
Mac/802_16/SS set dlratio_      .66
Mac/802_16 set fbandwidth_      10e+6
Mac/802_16 set disable_interference_ 0


#for ARQ parameters
Mac/802_16 set arq_block_size_ 256
Mac/802_16 set data_loss_rate_ 0.5
Mac/802_16 set arqfb_in_dl_data_  1
Mac/802_16 set arqfb_in_ul_data_  1
Mac/802_16 set queue_length_      100000     ;# maximum number of packets

Phy/WirelessPhy/OFDMA set g_ 0.125

#define coverage area for base station
#Phy/WirelessPhy set Pt_ 20 
Phy/WirelessPhy set Pt_ 0.2 
Phy/WirelessPhy set RXThresh_ 1.90546e-16
Phy/WirelessPhy set CSThresh_ [expr 0.9*[Phy/WirelessPhy set RXThresh_]]
Phy/WirelessPhy set OFDMA_ 1 

# Parameter for wireless nodes
set opt(chan)           Channel/WirelessChannel    ;# channel type
set opt(prop)           Propagation/OFDMA          ;# radio-propagation model
set opt(netif)          Phy/WirelessPhy/OFDMA      ;# network interface type
set opt(mac)            Mac/802_16/BS              ;# MAC type
set opt(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
set opt(ll)             LL                         ;# link layer type
set opt(ant)            Antenna/OmniAntenna        ;# antenna model
set opt(ifqlen)         50              	   ;# max packet in ifq
set opt(adhocRouting)   NOAH                       ;# routing protocol

set opt(x)		11000			   ;# X dimension of the topography
set opt(y)		11000			   ;# Y dimension of the topography


# BE Service Flow
set opt(be_sf_direction)	"DL"			;# DL - Downlink, UL - Uplink
set opt(be_sf_type)		"BE"			;# DL Data deivery service type UGS/ERT-VR/RT-VR/NRT-VR/BE
							;# UL grant scheduling type UGS/ertPS/rtPS/nrtPS/BE
set opt(be_sf_traffic_priority) 0			;# Traffic Priority
set opt(be_sf_mstr)		2.0e+6			;# Maximum Sustained Traffic Rate in bit/s
set opt(be_sf_max_burst_size)	0			;# Maximum Burst Size in byte
set opt(be_sf_mrtr)		0			;# Minimum Reserved Traffic Rate in bit/s
set opt(be_sf_regtrans_policy)  0			;# Request Transmission Policy
set opt(be_sf_tolerated_jitter) 1000			;# Tolerated Jitter in ms
set opt(be_sf_max_latency)	100			;# Maximum Latency in ms
set opt(be_sf_sdu_indicator)	0			;# SDU fixed size indicator 0:false 1:true
set opt(be_sf_sdu_size)		49			;# Size of fixed size SDUs

set opt(be_sf_arq_enable)	0			;# ARQ enable 0:false 1:true
set opt(be_sf_arq_window)	8			;# ARQ window size
set opt(be_sf_arq_timer)	100			;# ARQ retransmission timer
set opt(be_sf_arq_proctime)	0			;# ARQ Processing time
set opt(be_sf_arq_blocksize)	100			;# ARQ Block Size

set opt(be_sf_grant_interval)	20			;# Unsolicited Grant Interval in ms
set opt(be_sf_polling_interval) 20			;# Unsolicited Polling Interval in ms

set opt(be_sf_time_base)	50			;# Time Base

set opt(be_sf_error_rate)	0.0			;# Packet Error Rate


set opt(output_statfile) statistic.tr			;# statistic output filename

# Trace Parameters
set opt(trace_sampling_time)	0.1			;# samplingtime (s) for tracefile
# set opt(trace_header)		20			;# ip packet header
set opt(trace_start) 5
set opt(trace_stop) 21

set opt(statistic_file) [open $opt(output_statfile) w]
set opt(oldnow) 0.0



#defines function for flushing and closing files
proc finish {} {
    global ns tf output_dir nb_mn opt
    $ns flush-trace
    close $tf
    close $opt(output_statfile)
    exit 0
}


###################
# create statistic procedure
###################
proc statistic {} {
    global array names  opt ns nb_mn szen

    set now [$ns now]


    if { ( ($now >= $opt(trace_start)) && ($now <= $opt(trace_stop)) )} {
      
      # write data in statistic file
      set statdata "$now"
      set recv_bytes_sum 0

      for {set i 0} {$i < $nb_mn } {incr i} {
	# read data
	set distance [$szen(sink_$i) set distance_]
	set recv_bytes [$szen(sink_$i) set bytes_]
	set total_bytes [$szen(sink_$i) set total_bytes_]
	set recv_packets [$szen(sink_$i) set npkts_]
	set lost_packets [$szen(sink_$i) set nlost_]
	set total_packets [$szen(sink_$i) set total_npkts_]
	set delay [$szen(sink_$i) set delay_mean_]
	set jitter [$szen(sink_$i) set jitter_mean_]
	# calculate sum
	set recv_bytes_sum [expr $recv_bytes_sum + $recv_bytes]
	# write data
	set statdata [append statdata "  $distance $recv_bytes $recv_packets $lost_packets [expr (($recv_bytes * 8.0) / ( ($now - $opt(oldnow)) ))] $total_bytes $total_packets"]
	# reset monitor
	$szen(sink_$i) set bytes_ 0
	$szen(sink_$i) set npkts_ 0
	$szen(sink_$i) set nlost_ 0
	$szen(sink_$i) set delay_mean_ 0
	$szen(sink_$i) set jitter_mean_ 0
      }
      set statdata [append statdata "  [expr (($recv_bytes_sum * 8.0 ) / ( $opt(trace_sampling_time) ))]"]

      # write to file
      puts $opt(statistic_file) "$statdata"
  
  } else {
      # reset monitor
      for {set i 0} {$i < $nb_mn } {incr i} {
	  $szen(sink_$i) reset
      }
    }

    set opt(oldnow) $now;

    # restart statistic after $opt(trace_sampling_time)
    $ns at [expr ($now + $opt(trace_sampling_time)) ] "statistic"

}  









#create the simulator
set ns [new Simulator]
$ns use-newtrace

#create the topography
set topo [new Topography]
$topo load_flatgrid $opt(x) $opt(y)
#puts "Topology created"

#open file for trace
#set tf [open $output_dir/out_be.res w]
set tf [open $output_dir/out_mod_$diuc.res w]

$ns trace-all $tf
#puts "Output file configured"

# set up for hierarchical routing (needed for routing over a basestation)
#puts "start hierarchical addressing"
$ns node-config -addressType hierarchical
AddrParams set domain_num_ 2          			;# domain number
lappend cluster_num 1 1            			;# cluster number for each domain 
AddrParams set cluster_num_ $cluster_num
lappend eilastlevel 1 [expr ($nb_mn+1)] 		;# number of nodes for each cluster (1 for sink and one for mobile nodes + base station
AddrParams set nodes_num_ $eilastlevel
puts "Configuration of hierarchical addressing done"

# Create God
create-god [expr ($nb_mn + 2)]				;# nb_mn + 2 (base station and sink node)
#puts "God node created"

#creates the sink node in first addressing space.
set sinkNode [$ns node 0.0.0]
#provide some co-ord (fixed) to base station node
$sinkNode set X_ 50.0
$sinkNode set Y_ 50.0
$sinkNode set Z_ 0.0
#puts "sink node created"

#creates the Access Point (Base station)
$ns node-config -adhocRouting $opt(adhocRouting) \
    -llType $opt(ll) \
    -macType Mac/802_16/BS \
    -ifqType $opt(ifq) \
    -ifqLen $opt(ifqlen) \
    -antType $opt(ant) \
    -propType $opt(prop)    \
    -phyType $opt(netif) \
    -channel [new $opt(chan)] \
    -topoInstance $topo \
    -wiredRouting ON \
    -agentTrace ON \
    -routerTrace ON \
    -macTrace ON  \
    -movementTrace OFF
#puts "Configuration of base station"

#setup channel model
set prop_inst [$ns set propInstance_]
$prop_inst ITU_PDP PED_A
puts "after set pPDP"


set bstation [$ns node 1.0.0]  
$bstation random-motion 0
#puts "Base-Station node created"
#provide some co-ord (fixed) to base station node
$bstation set X_ 60.0
$bstation set Y_ 50.0
$bstation set Z_ 1.5
[$bstation set mac_(0)] set-channel 0

# creation of the mobile nodes
$ns node-config -macType Mac/802_16/SS \
    -wiredRouting OFF \
    -macTrace ON  				;# Mobile nodes cannot do routing.
for {set i 0} {$i < $nb_mn} {incr i} {
    set wl_node_($i) [$ns node 1.0.[expr $i + 1]] 	;# create the node with given @.	
    $wl_node_($i) random-motion 0			;# disable random motion
    $wl_node_($i) base-station [AddrParams addr2id [$bstation node-addr]] ;#attach mn to basestation
    #compute position of the node
    $wl_node_($i) set X_ [expr 60.0+$distance]
    $wl_node_($i) set Y_ 50.0
    $wl_node_($i) set Z_ 0.0
    puts "wireless node $i created ..."			;# debug info

    [$wl_node_($i) set mac_(0)] set-channel 0
    [$wl_node_($i) set mac_(0)] set-diuc $diuc
    [$wl_node_($i) set mac_(0)] setflow UL $opt(be_sf_type) $opt(be_sf_traffic_priority) \
					    $opt(be_sf_mstr) $opt(be_sf_max_burst_size)	$opt(be_sf_mrtr) $opt(be_sf_regtrans_policy) \
					    $opt(be_sf_tolerated_jitter) $opt(be_sf_max_latency) $opt(be_sf_sdu_indicator) $opt(be_sf_sdu_size) \
					    $opt(be_sf_arq_enable) $opt(be_sf_arq_window) $opt(be_sf_arq_timer) $opt(be_sf_arq_proctime) $opt(be_sf_arq_blocksize) \
					    $opt(be_sf_grant_interval) $opt(be_sf_polling_interval) $opt(be_sf_time_base) $opt(be_sf_error_rate)

    [$wl_node_($i) set mac_(0)] setflow DL $opt(be_sf_type) $opt(be_sf_traffic_priority) \
					    $opt(be_sf_mstr) $opt(be_sf_max_burst_size)	$opt(be_sf_mrtr) $opt(be_sf_regtrans_policy) \
					    $opt(be_sf_tolerated_jitter) $opt(be_sf_max_latency) $opt(be_sf_sdu_indicator) $opt(be_sf_sdu_size) \
					    $opt(be_sf_arq_enable) $opt(be_sf_arq_window) $opt(be_sf_arq_timer) $opt(be_sf_arq_proctime) $opt(be_sf_arq_blocksize) \
					    $opt(be_sf_grant_interval) $opt(be_sf_polling_interval) $opt(be_sf_time_base) $opt(be_sf_error_rate)

   # [$wl_node_($i) set mac_(0)] setflow UL 10000 BE 275 2 0 0.05 1024 1 0 0 0 0 0 0 0 0 0 0 ;# setting up static flows 
   # [$wl_node_($i) set mac_(0)] setflow DL 10000 BE 275 2 0 0.05 1024 1 0 0 0 0 0 0 0 0 0 0 ;# setting up static flows 

    #create source traffic
    #Create a UDP agent and attach it to node n0
    set udp_($i) [new Agent/UDP]
    $udp_($i) set packetSize_ $packet_size

    if { $direction == "ul" } {
	$ns attach-agent $wl_node_($i) $udp_($i)
    } else {
	$ns attach-agent $sinkNode $udp_($i)
    }

    # Create a CBR traffic source and attach it to udp0
    set cbr_($i) [new Application/Traffic/CBR]
    $cbr_($i) set packetSize_ $packet_size
    $cbr_($i) set interval_ $gap_size
    $cbr_($i) attach-agent $udp_($i)

    #create an sink into the sink node

    # Create the Null agent to sink traffic
    set szen(sink_$i) [new Agent/WimaxMonitor] 
    if { $direction == "ul" } {    
	$ns attach-agent $sinkNode $szen(sink_$i)
    } else {
	$ns attach-agent $wl_node_($i) $szen(sink_$i)
    }
    
    # Attach the 2 agents
    $ns connect $udp_($i) $szen(sink_$i)
}

# create the link between sink node and base station
$ns duplex-link $sinkNode $bstation 100Mb 1ms DropTail

# Traffic scenario: if all the nodes start talking at the same
# time, we may see packet loss due to bandwidth request collision
set diff 0.02
for {set i 0} {$i < $nb_mn} {incr i} {
    $ns at [expr $traffic_start+$i*$diff] "$cbr_($i) start"
    $ns at [expr $traffic_stop+$i*$diff] "$cbr_($i) stop"
}

#$ns at 4 "$nd_(1) dump-table"
#$ns at 5 "$nd_(1) send-rs"
#$ns at 6 "$nd_(1) dump-table"
#$ns at 8 "$nd_(1) dump-table"

$ns at $simulation_stop "finish"
#$ns at $simulation_stop "$ns halt"
# Run the simulation
puts "Running simulation for $nb_mn mobile nodes..."
$ns at 1.0 "statistic"
$ns run
puts "Simulation done."




