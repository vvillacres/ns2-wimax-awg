#
# Copyright (c) @ Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/scuba/complete/demo.tcl,v 1.4 1997/11/29 07:11:00 elan Exp $
#

set tcldir ../../../

source $tcldir/rtp/session-scuba.tcl
source $tcldir/rtp/session-rtp.tcl

proc flash_annotate { start duration msg } {
	global ns
	$ns at $start "trace_annotate {$msg}"
#	$ns at [expr $start+$duration] "trace_annotate  {}"
}

set ns [new Simulator]
Simulator set EnableMcast_ 1
Simulator set NumberInterfaces_ 1

# rtcp reports
$ns color 32 red
# scuba reports
$ns color 33 green

$ns color 1 gold
$ns color 2 blue
$ns color 3 orange
$ns color 4 magenta

for { set i 0 } { $i < 8 } { incr i } {
	set node($i) [$ns node]
}
$node(3) shape "square"
$node(7) shape "square"

set f [open out.tr w]
$ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

Queue set limit_ 8

proc makelinks { bw delay pairs } {
        global ns node
        foreach p $pairs {
                set src $node([lindex $p 0])
                set dst $node([lindex $p 1])
                $ns duplex-link $src $dst $bw $delay DropTail
                $ns duplex-link-op $src $dst orient [lindex $p 2]
        }
}

makelinks 1.5Mb 10ms {
        { 0 3 right-down }
        { 1 3 right }
        { 2 3 right-up }
        { 7 4 right-up }
        { 7 5 right }
        { 7 6 right-down }
}

makelinks 400kb 50ms {
	{ 3 7 right }
}

$ns duplex-link-op $node(3) $node(7) queuePos 0.5
set mproto DM
set mrthandle [$ns mrtproto $mproto {}]


#for { set i 0 } { $i < 8 } { incr i } {
#	set dm($i) [new DM $ns $node($i)]
#}

$ns at 0.0 "$ns run-mcast"

set sessbw 400kb/s

foreach n { 0 1 2 4 5 6 } {
	set sess($n) [new Session/RTP]
	$sess($n) session_bw $sessbw
	$sess($n) attach-node $node($n)
}

$ns at 0.1 {
	global sess
	foreach n { 0 1 2 4 5 6 } {
		$sess($n) join-group 0x8000
	}
}

foreach n { 0 1 2 } {
	set d [$sess($n) set dchan_]
	$d set class_ [expr $n]
}

flash_annotate 0 0.20 "Equal bandwidth - Saturate"
flash_annotate 0.25 0.1 "Starting Receivers..."

# start receivers
$ns at 0.25 {
	global sess
	$sess(4) start
	$sess(5) start
	$sess(6) start
}

# start senders
flash_annotate 0.5 0.2 "Starting sender 0 at 200kb/s..."
$ns at 0.5 "$sess(0) start"
$ns at 0.5 "$sess(0) transmit 200kb/s"

flash_annotate 1.0 0.2 "Starting sender 1 at 200kb/s..."
$ns at 1.0 "$sess(1) start"
$ns at 1.0 "$sess(1) transmit 200kb/s"

flash_annotate 1.5 0.2 "Starting sender 2 at 200kb/s..."
$ns at 1.5 "$sess(2) start"
$ns at 1.5 "$sess(2) transmit 200kb/s"

$ns at 1.75 {
	global sess
	foreach n { 0 1 2 } {
		$sess($n) stop
	}
}

$ns at 2.0 "trace_annotate {End}"


$ns at 2.5 "trace_annotate {Equal Bandwidth - No Saturation}"

# start senders
flash_annotate 3.0 0.2 "Starting sender 0 at 130kb/s..."
$ns at 3.0 "$sess(0) start"
$ns at 3.0 "$sess(0) transmit 130kb/s"

flash_annotate 3.5 0.2 "Starting sender 1 at 130kb/s..."
$ns at 3.5 "$sess(1) start"
$ns at 3.5 "$sess(1) transmit 130kb/s"

flash_annotate 4.0 0.2 "Starting sender 2 at 130kb/s..."
$ns at 4.0 "$sess(2) start"
$ns at 4.0 "$sess(2) transmit 130kb/s"

$ns at 4.5 {
	global sess
	foreach n { 0 1 2 } {
		$sess($n) stop
	}
}

$ns at 5.0 "trace_annotate {End}"


