-- Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
-- SPDX-License-Identifier: Apache-2.0
(--start local scope
global SMTDSettings, SMTDFunctions, SMTDPaths
global Frost_Deadline_GUI_Main
global FrostDeadlineStruct

local prtMesherWorkerCommand = "v:\\3dtools_synch\\ParticleMesher\\particle_mesher.py "

fn getFrostNode = (
	if Frost_Deadline_GUI_Main != undefined then (
		theNode = Frost_Deadline_GUI_Main.theFrostNode
		if isValidNode theNode then (
			return theNode
		)
	)
	throw "Frost Error: Could not get the Flood Particle Mesher node."
)

struct PRTMesherConfigWriter (
	-- helper for PRTMesherConfig which handles writing config sections and options
	outFile,
	fn write_section s = (
		format "\n[%]\n" s to:outFile
	),
	fn write_option opt val = (
		if classOf val == Float then (
			format "% = %\n" opt (formattedPrint val format:".8g") to:outFile
		)
		else (
			format "% = %\n" opt val to:outFile
		)
	)
)

struct PRTMesherConfig (
	fn writeGlobal cfg cacheName isProxy = (
		theNode = getFrostNode ()
		cfg.write_section "PRT Mesher"
		cfg.write_option "OutputMesh" cacheName
		for i = 1 to theNode.fileList.count do
		(
			cfg.write_option ("InputPRT" + (i as String)) theNode.fileList[i]
		)
		cfg.write_option "LoadSingleFrame" theNode.loadSingleFrame
		if theNode.limitToRange do
		(
			cfg.write_option "LimitToRange" theNode.limitToRange
			cfg.write_option "RangeStartFrame" theNode.startFrame
			cfg.write_option "RangeEndFrame" theNode.endFrame
		)
		cfg.write_option "FrameOffset" theNode.frameOffset
		cfg.write_option "ParticleScaleFactor" theNode.particleScaleFactor
		cfg.write_option "FrameRate" FrameRate
	),

	fn isMeshingMethodSupported meshingMethod = (
		local isUnionOfSpheres = (meshingMethod == "Union of Spheres")
		local isMetaBalls = (meshingMethod == "MetaBalls")
		local isZhuBridson = (meshingMethod == "Zhu/Bridson")

		if isUnionOfSpheres or isMetaBalls or isZhuBridson do (
			return true
		)
		return false
	),

	fn writeFrameMeshing cfg isProxy = (
		theNode = getFrostNode ()
		-- local isVertexCloud = (meshingMethod == "Vertex Cloud")
		-- local isSpheres = (meshingMethod == "Spheres")
		-- local isImplicitSurface = not (meshingMethod == "Spheres") and not isVertexCloud
		-- local isPlanes = (meshingMethod == "Planes")
		local isUnionOfSpheres = (theNode.meshingMethod == "Union of Spheres")
		local isMetaBalls = (theNode.meshingMethod == "MetaBalls")
		local isZhuBridson = (theNode.meshingMethod == "Zhu/Bridson")

		-- if isUnionOfSpheres or isMetaBalls or isZhuBridson then
		if PRTMesherConfig.isMeshingMethodSupported theNode.meshingMethod then
		(
			if true == theNode.useFileRadii then (
				cfg.write_option "UseFileRadius" theNode.useFileRadii
			)
			else (
				cfg.write_option "MaxRadius" theNode.particleRadius
				if theNode.randomizeParticleRadius then (
					cfg.write_option "RandomizeRadius" theNode.randomizeParticleRadius
					cfg.write_option "RadiusRandomVariation" theNode.particleRadiusRandomVariation
					cfg.write_option "RadiusRandomSeed" theNode.particleRadiusRandomSeed -- supposed to be time invariant
				)
			)

			if isProxy or theNode.renderUsingViewportSettings then (
				cfg.write_option "DisableMeshing" theNode.disableViewportMeshing
				cfg.write_option "MeshingResolution" theNode.viewportMeshingResolution
				cfg.write_option "VertexRefinement" theNode.viewportVertRefinementIterations
			)
			else (
				cfg.write_option "DisableMeshing" theNode.disableRenderMeshing
				cfg.write_option "MeshingResolution" theNode.renderMeshingResolution
				cfg.write_option "VertexRefinement" theNode.renderVertRefinementIterations
			)

			if isUnionofSpheres then (
				cfg.write_option "Method" "UnionOfSpheres"
			)
			else if isMetaBalls then (
				cfg.write_option "Method" "Metaballs"
				cfg.write_option "EffectRadiusScale" theNode.metaballEffectRadius
				cfg.write_option "ImplicitThreshold" theNode.metaballImplicitThreshold
			)
			else if isZhuBridson then (
				cfg.write_option "Method" "ZhuBridson"
				cfg.write_option "EffectRadiusScale" theNode.zhuBridsonEffectRadius
				cfg.write_option "TrimmingDensity" theNode.zhuBridsonLowDensityTrimmingDensity
				cfg.write_option "TrimmingStrength" theNode.zhuBridsonLowDensityTrimmingStrength
			)
			else
			(
				-- shouldn't be here
				throw "Unsupported meshing method" theNode.meshingMethod
			)
		)
		else
		(
			-- unsupported method
			-- should use the previous 3DS MAX submission to Deadline method instead
			throw "Unsupported meshing method" theNode.meshingMethod
		)
	),

	fn writeFrame cfg f isProxy = (
		theNode = getFrostNode ()

		cfg.write_section ("Frame " + f as String)

		at time f PRTMesherConfig.writeFrameMeshing cfg isProxy

		if theNode.enablePlaybackGraph do (
			at time f (
				cfg.write_option "PlaybackTime" theNode.playbackGraphTime
			)
			local timeStep = 1f/4
			intervalStart = at time (float(f) - timeStep / 2) theNode.playbackGraphTime
			intervalEnd  = at time (float(f) + timeStep / 2) theNode.playbackGraphTime
			timeDerivative = (intervalEnd - intervalStart) / timeStep
			cfg.write_option "TimeDerivative" timeDerivative
		)
	),

	fn saveFile cfgFileName saveFileName isProxy = (
		cfgFile = openFile cfgFileName mode:"w"
		configWriter = PRTMesherConfigWriter cfgFile
		theNode = getFrostNode ()

		PRTMesherConfig.writeGlobal configWriter saveFileName isProxy

		if isProxy then
			progressStart "Saving Mesher Proxy Configuration"
		else
			progressStart "Saving Mesher Configuration"

		for i = theNode.meshSaveStart to theNode.meshSaveEnd do
		(
			if  (not progressUpdate (100.0*(i-theNode.meshSaveStart+1.0)/(theNode.meshSaveEnd - theNode.meshSaveStart + 1.0))) do
				exit

			PRTMesherConfig.writeFrame configWriter i isProxy
		)
		progressEnd()

		close cfgFile
	),

	fn getConfigFileName saveFile isProxy:false = (
		theNode = getFrostNode ()
		-- return (getFilenamePath saveFile) + (getFilenameFile saveFile) + ".cfg"
		-- return (sysInfo.tempdir) + (getFilenameFile saveFile) + ".cfg"
		if isProxy then
		(
			return theNode.FrostBase.GetTempFileNameWithPrefix "pxy"
		)
		else
		(
			return theNode.FrostBase.GetTempFileNameWithPrefix "rdr"
		)
	),

	fn saveAllFiles saveFile noProxy = (
		--   Create a configuration file corresponding to saveFile, which
		-- gives the parameters necessary for the prt mesher to create the
		-- desired output meshes.
		--   saveFile is a filename we want to save the output mesh sequence to.
		-- Note that this is the mesh sequence name, and not the config file name.
		--   noProxy specifies whether to output proxy meshes
		--   returns a list of the configuration files created while running this
		-- function.
		theNode = getFrostNode ()

		configFilenames = #()
		try
		(
			if saveFile != undefined do
			(
				if (getFilenameFile saveFile).count == 0 do (
					throw ("Frost Error: Invalid filename \"" + saveFile as String + "\"")
				)

				-- local saveConfigFileName = (getFilenamePath saveFile) + (getFilenameFile saveFile) + ".cfg"
				local saveConfigFileName = PRTMesherConfig.getConfigFileName saveFile isProxy:false

				PRTMesherConfig.saveFile saveConfigFileName saveFile false

				append configFilenames saveConfigFileName

				if (noProxy == false) do
				(
					-- set the proxy path

					saveFile = theNode.FrostBase.StripSequenceNumber saveFile

					local newPath = getFilenamePath(saveFile) + getFilenameFile(saveFile) + "_proxy"
					makeDir newPath
					saveFile = newPath + "\\" + getFilenameFile(saveFile) + "_proxy" + getFilenameType(saveFile)
					-- saveConfigFileName = (getFilenamePath saveFile) + (getFilenameFile saveFile) + ".cfg"
					saveConfigFileName = PRTMesherConfig.getConfigFileName saveFile isProxy:true

					PRTMesherConfig.saveFile saveConfigFileName saveFile true

					append configFilenames saveConfigFileName
				)
			)
		)
		catch
		(
			for f in configFilenames do
			(
				if doesFileExist f do
				(
					deleteFile f
				)
			)
			throw ()
		)
		return configFilenames
	)
)

struct FrostDeadlineStructDef (

DeadlineDetected = false,
cacheFileName = "",

fn SanityCheck FrostNode =
(
	thePath = FrostDeadlineStruct.cacheFileName

	if thePath == undefined then (
		messagebox "The save location is not defined!" title:"Flood:Particle Mesher Sanity Check"
		return false
	)

	if matchPattern thePath  pattern:"C:\\*" then (
		messagebox "The save location points to a local drive!" title:"Frost Sanity Check"
		return false
	)

	return true
),

fn GetDeadlineNetworkRoot =
(
	local theNetworkRoot = ""
	-- from DeadlineProject2005\Scripts\Submission\ClientProxy\3dsmax\Deadline-SubmitMaxToDeadline.mcr
	-- call DeadlineCommand to find the network root (assume it is in the path)
	try
	(
		local result = -2

		local submitOutputFile = sysInfo.tempdir + "submitOutput.txt"
		local submitExitCodeFile = sysInfo.tempdir + "submitExitCode.txt"

		deleteFile submitOutputFile
		deleteFile submitExitCodeFile

		local commandArguments = "-outputfiles \"" + submitOutputFile + "\" \"" + submitExitCodeFile + "\" -getrepositoryroot"
		ShellLaunch "deadlinecommandbg.exe" commandArguments

		local startTimeStamp = timestamp()
		local ready = false
		while not ready do
		(
			sleep 0.15
			if doesFileExist submitExitCodeFile do
			(
				local theFile = openFile submitExitCodeFile
				try(result = readValue theFile)catch(result = -2)
				try(close theFile)catch()
				ready = true
			)
			if timestamp() - startTimeStamp > 10000 then
			(
				result = -3
				ready = true
			)
		)
		if( result == 0 ) then
		(
			local resultFile = OpenFile submitOutputFile
			local resultMsg = ""
			if (resultFile != undefined) do
			(
				try(resultMsg = readLine resultFile)catch()
				try(close resultFile)catch()
			)

			theNetworkRoot = resultMsg
		)
		else
		(
			if result == -3 then
				messageBox "Timed out getting Repository Root from Deadline Command. (error code: 1003)"
			else
				messageBox "Failed to get Repository Root from Deadline Command. (error code: 1004)"
		)
	)
	catch
		messageBox "Error calling Deadline Command to get Repository Root. (error code: 1005)"
--	try
--	(
--		local fileHandle = openFile( "c:/deadline/deadline.ini" )
--		if fileHandle != undefined then
--		(
--			while not eof fileHandle do
--			(
--				local fileLine = readLine fileHandle
--				local index = findString fileLine "NetworkRoot="
--				if index == 1 do theNetworkRoot = (substring fileLine 13 (fileLine.count-1))
--			)
--			close fileHandle
--		)
--	) catch()
	theNetworkRoot
),

fn DetectDeadline = (
	FranticParticles.LogDebug ">Detecting Deadline Render Manager..."
	-- Run the deadline functions script to make sure we have the functions we need later.
	if SMTDSettings == undefined then (
		local deadlineRoot = FrostDeadlineStruct.GetDeadlineNetworkRoot()
		local deadlineSubmitFunctions = deadlineRoot + "\\submission\\3dsmax\\SubmitMaxToDeadline_Functions.ms"
		if deadlineRoot != "" and (doesFileExist deadlineSubmitFunctions) then (
			fileIn deadlineSubmitFunctions
			FrostDeadlineStruct.DeadlineDetected = true
		) else (
			FrostDeadlineStruct.DeadlineDetected = false
		)
	) else (
		FrostDeadlineStruct.DeadlineDetected = true
	)

	-- failure to detect Deadline is reported by the caller
),

fn jobNameFromMaxName =
(
	theNode = getFrostNode ()
	-- job naming as in FrostDeadlineSubmissionGUI.ms jobNameFromMaxName
	local theName = getFileNameFile maxFileName
	if theName == "" do theName = "Untitled 3ds Max Frost Saving Job"
	theName += " - [" + theNode.name + "]"
),

fn OpenGUI =
(
	FrostDeadlineStruct.DetectDeadline()
	if FrostDeadlineStruct.DeadlineDetected then
	(
		try(destroyDialog Frost_Deadline_GUI_Main)catch()
		rollout Frost_Deadline_GUI_Main "Submit Frost Saving Job To Deadline"
		(
			local theFrostNode
			
			group "Output Path"
			(
				button btn_getCachePath "..." across:3 height:17 width:17 align:#left
				edittext edt_savePath fieldwidth:410 across:2 align:#center offset:[-1,0]
				button btn_exploreOutputFolder "E" align:#right height:17 width:17
			)

			group "Deadline Submission Settings"
			(
				edittext edt_jobname "Job Name:" fieldwidth:395 align:#left offset:[-2,0] --across:2
				--button btn_getJobNameMenu ">>" width:17 height:17 align:#right tooltip:"Open Menu to Set the Job Name to Max File Name or to Save Path Project/Sequence/Take..."

				edittext edt_comment "Comment:" fieldwidth:395 align:#left offset:[2,0] --across:2
				--button btn_getCommentMenu ">>" width:17 height:17 align:#right tooltip:"Open Menu to Set the Comment to Max File Name or to Save Path Project/Sequence/Take..."

				edittext edt_user "Name:" fieldwidth:120 align:#left offset:[18,0] across:2
				edittext edt_dept "Department:" fieldwidth:205 align:#right

				label lbl_pools "Pool:" align:#left across:4 offset:[25,4] enabled:false
				dropdownList ddl_poollist "" width:170 align:#center offset:[-30,0] enabled:false
				label lbl_groups "Group:" align:#center offset:[-24,4] enabled:false
				dropdownList ddl_grouplist "" width:170 align:#right enabled:false

				label lbl_priority "Priority:" align:#left offset:[15,2] across:3
				progressbar sld_priority width:345 height:18 range:[0,100,50] type:#integer align:#center offset:[2,0]
				spinner spn_priority "" type:#integer fieldwidth:35 align:#right  offset:[1,1]

				spinner spn_machineLimit "Number of Machines Working Concurrently: " range:[0,1000,1] type:#integer align:#left  offset:[0,2]  fieldwidth:35 across:2 enabled:false
				spinner spn_chunkSize "Frames Per Task: " range:[1,1000,1] type:#integer  align:#right  offset:[0,2]  fieldwidth:35  enabled:false --Render Task Chunk Size
			)


			button btn_submit "SELECT A VALID PATH TO SUBMIT TO DEADLINE..." height:30 width:467 enabled:false

			fn job_priority_update val =
			(
				if val <= 100 do
				(
					theRed = (255.0 - 255.0*val/100.0)*2.0
					if theRed > 255 do theRed = 255
					theGreen = 512.0*val/100.0
					if theGreen > 255 do theGreen = 255
					sld_priority.color = [theRed, theGreen, 0]
					sld_priority.value = spn_priority.value = val
				)
				val
			)

			fn jobNameFromMaxName =
			(
				local theName = getFileNameFile maxFileName
				if theName == "" do theName = "Untitled 3ds Max Frost Saving Job"
				theName += " - [" + theFrostNode.name + "]"
				SMTDSettings.jobName = edt_jobname.text = theName
			)
			
			fn canSubmitCheck =
			(
				local expectedFilenameExt = if theFrostNode.saveMode == "LevelSet" then ".rls" else ".xmesh"
				local accept = ((getFileNameType edt_savePath.text) == expectedFilenameExt) and (doesFileExist (getFileNamePath edt_savePath.text))
				btn_submit.enabled = accept
				btn_submit.caption = if accept then
					"SUBMIT FROST SAVING JOB TO DEADLINE"
				else
					"SELECT A VALID PATH TO SUBMIT TO DEADLINE..."
			)

			on btn_getCachePath pressed do
			(
				local theNode = theFrostNode
				local thePath

				local startFilename
				if edt_savePath.text.count > 0 then (
					startFilename = edt_savePath.text
				)
				else (
					local startFilename = if theNode.fileList.count > 0 then theNode.fileList[1] else undefined
					local startFilenameExt = if theNode.saveMode == "LevelSet" then ".rls" else ".xmesh"
					if startFilename == undefined then
						startFilename = ""
					else
						startFilename = (getFilenamePath startFilename) + (getFilenameFile startFilename) + startFilenameExt
				)

				if( theNode.saveMode == "LevelSet") then
					thePath = getSaveFileName caption:"Select Save Path:" filename:startFilename types:"RLE Level Set (*.rls)|*.rls|"
				else
					thePath = getSaveFileName caption:"Select Save Path:" filename:startFilename types:"XMesh File (*.xmesh)|*.xmesh|"

				if thePath != undefined do
				(
					edt_savePath.text = FrostDeadlineStruct.cacheFileName = thePath
				)

				canSubmitCheck()
			)
			
			on edt_savePath changed val do
			(
				FrostDeadlineStruct.cacheFileName = val
				canSubmitCheck()
			)

			on btn_exploreOutputFolder pressed do 
			(
				local thePath = (getFileNamePath edt_savePath.text)
				if doesFileExist thePath do shellLaunch "explorer.exe" thePath
			)		


			on btn_selectNode pressed do select theFrostNode[ddl_FrostNodes.selection]

			on ddl_FrostNodes selected itm do jobNameFromMaxName()
			on edt_jobname entered txt do SMTDSettings.jobName = txt
			on edt_comment entered txt do SMTDSettings.comment = txt

			on spn_priority changed value do
			(
				job_priority_update value
				setIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "Priority" (value as string)
			)

			on sld_priority clicked value do
			(
				job_priority_update value
				setIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "Priority" (value as string)
			)
			on ddl_poollist selected itm do
			(
				setIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "PoolName" (ddl_poollist.selected)
			)

			on ddl_grouplist selected itm do
			(
				setIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "group" (ddl_grouplist.selected)
			)

			on spn_machineLimit changed value do
			(
				setIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "MachineLimit" (value as string)
			)

			on spn_chunkSize changed value do
			(
				setIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "ChunkSize" (value as string)
			)

			on btn_submit pressed do
			(
				-- TODO: Add error checking
				local theNode = theFrostNode
				if isValidNode theNode and FrostDeadlineStructDef.SanityCheck theNode then
				(
					-- Copy the dialogue control values into the SMTDSettings.
					-- jobName is set again later, because it changes for 
					-- the proxy.
					SMTDSettings.jobName = edt_jobname.text
					SMTDSettings.comment = edt_comment.text
					SMTDSettings.userName = edt_user.text
					SMTDSettings.department = edt_dept.text
					SMTDSettings.poolName = ddl_poollist.selected
					SMTDSettings.group = ddl_grouplist.selected
					SMTDSettings.priority = spn_priority.value
					SMTDSettings.machineLimit  = spn_machineLimit.value
					SMTDSettings.chunkSize = spn_chunkSize.value

					oldMXSJob = SMTDSettings.SubmitAsMXSJob
					oldWSMode = SMTDSettings.ForceWorkstationMode

					SMTDSettings.SubmitAsMXSJob = true
					SMTDSettings.ForceWorkstationMode = false

					SMTDSettings.SequentialJob = true
					SMTDSettings.LimitEnabled = SMTDSettings.MachineLimit > 0

					local renderPath = FrostDeadlineStruct.cacheFileName
					renderPath = theNode.FrostBase.StripSequenceNumber renderPath
					
					-- use the Python PRT Mesher if it supports this meshing method
					if( theNode.saveMode == "Mesh" and ( PRTMesherConfig.isMeshingMethodSupported theNode.meshingMethod ) ) then (
						configFilenames = #()
						try
						(
							local saveFile = FrostDeadlineStruct.cacheFileName

							configFilenames = PRTMesherConfig.saveAllFiles saveFile theNode.noProxy

							local frameSequence = ((theNode.meshSaveStart as string) + "-" + (theNode.meshSaveEnd as string))

							if configFilenames.count < 1 or configFilenames.count > 2 then (
								throw "Frost Internal Error: wrote " + (configFilenames.count as String) + " config files, when we expect only one or two."
							)

							local proxy = not theNode.noProxy
							if proxy and configFilenames.count < 2 then (
								throw "Frost Internal Error: need 2 config files when proxy is enabled, but only wrote " + (configFilenames.count as String) + "."
							)

							SMTDFunctions.CreateSubmitInfoFile SMTDPaths.SubmitInfofile customPlugin:"PythonScript" customFrameSequence:frameSequence customOutputFile:renderPath

							local renderConfigFilename = configFilenames[1]

							SMTDFunctions.CreateJobInfoFile SMTDPaths.JobInfofile
							local theJobInfofile = openFile SMTDPaths.JobInfofile mode:"wt" -- clear the max job info and start from fresh
							format "Script=%\n" prtMesherWorkerCommand to:theJobInfofile
							format "Arguments=%\n" ("<FILE0> <STARTFRAME> <ENDFRAME>") to:theJobInfofile
							format "CopyScriptDirectory=true\n" to:theJobInfofile
							close theJobInfoFile

							local initialArgs = " \"" + SMTDPaths.SubmitInfofile + "\" \"" + SMTDPaths.JobInfofile + "\" \"" + renderConfigFilename + "\""

							local retCode = SMTDFunctions.waitForCommandToComplete initialArgs 3600

							SMTD_LastMessage = SMTDFunctions.getRenderMessage()
							if retCode != #success then
							(
								throw SMTD_LastMessage
							)
							else
							(
								messageBox SMTD_LastMessage title:"Frost Job Submission to Deadline SUCCESSFUL!"
							)

							if proxy do
							(
								local proxyConfigFilename = configFilenames[2]
								SMTDSettings.jobName = (edt_jobname.text + " Proxy")
								local proxyPath = getFilenamePath(renderPath) + getFilenameFile(renderPath) + "_proxy"
								proxyPath = proxyPath + "\\" + getFilenameFile(renderPath) + "_proxy" + getFilenameType(renderPath)
								
								SMTDFunctions.CreateSubmitInfoFile SMTDPaths.SubmitInfofile customPlugin:"PythonScript" customFrameSequence:frameSequence  customOutputFile:proxyPath

								-- Should already have theJobInfo file from before.
								-- They are the same for both the proxy and the render.

								local initialArgs = " \"" + SMTDPaths.SubmitInfofile + "\" \"" + SMTDPaths.JobInfofile + "\" \"" + proxyConfigFilename + "\""

								local retCode = SMTDFunctions.waitForCommandToComplete initialArgs 3600

								SMTD_LastMessage = SMTDFunctions.getRenderMessage()
								if retCode != #success then
								(
									throw SMTD_LastMessage
								)
								else
								(
									messageBox SMTD_LastMessage title:"Frost Job Submission to Deadline SUCCESSFUL!"
								)
							)
						)
						catch
						(
							messageBox (getCurrentException() as String) title:"ERROR Submitting Frost Job to Deadline!"
						)
						for f in configFilenames do
						(
							if doesFileExist f do
							(
								deleteFile f
							)
						)
					)
					else (
						if( theNode.saveMode == "LevelSet") then
							SMTDFunctions.CreateSubmitInfoFile SMTDPaths.SubmitInfofile customFrameSequence:((theNode.savingRollout.spn_levelSetSaveStart.value as string) + "-" + (theNode.savingRollout.spn_levelSetSaveEnd.value as string)) customOutputFile:renderPath
						else
							SMTDFunctions.CreateSubmitInfoFile SMTDPaths.SubmitInfofile customFrameSequence:((theNode.savingRollout.spn_meshSaveStart.value as string) + "-" + (theNode.savingRollout.spn_meshSaveEnd.value as string)) customOutputFile:renderPath

						SMTDFunctions.CreateJobInfoFile SMTDPaths.JobInfofile renderOutputOverride:FrostDeadlineStruct.cacheFileName

						local TempMaxFile = SMTDPaths.TempDir + maxFileName
						if maxFileName == "" do TempMaxFile += "untitled.max"
						if (doesFileExist TempMaxFile) do deleteFile TempMaxFile
						SMTDFunctions.SaveMaxFileCopy TempMaxFile

						local theJobInfofile = openFile SMTDPaths.JobInfofile mode:"at"
						format "FrostNode=%\n" (theNode.name) to:theJobInfofile
							format "CacheFileName=%\n" FrostDeadlineStruct.cacheFileName to:theJobInfofile
						if( theNode.saveMode == "LevelSet") do
							format "LevelSetCache=%\n" 1 to:theJobInfofile
						close theJobInfofile

						local initialArgs = " \"" + SMTDPaths.SubmitInfofile + "\" \"" + SMTDPaths.JobInfofile  + "\" \"" + TempMaxFile  + "\" \""
						initialArgs += FrostUtils.FrostHome + "Scripts\\FrostCachingDeadlineScriptJob.ms\""
						format "%\n" initialArgs
						local retCode = SMTDFunctions.waitForCommandToComplete initialArgs 3600

						SMTD_LastMessage = SMTDFunctions.getRenderMessage()
						if retCode != #success then
						(
							messageBox SMTD_LastMessage title:"ERROR Submitting Frost Job to Deadline!"
						)
						else
						(
							messageBox SMTD_LastMessage title:"Frost Job Submission to Deadline SUCCESSFUL!"
						)

						if ( (not theNode.saveMode == "LevelSet") and (not theNode.savingRollout.chk_noProxy.checked) ) do
						(
							SMTDSettings.jobName = (edt_jobname.text + " Proxy")
							local proxyPath = getFilenamePath(renderPath) + getFilenameFile(renderPath) + "_proxy"
							proxyPath = proxyPath + "\\" + getFilenameFile(renderPath) + "_proxy" + getFilenameType(renderPath)
							SMTDFunctions.CreateSubmitInfoFile SMTDPaths.SubmitInfofile customFrameSequence:((theNode.savingRollout.spn_meshSaveStart.value as string) + "-" + (theNode.savingRollout.spn_meshSaveEnd.value as string))  customOutputFile:proxyPath
							local theJobInfofile = openFile SMTDPaths.JobInfofile mode:"at"
							format "ProxyMode=true\n" to:theJobInfofile
							close theJobInfofile
							retCode = SMTDFunctions.waitForCommandToComplete initialArgs 3600
							SMTD_LastMessage = SMTDFunctions.getRenderMessage()
							if retCode != #success then
							(
								messageBox SMTD_LastMessage title:"ERROR Submitting Frost Proxy Saving Job to Deadline!"
							)
							else
							(
								messageBox SMTD_LastMessage title:"Frost Proxy Saving Job Submission to Deadline SUCCESSFUL!"
							)
						)
					)
					SMTDSettings.SubmitAsMXSJob =oldMXSJob
					SMTDSettings.ForceWorkstationMode = oldWSMode
				)
				else
					messageBox "Invalid Node"

			)

			fn displayPoolList =
			(
				-- SMTDFunctions.CollectPools() returns a "" as the first pool
				-- If we leave it in, then we need to make sure that it's not 
				-- selected later on.
				-- Unlike the SMTD rollout, we will filter it out now.
				--ddl_poollist.items = SMTDSettings.Pools
				
				poolList = #()
				for i in SMTDSettings.Pools do 
					if i.count > 0 do 
						append poolList i
				if poolList.count == 0 do
					append poolList "none"
				
				ddl_poollist.items = poolList
					
				local theIndex = findItem ddl_poollist.items SMTDSettings.PoolName
				if theIndex > 0 then 
					ddl_poollist.selection = theIndex 
				else	
					ddl_poollist.selection = 1

				lbl_pools.enabled = ddl_poollist.enabled = true
			)
			
			fn displayGroupList =
			(
				--ddl_grouplist.items = SMTDSettings.Groups

				groupList = #()
				for i in SMTDSettings.Groups do 
					if i.count > 0 do 
						append groupList i
				if groupList.count == 0 do
					append groupList "none"
				
				ddl_grouplist.items = groupList
				
				local theIndex = findItem ddl_grouplist.items SMTDSettings.Group
				if theIndex > 0 then 
					ddl_grouplist.selection = theIndex 
				else	
					ddl_grouplist.selection = 1

				lbl_groups.enabled = ddl_grouplist.enabled = true
			)

			on Frost_Deadline_GUI_Main open do
			(
				makeDir (GetDir #plugcfg + "\\Frost") all:true
				SMTDFunctions.loadSettings()

				if ((maxVersion())[1] > 8000) do
				(
					if is64bitApplication() then
						SMTDSettings.MaxVersionToForce = "64bit"
					else
						SMTDSettings.MaxVersionToForce = "32bit"
				)

				theVal = execute (getIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "Priority")
				if theVal != OK do SMTDSettings.Priority= theVal
				SMTDSettings.PoolName= (getIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "PoolName")
				SMTDSettings.group= (getIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "group")
				theVal = execute (getIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "MachineLimit")
				if theVal != OK do SMTDSettings.MachineLimit  = theVal
				theVal = execute (getIniSetting (GetDir #plugcfg + "\\Frost\\DeadlineSubmissionSettings.ini") "Deadline" "ChunkSize")
				if theVal != OK do SMTDSettings.ChunkSize = theVal

				if SMTDSettings.Pools.count == 0 do
					SMTDFunctions.collectPools()
				displayPoolList()

				if SMTDSettings.Groups.count == 0 do
					SMTDFunctions.collectGroups()
				displayGroupList()

				theFrostNode = selection[1]
				btn_getCachePath.enabled = edt_savePath.enabled = spn_machineLimit.enabled = spn_chunkSize.enabled = true

				jobNameFromMaxName()
				edt_user.text = SMTDSettings.UserName
				edt_dept.text = SMTDSettings.Department
				spn_machineLimit.value = SMTDSettings.machineLimit
				spn_chunkSize.value = SMTDSettings.chunkSize
				job_priority_update SMTDSettings.Priority
	
				canSubmitCheck()
			)
		)
		createDialog Frost_Deadline_GUI_Main 475 255
	)
	else
		messagebox "Deadline Was Not Detected On This Computer!" title:"Frost Deadline Submission"
)

)

global FrostDeadlineStruct = FrostDeadlineStructDef()
)