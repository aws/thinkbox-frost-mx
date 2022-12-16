-- Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
-- SPDX-License-Identifier: Apache-2.0
global Frost_ConvertToPRT_Dialog
global Frost_ConvertToPRT_InFilename = ""
global Frost_ConvertToPRT_OutFilename = ""
global Frost_ConvertToPRT_LockInFilename = false

rollout Frost_ConvertToPRT_Dialog "Frost - Convert to PRT File"
(
	group "Input Sequence"
	(
		edittext edt_inputPath align:#left width:438 across:2
		button btn_inputLocation "..." offset:[0,-2] align:#right width:32 height:20
	)
	
	group "Output Sequence"
	(
		edittext edt_outputPath align:#left width:438 across:2
		button btn_outputLocation "..." offset:[0,-2] align:#right width:32 height:20
	)
	
	group "Timing"
	(
		radiobuttons rad_frameRangeMode labels:#("Single File", "Frame Range:") columns:1 align:#left across:4
		spinner spn_frameStart type:#integer range:[-100000,100000,0] offset:[-80,16]
		spinner spn_frameEnd " to " type:#integer range:[-100000,100000,0] offset:[-116,16]
		button btn_setRangeFromFiles "Set Using Existing Frames" align:#right
	)
	
	label lbl_notify align:#left offset:[0,4] across:3
	button btn_ok "OK" width:75 align:#right offset:[70,0]
	button btn_cancel "Cancel" width:75 align:#right
	
	fn update_enables =
	(
		if rad_frameRangeMode.state == 1 then
		(
			enable = false
		)
		else if rad_frameRangeMode.state == 2 then
		(
			enable = true
		)
		spn_frameStart.enabled = spn_frameEnd.enabled = enable
	)
	
	fn update_output_from_input =
	(
		local infile = edt_inputPath.text
		if infile.count > 0 do
		(
			try
			(
				edt_outputPath.text = ( getFilenamePath infile ) + ( getFilenameFile infile ) + ".prt"
			)
			catch
			(
			)
		)
	)
	
	fn update_timing_from_input showErrors:false =
	(
		local fileExists = doesFileExist edt_inputPath.text
		local range = FrostUtils.GetFileSequenceRange edt_inputPath.text
		local sequenceExists = ( range != undefined ) and ( range.count == 2 ) and ( range[1] <= range[2] )
		
		if sequenceExists then
		(
			rad_frameRangeMode.state = 2
			spn_frameStart.value = range[1]
			spn_frameEnd.value = range[2]
			update_enables()
		)
		else if fileExists then
		(
			rad_frameRangeMode.state = 1
			update_enables()
		)
		else
		(
			if showErrors do
			(
				messageBox ( "Cannot determine range from missing file: " + edt_inputPath.text ) title:"Frost Error"
			)
		)
	)

	on btn_inputLocation pressed do
	(
		newPath = getOpenFileName caption:"Select File to Convert" filename:edt_inputPath.text types:"All Particle Files|*.prt;*.sprt;*.bin;*.rpc;*.csv;*.las;*.e57;*.pts;*.ptx;*.xyz;*.ptg;*.ply|Thinkbox Particle Files (*.prt, *.sprt)|*.prt;*.sprt|RealFlow Particle Files (*.bin, *.rpc)|*.bin;*.rpc|CSV Files (*.csv)|*.csv|LAS Files (*.las)|*.las|E57 Files (*.e57)|*.e57|PTS Files (*.pts)|*.pts|PTX Files (*.ptx)|*.ptx|XYZ Files (*.xyz)|*.xyz|PTG Files (*.ptg)|*.ptg|PLY Files (*.ply)|*.ply|All Files (*.*)|*.*|"
		
		if newPath != undefined do
		(
			edt_inputPath.text = newPath
			update_output_from_input()
			update_timing_from_input()
		)
	)
	
	on btn_outputLocation pressed do 
	(
		newPath = getSaveFileName caption:"Select Save Location for PRT Files" filename:edt_outputPath.text types:"Thinkbox Particle Files (*.prt)|*.prt|"
		if newPath != undefined do
		(
			edt_outputPath.text = newPath
		)
	)
	
	on rad_frameRangeMode changed state do
	(
		update_enables()
	)
	
	on Frost_ConvertToPRT_Dialog open do
	(
		edt_inputPath.text = Frost_ConvertToPRT_InFilename
		update_output_from_input()
		update_timing_from_input()
		update_enables()
		if Frost_ConvertToPRT_LockInFilename then
		(
			lbl_notify.text = "You must copy this file to PRT format before opening."
			edt_inputPath.enabled = false
			btn_inputLocation.enabled = false
		)
	)
	
	on btn_setRangeFromFiles pressed do
	(
		update_timing_from_input showErrors:true
	)
	
	on btn_ok pressed do
	(
		local done = false
		try
		(
			if rad_frameRangeMode.state == 1 then
			(
				done = FrostUtils.ConvertParticleFile edt_inputPath.text edt_outputPath.text
			)
			else if rad_frameRangeMode.state == 2 then
			(
				done = FrostUtils.ConvertParticleFileSequence edt_inputPath.text edt_outputPath.text spn_frameStart.value spn_frameEnd.value
			)
		)
		catch
		(
			messageBox (getCurrentException()) title:"Error"
		)
		if done do
		(
			Frost_ConvertToPRT_OutFilename = edt_outputPath.text
			destroyDialog Frost_ConvertToPRT_Dialog
		)
	)
	
	on btn_cancel pressed do
	(
		Frost_ConvertToPRT_OutFilename = ""
		destroyDialog Frost_ConvertToPRT_Dialog
	)
)