set scuba_sim_time 5.5
$ns at $scuba_sim_time "scuba_sim $scuba_sim_time"
$ns at [expr $scuba_sim_time + 4.7] "finish"

proc scuba_sim { start } {
	global ns sess node sessbw

	flash_annotate $start 0.2 {SCUBA}
	foreach n { 0 1 2 4 5 6 } {
		set sess($n) [new Session/RTP/Scuba]
		$sess($n) session_bw $sessbw
		$sess($n) attach-node $node($n)
	}

	set t [expr $start+0.5]
	$ns at $t {
		global sess
		foreach n { 0 1 2 4 5 6 } {
			$sess($n) join-group 0x8000
		}
	}

	foreach n { 0 1 2 } {
		set d [$sess($n) set dchan_]
		$d set class_ [expr $n]
	}

	flash_annotate $t 0.2 {Starting receivers...}
	# start receivers
	$ns at $t {
		global sess
		$sess(4) start 0 1
		$sess(5) start 0 1
		$sess(6) start 0 1
	}

	# start senders
	flash_annotate [expr $start+1.1] 0.05 "Starting sender 0..."
	$ns at [expr $start+1.1] "$sess(0) start 1 0"

	flash_annotate [expr $start+1.2] 0.05 "Starting sender 1..."
	$ns at [expr $start+1.2] "$sess(1) start 1 0"

	flash_annotate [expr $start+1.3] 0.05 "Starting sender 2..."
	$ns at [expr $start+1.3] "$sess(2) start 1 0"

	# 4 focus on 0
	flash_annotate [expr $start+1.5] 0.1 "4 focussing on 0..."
	$ns at [expr $start+1.5] "[$sess(4) set repAgent_] set class_ 4"
	$ns at [expr $start+1.5] "$sess(4) scuba_focus $sess(0)"
	$ns at [expr $start+1.55] "[$sess(4) set repAgent_] set class_ 33"

	# 5 focus on 1
	flash_annotate [expr $start+2.0] 0.1 "5 focussing on 1..."
	$ns at [expr $start+2.0] "[$sess(5) set repAgent_] set class_ 4"
	$ns at [expr $start+2.0] "$sess(5) scuba_focus $sess(1)"
	$ns at [expr $start+2.05] "[$sess(5) set repAgent_] set class_ 33"

	# 5 focus on 0
	flash_annotate [expr $start+2.5] 0.1 "5 focussing on 0..."
	$ns at [expr $start+2.5] "[$sess(5) set repAgent_] set class_ 4"
	$ns at [expr $start+2.5] "$sess(5) scuba_focus $sess(0)"
	$ns at [expr $start+2.55] "[$sess(5) set repAgent_] set class_ 33"

	# 5 unfocus on 0
	flash_annotate [expr $start+3.0] 0.1 "5 unfocussing off 0..."
	$ns at [expr $start+3.0] "[$sess(5) set repAgent_] set class_ 4"
	$ns at [expr $start+3.0] "$sess(5) scuba_unfocus $sess(0)"
	$ns at [expr $start+3.05] "[$sess(5) set repAgent_] set class_ 33"

	# 4,5 unfocus on 1,0  and focus on 2
	flash_annotate [expr $start+3.5] 0.1 "4,5 unfocussing off 1,0 and focus on 2..."
	$ns at [expr $start+3.5] "[$sess(5) set repAgent_] set class_ 4"
	$ns at [expr $start+3.5] "[$sess(4) set repAgent_] set class_ 4"
	$ns at [expr $start+3.5] "$sess(5) scuba_unfocus $sess(1)"
	$ns at [expr $start+3.5] "$sess(4) scuba_unfocus $sess(0)"
	$ns at [expr $start+3.5] "$sess(5) scuba_focus $sess(2)"
	$ns at [expr $start+3.5] "$sess(4) scuba_focus $sess(2)"
	$ns at [expr $start+3.55] "[$sess(5) set repAgent_] set class_ 33"
	$ns at [expr $start+3.55] "[$sess(4) set repAgent_] set class_ 33"
}

proc finish {} {
	trace_annotate "End Simulation"
	global tcldir
	puts "converting output to nam format..."
        global ns
        $ns flush-trace

	puts "running nam..."
	exec /usr/local/src/nam-1/nam out.nam &
        exit 0
}

$ns run

