
##############################################################################
#                       CONFIGURATION OF PARAMETERS                          #
##############################################################################

# Simulation enviroment
###################################

set opt(output_dir) "."					;# output directory
set opt(output_tracefile) out.tr			;# trace output filename
set opt(output_statfile) statistic.tr			;# statistic output filename
set opt(simu_stop) 23.0					;# duration of the simulation
set opt(debug)	""					;# debug switch

# set opt(scheduler) 3					;# version of the scheduler




# Szenario definition
##################################

# Topologie
set opt(topo_x) 2000;					;# x-dimension of the simulation area
set opt(topo_y) 2000;					;# y-dimension of the simulation area

# Position of nodes
set opt(szen_sn_x) 1000;				;# x-position of the sink node
set opt(szen_sn_y) 1001;				;# y-position of the sink node

set opt(szen_bs_x) 1000;				;# x-position of the base station
set opt(szen_bs_y) 1000;				;# y-position of the base station

set opt(szen_bsant_z) 32				;# heigth of base station antenna
set opt(szen_bsant_gain) 63.09				;# gain of base station antenna

# ms
set opt(szen_be_x) 1010;
set opt(szen_be_y) 1000;

set opt(szen_rt_x) 990;
set opt(szen_rt_y) 1000;

set opt(szen_ssant_z) 1.5				;# heigth of subscriber station antenna
set opt(szen_ssant_gain) 15.84				;# gain of subscriber station antenna

set opt(szen_start_motion) 11;
set opt(szen_speed) 0.833333


set opt(szen_number_ms) 1;
set opt(szen_diuc) 7;


# Traffic parameters
####################################

# Generatal Traffic parameters
set opt(traffic_start) 10;				;# starttime of traffic generation
 set opt(traffic_stop) 22;				;# stoptime of traffic generation
set opt(traffic_start_diff) 0.01			;# difference between traffic starts

# Trace Parameters
set opt(trace_sampling_time)	0.1			;# samplingtime (s) for tracefile
# set opt(trace_header)		20			;# ip packet header
set opt(trace_start) 11
set opt(trace_stop) 21

# BE Traffic parameters
set opt(packet_size_be)	1500;
set opt(datarate_be) 1.5e+7;
set opt(start_diff_be) 0;
puts "datarate be=$opt(datarate_be)"

# BE Service Flow
set opt(be_sf_direction)	"DL"			;# DL - Downlink, UL - Uplink
set opt(be_sf_type)		"BE"			;# DL Data deivery service type UGS/ERT-VR/RT-VR/NRT-VR/BE
							;# UL grant scheduling type UGS/ertPS/rtPS/nrtPS/BE
set opt(be_sf_traffic_priority) 0			;# Traffic Priority
set opt(be_sf_mstr)		2.0e+7			;# Maximum Sustained Traffic Rate in bit/s
set opt(be_sf_max_burst_size)	0			;# Maximum Burst Size in byte
set opt(be_sf_mrtr)		2.0e+7			;# Minimum Reserved Traffic Rate in bit/s
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

set opt(be_sf_grant_interval)	5			;# Unsolicited Grant Interval in ms
set opt(be_sf_polling_interval) 5			;# Unsolicited Polling Interval in ms

set opt(be_sf_time_base)	50			;# Time Base

set opt(be_sf_error_rate)	0.0			;# Packet Error Rate






# WiMAX Parameters
###################################

# Parameter for wireless nodes
set opt(wimax_chan)           Channel/WirelessChannel   ;# channel type
set opt(wimax_prop)           Propagation/OFDMA         ;# radio-propagation model
set opt(wimax_netif)          Phy/WirelessPhy/OFDMA     ;# network interface type
set opt(wimax_mac)            Mac/802_16/BS             ;# MAC type
set opt(wimax_ifq)            Queue/DropTail/PriQueue   ;# interface queue type
set opt(wimax_ll)             LL                        ;# link layer type
set opt(wimax_ant)            Antenna/OmniAntenna       ;# antenna model
set opt(wimax_ifqlen)         50			;# max packet in ifq
set opt(wimax_adhocRouting)   NOAH                      ;# routing protocol

# WiMAX Frame
set opt(wimax_debug)	1
set opt(wimax_rtg)	232
set opt(wimax_ttg)	232
set opt(wimax_frame_duration)	0.005
set opt(wimax_dlratio)	0.5

