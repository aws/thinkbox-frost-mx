-- Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
-- SPDX-License-Identifier: Apache-2.0
(
	local du = DeadlineUtil  --this is the interface exposed by the Lightning Plug-in which provides communication between Deadline and 3ds Max
	if du == undefined do  --if the script is not being run on Deadline (for testing purposes),
	(
		struct DeadlineUtilStruct   --define a stand-in struct with the same methods as the Lightning plug-in
		(
			fn SetTitle title = ( format "Title: %\n" title ),
			fn SetProgress percent = (true),
			fn FailRender msg = ( throw msg ),
			fn GetJobInfoEntry key = ( undefined ),
			fn GetAuxFilename index = ( undefined ),
			fn LogMessage msg = ( format "%\n" msg ),
			CurrentFrame = ((sliderTime as string) as integer)
		)
		du = DeadlineUtilStruct() --create an instance of the stand-in struct
	)--end if
	
	du.SetTitle "MAXScript Job" --set the job title 
	du.LogMessage "Starting MAXScript Job..." --output a message to the log
	local st = timestamp() --get the current system time
	
	saveFile = du.GetJobInfoEntry "CacheFileName"
	proxyMode = du.GetJobInfoEntry "ProxyMode"
    	levelSetCache = du.GetJobInfoEntry "LevelSetCache"
	prt_mesher = getNodeByName (du.GetJobInfoEntry "FrostNode")
	if isValidNode prt_mesher then
	(
        	-- if it's a level set caching job
		if (levelSetCache != undefined) then
		(
			prt_mesher.delegate.SetCacheName saveFile
			du.LogMessage ("Saving Level Set for Frame " + du.CurrentFrame as string + "...")
			if ( prt_mesher.transformNode == undefined ) then
				customTransform = (matrix3 1)
			else
				customTransform = prt_mesher.transformNode.transform
					
			prt_mesher.delegate.SaveLevelSetToSequence du.CurrentFrame (inverse customTransform)
			
		)
		-- If the proxy flag wasn't defined, save at the given location
		else if (proxyMode == undefined) then
		(
			prt_mesher.delegate.SetCacheName saveFile
			du.LogMessage ("Saving Frame " + du.CurrentFrame as string + "...")
			prt_mesher.delegate.SaveMeshToSequence du.CurrentFrame true
		)
		-- Otherwise, save at a proxy location
		else
		(
			-- set the proxy path
			saveFile = prt_mesher.delegate.StripSequenceNumber saveFile
			newPath = getFilenamePath(saveFile) + getFilenameFile(saveFile) + "_proxy"
			makeDir newPath 
			saveFile = newPath + "\\" + getFilenameFile(saveFile) + "_proxy" + getFilenameType(saveFile)
			
			prt_mesher.delegate.SetCacheName saveFile
			du.LogMessage ("Saving Proxy Frame " + du.CurrentFrame as string + "...")
			prt_mesher.delegate.SaveMeshToSequence du.CurrentFrame false
		)

	)
	else
		du.FailRender ("Object " + du.GetJobInfoEntry "MesherName" + " not found. Aborting....")
	
	du.LogMessage ("Finished MAXScript Job in "+ ((timestamp() - st)/1000.0) as string + " sec.") --output the job duration
	true  --return true if the task has finished successfully, return false to fail the task.
)--end script
