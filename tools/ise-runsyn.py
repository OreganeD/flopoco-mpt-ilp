##
################################################################################
##             Compilation and Result Generation script for ISE
## This tool is part of  FloPoCo
## Author:  Florent de Dinechin, 2016, from a bash script by B. Pasca
## All rights reserved
################################################################################

import os
import sys
import re
import string
import argparse

def report(text):
    print "ise_runsyn: " + text

    
def get_compile_info(filename):
    vhdl=open(filename).read()

    # last entity
    endss = [match.end() for match in re.finditer("entity", vhdl)] # list of endpoints of match of "entity"
    last_entity_name_start = endss[-2] +1 # skip the space
    i = last_entity_name_start
    while(vhdl[i]!=" "):
        i=i+1
    last_entity_name_end = i
    entityname=vhdl[last_entity_name_start:last_entity_name_end]

    # target 
    endss = [match.end() for match in re.finditer("-- VHDL generated for", vhdl)] # list of endpoints of match of "entity"
    target_name_start = endss[-1] +1
    i = target_name_start
    while(vhdl[i]!=" "):
        i=i+1
    target_name_end = i
    targetname=vhdl[target_name_start:target_name_end]
    
    # the frequency follows but we don't need to read it so far
    frequency_start=target_name_end+3 #  skip " @ "
    i = frequency_start
    while(vhdl[i]!=" " and vhdl[i]!="M"): # 400MHz or 400 MHz
        i=i+1
    frequency_end = i
    frequency = vhdl[frequency_start:frequency_end]

    return (entityname, targetname, frequency)



#/* main */
if __name__ == '__main__':


    parser = argparse.ArgumentParser(description='This is an helper script for FloPoCo that launches Xilinx Vivado and extracts resource consumption and critical path information')
    parser.add_argument('-i', '--implement', action='store_true', help='Go all the way to implementation (default stops after synthesis)')
    parser.add_argument('-v', '--vhdl', help='VHDL file name (default flopoco.vhdl)')
    parser.add_argument('-e', '--entity', help='Entity name (default is last entity of the VHDL file)')
    parser.add_argument('-t', '--target', help='Target name (default is read from the VHDL file)')
    parser.add_argument('-f', '--frequency', help='Objective frequency (default is read from the VHDL file)')
    parser.add_argument('-d', '--dsp', help='use DSP blocks (should be yes, no or auto, default is auto)')

    options=parser.parse_args()


    if (options.implement==True):
        synthesis_only=False
    else:
        synthesis_only=True

    if (options.vhdl==None):
        filename = "flopoco.vhdl"
    else:
        filename = options.vhdl

    # Read from the vhdl file the entity name and target hardware and freqyency
    report("Reading file " + filename)
    (entity_in_file, target_in_file, frequency_in_file) =  get_compile_info(filename)

    if (options.entity==None):
        entity = entity_in_file
    else:
        entity = options.entity

    if (options.target==None):
        target = target_in_file
    else:
        target = options.target

    if (options.frequency==None):
        frequency = frequency_in_file
    else:
        frequency = options.frequency

    if (options.dsp==None):
        dsp = "auto"
    elif options.dsp in ["yes", "no", "true"]:
        dsp = options.dsp
    else :
        raise BaseException("option for dsp should be yes, true or auto ")
       
        

    report("   entity:     " +  entity)
    report("   target:     " +  target)
    report("   frequency:  " +  frequency + " MHz")
    report("   dsp:        " +  dsp)

    target = target.lower()
    if target=="virtex4":
        part="xc4vfx100-12-ff1152"
    elif target.lower()=="virtex5":
        part="xc5vfx100T-3-ff1738"
    elif target.lower()=="virtex6":
        part="xc6vhx380T-3-ff1923"
    #Add your part here 
    else:
        raise BaseException("Target " + target + " not supported")
        
    workdir="/tmp/ise_runsyn_"+entity+"_"+target+"_"+frequency
    os.system("rm -R "+workdir)
    os.mkdir(workdir)
    os.system("cp flopoco.vhdl "+workdir)
    os.chdir(workdir)
    

    prj_file_name = entity+".prj"
    report("Writing PRJ file " + prj_file_name)
    prj_file = open(prj_file_name,"w")
    prj_file.write('vhdl work "flopoco.vhdl"')
    prj_file.close()

    lso_file_name = entity+".lso"
    report("Writing LSO file " + lso_file_name)
    lso_file = open(lso_file_name,"w")
    lso_file.write('work')
    lso_file.close()


    xst_file_name = entity+".xst"
    report("Writing XST file " + xst_file_name)
    xst_file = open(xst_file_name,"w")
    xst_file.write("# This file was created by the ise_runsyn utility. Sorry to clutter your tmp.\n")
    os.mkdir("xst")
    os.mkdir("xst/projnav.tmp")
    
    
    xst_file.write('''
set -xsthdpdir "./xst"
set -tmpdir "./xst/projnav.tmp"
run
''')
    xst_file.write('-ifn ' + entity+'.prj'+'\n')
    xst_file.write('-ofn ' + entity+'\n')
    xst_file.write('-top ' + entity+'\n')
    xst_file.write('-lso ' + entity +'.lso'+'\n')
    xst_file.write('-p ' + part+'\n')
    xst_file.write('-use_dsp48 ' + dsp+'\n')
    xst_file.write('''-iuc NO
-ifmt mixed
-ofmt NGC
-opt_mode Speed
-opt_level 2
-power NO
-keep_hierarchy NO
-netlist_hierarchy as_optimized
-rtlview Yes
-glob_opt AllClockNets
-read_cores YES
-write_timing_constraints NO
-cross_clock_analysis NO
-hierarchy_separator /
-bus_delimiter <>
-case maintain
-slice_utilization_ratio 100
-bram_utilization_ratio 100
-dsp_utilization_ratio 100
-lc auto
-reduce_control_sets auto
-fsm_extract YES -fsm_encoding Auto
-safe_implementation No
-fsm_style LUT
-ram_extract Yes
-ram_style Auto
-rom_extract Yes
-shreg_extract YES
-rom_style Auto
-auto_bram_packing YES
-resource_sharing YES
-async_to_sync NO
-shreg_min_size 2
-iobuf YES
-max_fanout 100000
-bufg 32
-register_duplication YES
-register_balancing NO
-move_first_stage NO
-move_last_stage NO
-optimize_primitives YES
-use_clock_enable Auto
-use_sync_set Auto
-use_sync_reset Auto
-iob auto
-equivalent_register_removal YES
-slice_utilization_ratio_maxmargin 5"
''')
    if(target == "virtex4"):
	xst_file.write('-bufr 16')

    xst_file.close()


    xst_command = "xst  -ifn " + entity +".xst -ofn " + entity + ".syr | tee console.out"
    report("Running the xst command: " + xst_command)
    os.system(xst_command)
        