# WiMAX Phy
set opt(wimax_bandwidth)	1e7			;# in Hz
set opt(wimax_ITU_PDP)	1
set opt(wimax_channel_model)	"PED_A"			;# Channel Model "PED_A" "PED_B" "VEHIC_A"
set opt(wimax_disable_interference)	0

# Physical Parameters
set opt(phy_ofdma)	1				;# OFMDA used
set opt(phy_cp)	0.125					;# OFDM cylcic prefix
set opt(phy_transmitter_power)	0.5			;# in watt
set opt(phy_RXThresh)	1.90546e-16			;# Defines the distribution range of a packet
set opt(phy_CSThresh)	[expr 0.9 * $opt(phy_RXThresh)]




##############################################################################
#                       DEFINITION OF PROCEDURES                             #
##############################################################################

##################
# Calculation of parameters
##################
proc calc_parameters {} {
	global opt

	#set opt(traffic_stop) [expr $opt(simu_stop) - 1.0]
	#set opt(trace_stop) $opt(traffic_stop)


}

##################
# parse command-line options and store values into the $opt(.) hash
# syntax: ns skript.tcl -parameter value
##################
proc getopt {argc argv} {
        global opt

	set optlength [ array size opt ] 

        for {set i 0} {$i < $argc} {incr i} {
                set arg [lindex $argv $i]
                if {[string range $arg 0 0] != "-"} continue

                set name [string range $arg 1 end]
                set opt($name) [lindex $argv [expr $i+1]]
		puts "$name : [lindex $argv [expr $i+1]]"
        }

	if { $optlength != [ array size opt ] } {
	  puts "Zusätzliches Element in opt / Tippfehler"
	  exit 
	}
}

###################
# print out options
###################
proc printopt { } {
        global opt
	puts "### Debug Output ###"
        foreach x [lsort [array names opt]] {
                puts "$x = $opt($x)"
        }
	puts "### Debug Output ###"
}

###################
# die function
###################
proc die { x } {
        puts $x
        exit 1
}

###################
# alive function
###################
proc alive { } {
        global ns opt

        if { [$ns now] != 0 } {
                puts -nonewline \
                 [format "elapsed %.0f s (remaining %.0f s) completed %.f%%" \
                 [$ns now] \
                 [expr $opt(simu_stop) - [$ns now]] \
                 [expr 100 * [$ns now] / $opt(simu_stop)]]
                if { [$ns now] >= $opt(traffic_start) } {
                        puts " stat collection ON"
                } else {
                        puts ""
                }
        }
        $ns at [expr [$ns now] + $opt(simu_stop) / 10.0] "alive"
}


##################
# init
##################
proc init {} {
    global opt defaultRNG ns

    # create the simulator instance
    set ns [new Simulator]
    $defaultRNG seed 1

    # initialize trace file
    $ns use-newtrace
    set opt(tf) [open $opt(output_dir)/$opt(output_tracefile) w]
    #set opt(tf) [open /dev/zero w]
    $ns trace-all $opt(tf)
    
    # initialize statistic file
    set opt(statistic_file) [open $opt(output_dir)/$opt(output_statfile) w]
    set opt(oldnow) 0.0

    # initialize wimax frame
    Mac/802_16 set debug_		$opt(wimax_debug)
    Mac/802_16 set rtg_			$opt(wimax_rtg)
    Mac/802_16 set ttg_			$opt(wimax_ttg)
    Mac/802_16 set frame_duration_  	$opt(wimax_frame_duration)
    Mac/802_16/BS set dlratio_      	$opt(wimax_dlratio)
    Mac/802_16/SS set dlratio_		$opt(wimax_dlratio)

    #initialize wimax phy		
    Mac/802_16 set fbandwidth_		$opt(wimax_bandwidth)
    Mac/802_16 set ITU_PDP_		$opt(wimax_ITU_PDP)
    Mac/802_16 set disable_interference_ $opt(wimax_disable_interference)

    # initialize physical parameters
    Phy/WirelessPhy set OFDMA_		$opt(phy_ofdma)
    Phy/WirelessPhy/OFDMA set g_	$opt(phy_cp)
    Phy/WirelessPhy set Pt_		$opt(phy_transmitter_power)
    Phy/WirelessPhy set RXThresh_	$opt(phy_RXThresh)
    Phy/WirelessPhy set CSThresh_	$opt(phy_CSThresh)

}

