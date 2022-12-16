-- Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
-- SPDX-License-Identifier: Apache-2.0
-- If Frost binary is installed
if classof Frost == GeometryClass then (
	
	local scriptPath = FrostUtils.FrostHome + "Scripts\\"
	local iconPath = FrostUtils.FrostHome + "Icons\\"

	fileIn (scriptPath + "FrostUI.ms")
	fileIn (scriptPath + "FrostPresetMan.ms")
	fileIn (scriptPath + "Frost-CreateFrost.mcr")
	fileIn (scriptPath + "FrostROIManipulator.ms")

	if not (IsNetServer()) do --if running in workstation mode,
	(
		local theFilesToCopy = #("Frost_24i.bmp", "Frost_24a.bmp", "Frost_16i.bmp", "Frost_16a.bmp")
		local updatedIcons = 0
		for i in theFilesToCopy do
		(
			local theTarget = (getDir #usericons + "\\" + i)
			local theSource = (iconPath + i)
			if (doesFileExist theSource and doesFileExist theTarget and getFileModDate theTarget != getFileModDate theSource) or (not doesFileExist theTarget) then
			(
				deleteFile theTarget  --delete the old file (MAXScript does not support direct overwriting)
				local result = copyFile theSource theTarget  --copy the new file to the Max Icons folder
				if result do updatedIcons += 1
			)
		)
		if updatedIcons > 0 do
		(
			try(colorman.reiniticons())catch()
		)
		callbacks.addScript #postSystemStartup "fileIn (FrostUtils.FrostHome+\"/Scripts/FrostMenuCreator.ms\")" id:#froststartup
	)
) else (
	if not (IsNetServer()) do
	(
		messageBox "The Frost plugin is not installed correctly.\n\nFrost is not available.\n\nPlease reinstall." title:"Frost"
	)
)