####################
# topologie
####################
proc szenario {} {
    global opt ns szen

    # start time of simulation
    set szen(simtime) [clock seconds]

    # create topologie
    set szen(topo) [new Topography]
    $szen(topo) load_flatgrid $opt(topo_x) $opt(topo_y)

    # create God
    create-god [expr ($opt(szen_number_ms) + 2)]				;# nb_mn + 2 (base station and sink node)
    #puts "God node created"

    # set up for hierarchical routing (needed for routing over a basestation)
    $ns node-config -addressType hierarchical
    AddrParams set domain_num_ 2          			;# domain number
    lappend cluster_num 1 1  	          			;# cluster number for each domain 
    AddrParams set cluster_num_ $cluster_num
    lappend eilastlevel 1 [expr ($opt(szen_number_ms) + 1)]	;# number of nodes for each cluster (1 for sink and one for mobile nodes + base station
    AddrParams set nodes_num_ $eilastlevel
    puts "Configuration of hierarchical addressing done"

    # creates the sink node in first addressing space.
    set szen(sinkNode) [$ns node 0.0.0]
    $szen(sinkNode) set X_  $opt(szen_sn_x)
    $szen(sinkNode) set Y_  $opt(szen_sn_y)
    $szen(sinkNode) set Z_  0.0

    # configure bs antenna
    Antenna/OmniAntenna set X_ 0
    Antenna/OmniAntenna set Y_ 0
    Antenna/OmniAntenna set Z_ $opt(szen_bsant_z)
    Antenna/OmniAntenna set Gt_ $opt(szen_bsant_gain)
    Antenna/OmniAntenna set Gr_ $opt(szen_bsant_gain)

    # creates the base station
    $ns node-config -adhocRouting $opt(wimax_adhocRouting) \
		    -llType $opt(wimax_ll) \
		    -macType Mac/802_16/BS \
		    -ifqType $opt(wimax_ifq) \
		    -ifqLen $opt(wimax_ifqlen) \
		    -antType $opt(wimax_ant) \
		    -propType $opt(wimax_prop)    \
		    -phyType $opt(wimax_netif) \
		    -channel [new $opt(wimax_chan)] \
		    -topoInstance $szen(topo) \
		    -wiredRouting ON \
		    -agentTrace ON \
		    -routerTrace ON \
		    -macTrace ON  \
		    -movementTrace OFF
  
    set szen(bstation) [$ns node 1.0.0]  
    $szen(bstation) random-motion 0
    $szen(bstation) set X_ $opt(szen_bs_x)
    $szen(bstation) set Y_ $opt(szen_bs_y)
    $szen(bstation) set Z_ 0.0
    [$szen(bstation) set mac_(0)] set-channel 0

 #   WimaxScheduler/BSEXT$opt(scheduler) set Repetition_code_ 1
 #   WimaxScheduler/BSEXT$opt(scheduler) set init_contention_size_ 5
 #   WimaxScheduler/BSEXT$opt(scheduler) set bw_req_contention_size_ 5
 #   WimaxScheduler/BSEXT$opt(scheduler) set dlratio_      	$opt(wimax_dlratio)
 #   set szen(bsscheduler) [new WimaxScheduler/BSEXT$opt(scheduler)]
 #   [$szen(bstation) set mac_(0)] set-scheduler $szen(bsscheduler)

    # setup channel model
    set prop_inst [$ns set propInstance_]
    $prop_inst ITU_PDP $opt(wimax_channel_model)
    
    # create the link between sink node and base station
    $ns duplex-link $szen(sinkNode) $szen(bstation) 100Mb 1ms DropTail

    # configure ss antenna
    Antenna/OmniAntenna set X_ 0
    Antenna/OmniAntenna set Y_ 0
    Antenna/OmniAntenna set Z_ $opt(szen_ssant_z)
    Antenna/OmniAntenna set Gt_ $opt(szen_ssant_gain)
    Antenna/OmniAntenna set Gr_ $opt(szen_ssant_gain)

    # creates mobile stations
    $ns node-config -macType Mac/802_16/SS \
		    -antType $opt(wimax_ant) \
		    -wiredRouting OFF \
		    -macTrace OFF  				;# Mobile nodes cannot do routing.
#
    set szen(wl_node_be) [$ns node 1.0.1]
    $szen(wl_node_be) random-motion 0
    $szen(wl_node_be) base-station [AddrParams addr2id [$szen(bstation) node-addr]] ;#attach mn to basestation
    $szen(wl_node_be) set X_ $opt(szen_be_x)
    $szen(wl_node_be) set Y_ $opt(szen_be_y)
    $szen(wl_node_be) set Z_ 0.0
    [$szen(wl_node_be) set mac_(0)] set-channel 0
    [$szen(wl_node_be) set mac_(0)] set-diuc $opt(szen_diuc)

    # defines movement of mobile nodes
    #$ns at $opt(szen_start_motion) "$szen(wl_node_be) setdest [expr ($opt(topo_x)-1)] $opt(szen_be_y) $opt(szen_speed)"

 #   set szen(wl_node_rt) [$ns node 1.0.2]
 #   $szen(wl_node_rt) random-motion 0
 #   $szen(wl_node_rt) base-station [AddrParams addr2id [$szen(bstation) node-addr]] ;#attach mn to basestation
 #   $szen(wl_node_rt) set X_ $opt(szen_rt_x)
 #   $szen(wl_node_rt) set Y_ $opt(szen_rt_y)
 #   $szen(wl_node_rt) set Z_ 0.0
 #   [$szen(wl_node_rt) set mac_(0)] set-channel 0
 #   [$szen(wl_node_rt) set mac_(0)] set-diuc $opt(szen_diuc)

}

####################
# traffic configuration
####################
proc traffic {} {
    global opt ns szen

    # configure service flows
    #########################

    # be node
	# be node
#    [$szen(wl_node_be) set mac_(0)] setflow $opt(be_sf_direction) $opt(be_sf_type) $opt(be_sf_traffic_priority) $opt(be_sf_mstr) \
#					$opt(be_sf_max_burst_size) $opt(be_sf_mrtr) $opt(be_sf_tolerated_jitter) $opt(be_sf_max_latency) \
#					$opt(be_sf_sdu_indicator) $opt(be_sf_sdu_size) $opt(be_sf_regtrans_policy) $opt(be_sf_traget_said) \
#					$opt(be_sf_grant_interval) $opt(be_sf_polling_interval) $opt(be_sf_arq_enable) $opt(be_sf_arq_timer) \
#					$opt(be_sf_arq_window) $opt(be_sf_arq_ack_period)


    [$szen(wl_node_be) set mac_(0)] setflow $opt(be_sf_direction) $opt(be_sf_type) $opt(be_sf_traffic_priority) \
					    $opt(be_sf_mstr) $opt(be_sf_max_burst_size)	$opt(be_sf_mrtr) $opt(be_sf_regtrans_policy) \
					    $opt(be_sf_tolerated_jitter) $opt(be_sf_max_latency) $opt(be_sf_sdu_indicator) $opt(be_sf_sdu_size) \
					    $opt(be_sf_arq_enable) $opt(be_sf_arq_window) $opt(be_sf_arq_timer) $opt(be_sf_arq_proctime) $opt(be_sf_arq_blocksize) \
					    $opt(be_sf_grant_interval) $opt(be_sf_polling_interval) $opt(be_sf_time_base) $opt(be_sf_error_rate)


 #   # rt node
  #  [$szen(wl_node_rt) set mac_(0)] setflow $opt(rt_sf_direction) $opt(rt_sf_type) $opt(rt_sf_traffic_priority) $opt(rt_sf_mstr) \
#					$opt(rt_sf_max_burst_size) $opt(rt_sf_mrtr) $opt(rt_sf_tolerated_jitter) $opt(rt_sf_max_latency) \
#					$opt(rt_sf_sdu_indicator) $opt(rt_sf_sdu_size) $opt(rt_sf_regtrans_policy) $opt(rt_sf_traget_said) \
#					$opt(rt_sf_grant_interval) $opt(rt_sf_polling_interval) $opt(rt_sf_arq_enable) $opt(rt_sf_arq_timer) \
#					$opt(rt_sf_arq_window) $opt(rt_sf_arq_ack_period)



    # create source traffic
    #######################
    
    # create a UDP agent and attach it to source node
    set szen(udp_agent_0) [new Agent/UDP]
    $szen(udp_agent_0) set packetSize_ $opt(packet_size_be)

    if { $opt(be_sf_direction) == "DL" } {
      $ns attach-agent $szen(sinkNode) $szen(udp_agent_0)
    } else {
      $ns attach-agent $szen(wl_node_be) $opt(udp_agent_0)
    }
    
    # create a CBR traffic source and attach it to upd agent
    set szen(cbr_0) [new Application/Traffic/CBR]
    $szen(cbr_0) set packetSize_ $opt(packet_size_be)
    $szen(cbr_0) set rate_ $opt(datarate_be)
    $szen(cbr_0) attach-agent $szen(udp_agent_0)

    ### rt service

#    # create a UDP agent and attach it to source node
#    set szen(udp_agent_1) [new Agent/UDP]
#    $szen(udp_agent_1) set packetSize_ $opt(packet_size_rt)

#    if { $opt(rt_sf_direction) == "DL" } {
#      $ns attach-agent $szen(sinkNode) $szen(udp_agent_1)
#    } else {
#      $ns attach-agent $szen(wl_node_rt) $opt(udp_agent_1)
#    }
    
#    # create a CBR traffic source and attach it to upd agent
#    set szen(cbr_1) [new Application/Traffic/CBR]
#    $szen(cbr_1) set packetSize_ $opt(packet_size_rt)
#    $szen(cbr_1) set rate_ 10000000
#    $szen(cbr_1) attach-agent $szen(udp_agent_1)

#	puts [$szen(cbr_1) set packetSize_ ]
#	puts [$szen(cbr_1) set rate_ ]
	


    # create traffic sink
    #####################

    # create the statistic agent and attach it to sink node
    set szen(sink_0) [new Agent/WimaxMonitor]
    if { $opt(be_sf_direction) == "DL" } {
      $ns attach-agent $szen(wl_node_be) $szen(sink_0)
    } else {
      $ns attach-agent $szen(sinkNode) $szen(sink_0)
    }

    # connect source and sink
    $ns connect $szen(udp_agent_0) $szen(sink_0)

    ### rtps

    # create the statistic agent and attach it to sink node
#    set szen(sink_1) [new Agent/StatisticMonitor]
#    if { $opt(rt_sf_direction) == "DL" } {
#      $ns attach-agent $szen(wl_node_rt) $szen(sink_1)
#    } else {
#      $ns attach-agent $szen(sinkNode) $szen(sink_1)
#    }

    # connect source and sink
#    $ns connect $szen(udp_agent_1) $szen(sink_1)


    # START TRAFFIC
    #####################

    # Traffic scenario: if all the nodes start talking at the same
    # time, we may see packet loss due to bandwidth request collision
 #   set opt(current_datarate_be) 0.0

    $ns at [expr $opt(traffic_start)] "$szen(cbr_0) start"
    $ns at [expr $opt(traffic_stop)] "$szen(cbr_0) stop"

   
  #  $ns at [expr $opt(traffic_start)] "$szen(cbr_1) start"	
  #  $ns at [expr $opt(traffic_start)] "change_rt_traffic"
  #  $ns at [expr $opt(traffic_start) + $opt(stage_interval)] "change_rt_stage" 
  #  $ns at [expr $opt(traffic_start) + 12 * $opt(stage_interval)] "$szen(cbr_1) stop"

}



###################
# finish simulation
###################

#defines function for flushing and closing files
proc finish {} {
    global ns opt szen
    $ns flush-trace
    close $opt(output_tracefile)
    close $opt(output_statfile)
    
    # print out the simulation time    
    set simtime [expr [clock seconds] - $szen(simtime)]
    puts "run duration: $simtime s"
    exit 0
}

###################
# create statistic procedure
###################
proc statistic {} {
    global array names  szen opt ns 

    set now [$ns now]


    if { ( ($now >= $opt(trace_start)) && ($now <= $opt(trace_stop)) )} {
      
      # write data in statistic file
      set statdata "$now"
      set recv_bytes_sum 0

      for {set i 0} {$i < ($opt(szen_number_ms) )} {incr i} {
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
      for {set i 0} {$i < ($opt(szen_number_ms) )} {incr i} {
	  $szen(sink_$i) reset
      }
    }

    set opt(oldnow) $now;

    # restart statistic after $opt(trace_sampling_time)
    $ns at [expr ($now + $opt(trace_sampling_time)) ] "statistic"

}    


##############################################################################
#                       MAIN BODY		                             #
##############################################################################

getopt $argc $argv
calc_parameters
if { $opt(debug) != "" } {
        printopt
}
init
szenario
traffic
puts "start stat"
alive
$ns at 1.0 "statistic"
$ns at $opt(simu_stop) "finish"
puts "Running simulation..."
$ns run
puts "Simulation done."
