-- Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
-- SPDX-License-Identifier: Apache-2.0
struct Frost_PresetsOptions_Item ( label, name, property, save, isString=false )

global Frost_PresetsDirectory = FrostUtils.GetSettingsDirectory()
global Frost_PresetsOptions 
global Frost_PresetsOptions_Dialog
global Frost_PresetsOptions_Filename = ""

global Frost_ConvertToPRT_InFilename
global Frost_ConvertToPRT_LockInFilename
global Frost_ConvertToPRT_OutFilename
global Frost_ConvertToPRT_Dialog

global Frost_RadiusScaleAnimationTools_Value = undefined
global Frost_RadiusScaleAnimationTools_Prompt = undefined

struct FrostUi (
	fn loadSettingFromPresetFile theFrostNode thePresetFile theName theProperty isString:false =
	(
		if isString then (
			theVal = getIniSetting thePresetFile "Settings" theName
			try(setProperty theFrostNode theProperty theVal)catch()
		) else (
			theVal = execute (getIniSetting thePresetFile "Settings" theName )
			if theVal != OK do 
			(
				try(setProperty theFrostNode theProperty theVal)catch()
			)
		)
	),

	fn getUsePresets =
	(
		theVal = execute ( getIniSetting ( FrostUtils.GetSettingsFilename() ) "Presets" "UsePreset" )
		return ( theVal == true )
	),

	fn getPresetName =
	(
		return getIniSetting ( FrostUtils.GetSettingsFilename() ) "Presets" "PresetName"
	),

	fn getPresetFilename presetName = 
	(
		return ( FrostUtils.GetSettingsDirectory() + "\\presets\\" + presetName + ".ini" )
	),

	fn resetFrostPresetsOptions = (
		Frost_PresetsOptions = #( Frost_PresetsOptions_Item "Show Icon" "showIcon" #showIcon true,
		Frost_PresetsOptions_Item "Icon Size" "iconSize" #iconSize false,
		Frost_PresetsOptions_Item "Enable Velocity to Map Channel" "writeVelocityMapChannel" #writeVelocityMapChannel true,
		Frost_PresetsOptions_Item "Velocity to Map Channel" "velocityMapChannel" #velocityMapChannel true,
		Frost_PresetsOptions_Item "Update on Frost Change" "updateOnFrostChange" #updateOnFrostChange true,
		Frost_PresetsOptions_Item "Update on Particle Change" "updateOnParticleChange" #updateOnParticleChange true,
		Frost_PresetsOptions_Item "Particle Flow Events - Selection Mode" "pfEventFilterMode" #pfEventFilterMode true,
		Frost_PresetsOptions_Item "Thinking Particles Groups - Selection Mode" "tpGroupFilterMode" #tpGroupFilterMode true,
		Frost_PresetsOptions_Item "File - Before Range Behavior" "beforeRangeBehavior" #beforeRangeBehavior true,
		Frost_PresetsOptions_Item "File - After Range Behavior" "afterRangeBehavior" #afterRangeBehavior true,
		Frost_PresetsOptions_Item "File - Unit" "fileLengthUnit" #fileLengthUnit true,
		Frost_PresetsOptions_Item "File - Custom Scale" "fileCustomScale" #fileCustomScale true,
		Frost_PresetsOptions_Item "Material - Material Mode" "materialMode" #materialMode true,
		Frost_PresetsOptions_Item "Meshing - Meshing Method" "meshingMethod" #meshingMethod true,
		Frost_PresetsOptions_Item "Meshing - Enable Render Mesh" "enableRenderMesh" #enableRenderMesh true,
		Frost_PresetsOptions_Item "Meshing - Enable Viewport Mesh" "enableViewportMesh" #enableViewportMesh true,
		Frost_PresetsOptions_Item "Meshing - View Render Particles" "viewRenderParticles" #viewRenderParticles true,
		Frost_PresetsOptions_Item "Meshing - Viewport Load Mode" "viewportLoadMode" #viewportLoadMode true,
		Frost_PresetsOptions_Item "Meshing - Percent in Viewport" "viewportLoadPercent" #viewportLoadPercent true,
		Frost_PresetsOptions_Item "Meshing - Radius" "radius" #radius true,
		Frost_PresetsOptions_Item "Meshing - Use Radius Channel" "useRadiusChannel" #useRadiusChannel true,
		Frost_PresetsOptions_Item "Meshing - Randomize Radius" "randomizeRadius" #randomizeRadius true,
		Frost_PresetsOptions_Item "Meshing - Radius Random Variation" "radiusRandomVariation" #radiusRandomVariation true,
		Frost_PresetsOptions_Item "Meshing - Radius Random Seed" "radiusRandomSeed" #radiusRandomSeed true,
		Frost_PresetsOptions_Item "Meshing - Enable Radius Scale" "enableRadiusScale" #enableRadiusScale true,
		Frost_PresetsOptions_Item "Meshing - Radius Scale" "radiusScale" #radiusScale true,
		Frost_PresetsOptions_Item "Meshing - Radius Scale Sync By" "radiusAnimationMode" #radiusAnimationMode true,
		Frost_PresetsOptions_Item "Meshing - Motion Blur Mode" "motionBlurMode" #motionBlurMode true,
		Frost_PresetsOptions_Item "Meshing Quality - Resolution Mode" "meshingResolutionMode" #meshingResolutionMode true,
		Frost_PresetsOptions_Item "Meshing Quality - Render Relative to Max Radius" "renderMeshingResolution" #renderMeshingResolution true,
		Frost_PresetsOptions_Item "Meshing Quality - Viewport Relative to Max Radius" "viewportMeshingResolution" #viewportMeshingResolution true,
		Frost_PresetsOptions_Item "Meshing Quality - Render Absolute Spacing" "renderVoxelLength" #renderVoxelLength true,
		Frost_PresetsOptions_Item "Meshing Quality - Viewport Absolute Spacing" "viewportVoxelLength" #viewportVoxelLength true,
		Frost_PresetsOptions_Item "Meshing Quality - Render Vert Refine" "renderVertRefinementIterations" #renderVertRefinementIterations true,
		Frost_PresetsOptions_Item "Meshing Quality - Viewport Vert Refine" "viewportVertRefinementIterations" #viewportVertRefinementIterations true,
		Frost_PresetsOptions_Item "Meshing Quality - Render Using Viewport Quality" "renderUsingViewportSettings" #renderUsingViewportSettings true,
		Frost_PresetsOptions_Item "Metaballs - Radius Scale" "metaballRadiusScale" #metaballRadiusScale true,
		Frost_PresetsOptions_Item "Metaballs - Surface Level" "metaballIsosurfaceLevel" #metaballIsosurfaceLevel true,
		Frost_PresetsOptions_Item "Zhu/Bridson - Blend Radius" "zhuBridsonBlendRadiusScale" #zhuBridsonBlendRadiusScale true,
		Frost_PresetsOptions_Item "Zhu/Bridson - Enable Low Density Trimming" "zhuBridsonEnableLowDensityTrimming" #zhuBridsonEnableLowDensityTrimming true,
		Frost_PresetsOptions_Item "Zhu/Bridson - Low Density Trimming Threshold" "zhuBridsonLowDensityTrimmingThreshold" #zhuBridsonLowDensityTrimmingThreshold true,
		Frost_PresetsOptions_Item "Zhu/Bridson - Low Density Trimming Strength" "zhuBridsonLowDensityTrimmingStrength" #zhuBridsonLowDensityTrimmingStrength true,
		Frost_PresetsOptions_Item "Anisotropic - Radius Scale" "anisotropicRadiusScale" #anisotropicRadiusScale true,
		Frost_PresetsOptions_Item "Anisotropic - Surface Level" "anisotropicIsosurfaceLevel" #anisotropicIsosurfaceLevel true,
		Frost_PresetsOptions_Item "Anisotropic - Max Stretch" "anisotropicMaxAnisotropy" #anisotropicMaxAnisotropy true,
		Frost_PresetsOptions_Item "Anisotropic - Min Neighbors for Stretching" "anisotropicMinNeighborCount" #anisotropicMinNeighborCount true,
		Frost_PresetsOptions_Item "Anisotropic - Smoothing Weight" "anisotropicPositionSmoothingWeight" #anisotropicPositionSmoothingWeight true,
		Frost_PresetsOptions_Item "Particle Geometry - Geometry Type" "geometryType" #geometryType true,
		Frost_PresetsOptions_Item "Particle Geometry - Use V-Ray Instancing" "useRenderInstancing" #useRenderInstancing true,
		Frost_PresetsOptions_Item "Particle Geometry - Geometry Selection Mode" "geometrySelectionMode" #geometrySelectionMode true,
		Frost_PresetsOptions_Item "Particle Geometry - Geometry Selection Random Seed" "geometrySelectionSeed" #geometrySelectionSeed true,
		Frost_PresetsOptions_Item "Particle Geometry - Animation Timing Base Mode" "geometrySampleTimeBaseMode" #geometrySampleTimeBaseMode true,
		Frost_PresetsOptions_Item "Particle Geometry - Animation Timing Offset Mode" "geometrySampleTimeOffsetMode" #geometrySampleTimeOffsetMode true,
		Frost_PresetsOptions_Item "Particle Geometry - Animation Timing Random Offset" "geometrySampleTimeMaxRandomOffset" #geometrySampleTimeMaxRandomOffset true,
		Frost_PresetsOptions_Item "Particle Geometry - Animation Timing Random Seed" "geometrySampleTimeSeed" #geometrySampleTimeSeed true,
		Frost_PresetsOptions_Item "Particle Geometry - Orientation Mode" "geometryOrientationMode" #geometryOrientationMode true,
		Frost_PresetsOptions_Item "Particle Geometry - Orientation Vector Channel" "geometryOrientationVectorChannel" #geometryOrientationVectorChannel true isString:true,
		Frost_PresetsOptions_Item "Particle Geometry - Orientation" #( "geometryOrientationX", "geometryOrientationY", "geometryOrientationZ" ) #( #geometryOrientationX, #geometryOrientationY, #geometryOrientationZ ) true,
		Frost_PresetsOptions_Item "Particle Geometry - Divergence" "geometryOrientationDivergence" #geometryOrientationDivergence true,
		Frost_PresetsOptions_Item "Particle Geometry - Restrict Divergence Axis" "geometryOrientationRestrictDivergenceAxis" #geometryOrientationRestrictDivergenceAxis true,
		Frost_PresetsOptions_Item "Particle Geometry - Divergence Axis Space" "geometryOrientationDivergenceAxisSpace" #geometryOrientationDivergenceAxisSpace true,
		Frost_PresetsOptions_Item "Particle Geometry - Divergence Axis" #( "geometryOrientationDivergenceAxisX", "geometryOrientationDivergenceAxisY", "geometryOrientationDivergenceAxisZ" ) #( #geometryOrientationDivergenceAxisX, #geometryOrientationDivergenceAxisY, #geometryOrientationDivergenceAxisZ ) true )
	),

	fn loadPreset frostNode thePresetFile =
	(
		FrostUI.resetFrostPresetsOptions()
		for i = 1 to Frost_PresetsOptions.count do
		(
			if classof Frost_PresetsOptions[i].name == Array then
			(
				for j = 1 to Frost_PresetsOptions[i].name.count do
				(
					FrostUi.loadSettingFromPresetFile frostNode thePresetFile Frost_PresetsOptions[i].name[j] Frost_PresetsOptions[i].property[j] isString:(Frost_PresetsOptions[i].isString)
				)
			)
			else
			(
				FrostUi.loadSettingFromPresetFile frostNode thePresetFile Frost_PresetsOptions[i].name Frost_PresetsOptions[i].property isString:(Frost_PresetsOptions[i].isString)
			)
		)
	),

	fn getPresetNameList =
	(
		theFiles = getFiles ( FrostUtils.GetSettingsDirectory() + "\\presets\\*.ini" )
		items = for f in theFiles collect getFileNameFile f
		return items
	),
	
	fn on_LogWindowOptions_pressed frostNode =
	(
		rcmenu Frost_LogWindowOptions_RCMenu
		(
			menuitem mnu_popupLogWindowOnError "Show Log Window on ERRORS" checked:(FrostUtils.PopupLogWindowOnError)
			on mnu_popupLogWindowOnError picked do
			(
				FrostUtils.PopupLogWindowOnError = not FrostUtils.PopupLogWindowOnError
				setIniSetting ( FrostUtils.GetSettingsFilename() ) "Logging" "PopupLogWindowOnError" (FrostUtils.PopupLogWindowOnError as String)
			)
		)
		popUpMenu Frost_LogWindowOptions_RCMenu pos:mouse.screenpos
	),

	fn on_savePreset_pressed frostNode =
	(
		FrostUI.resetFrostPresetsOptions()
		Frost_PresetsOptions_Filename = ""
		createDialog Frost_PresetsOptions_Dialog 320 690 modal:true
		if Frost_PresetsOptions_Filename != "" then
		(
			makeDir ( FrostUtils.GetSettingsDirectory() + "\\presets\\" ) all:true
			thePresetFile = ( FrostUtils.GetSettingsDirectory() + "\\presets\\" + Frost_PresetsOptions_Filename + ".ini" )
			deleteFile thePresetFile 
			
			for i = 1 to Frost_PresetsOptions.count do
			(
				if Frost_PresetsOptions[i].save do
				(
					if classof Frost_PresetsOptions[i].name == Array then
					(
						for j = 1 to Frost_PresetsOptions[i].name.count do
						(
							setIniSetting thePresetFile "Settings" Frost_PresetsOptions[i].name[j] ( ( getProperty frostNode Frost_PresetsOptions[i].property[j] ) as string )
						)
					)
					else
					(
						setIniSetting thePresetFile "Settings" Frost_PresetsOptions[i].name ( ( getProperty frostNode Frost_PresetsOptions[i].property ) as string )
					)
				)
			)
			return Frost_PresetsOptions_Filename
		)
		return ""
	),

	fn on_setDefaultPreset_pressed frostNode presetName =
	(
		if presetName == "" then
		(
			if (querybox ("Do you really want to DISABLE Defaults Loading for newly created Frost objects?") title:"Frost : No Preset") do
			(
				makeDir ( FrostUtils.GetSettingsDirectory() ) all:true
				setIniSetting ( FrostUtils.GetSettingsFilename() ) "Presets" "PresetName" ""
				setIniSetting ( FrostUtils.GetSettingsFilename() ) "Presets" "UsePreset" "false"
				return true
			)
		)
		else
		(
			thePresetFile = ( FrostUtils.GetSettingsDirectory() + "\\presets\\" + presetName + ".ini") 
			if doesFileExist thePresetFile and (querybox ("Do you really want to SET the Preset File [" + presetName +"] as Default for newly created Frost objects?") title:"Frost : Default Preset") do
			(
				makeDir ( FrostUtils.GetSettingsDirectory() ) all:true
				setIniSetting ( FrostUtils.GetSettingsFilename() ) "Presets" "PresetName" presetName
				setIniSetting ( FrostUtils.GetSettingsFilename() ) "Presets" "UsePreset" "true"
				return true
			)
		)
		return false
	),

	fn on_deletePreset_pressed frostNode presetName =
	(
		thePresetFile = ( FrostUtils.GetSettingsDirectory() + "\\presets\\" + presetName + ".ini") 
		if doesFileExist thePresetFile and (querybox ("Do you really want to delete the Preset File [" + presetName +"]?") title:"Frost : Delete Preset") do
		(
			deleteFile thePresetFile 
			return true
		)
		return false
	),

	fn on_explorePresets_pressed =
	(
		makeDir ( FrostUtils.GetSettingsDirectory() + "\\presets\\" ) all:true
		shellLaunch "explorer.exe" ( FrostUtils.GetSettingsDirectory() + "\\presets\\" )
	),

	fn on_NodeListOptions_pressed frostNode =
	(
		rcmenu Frost_NodeListOptions_RCMenu
		(
			menuitem mnu_selectHighlighted "SELECT Highlighted In the Scene" enabled:(((modPanel.getCurrentObject()).GetNodeListSelection()).count > 0)
			separator sep_100
			menuitem mnu_removeDeletedObjects "REMOVE DELETED Objects"
			separator sep_150
			submenu "HIGHLIGHT Object In The List"
			(
				menuItem mnu_highlightAll "All"
				menuItem mnu_highlightInvert "Invert"
				separator sep_160
				menuItem mnu_highlightSimilar "Similar" enabled:(((modPanel.getCurrentObject()).GetNodeListSelection()).count == 1)
			)
			separator sep_200
			submenu "MOVE Highlighted Objects"
			(
				menuitem mnu_moveSelectedToTop "To Top" enabled:(((modPanel.getCurrentObject()).GetNodeListSelection()).count > 0)
				menuitem mnu_moveSelectedUp "One Object Up" enabled:( theSel = ((modPanel.getCurrentObject()).GetNodeListSelection()); theSel.count == 1 and theSel[1] != 1)
				menuitem mnu_moveSelectedDown "One Object Down" enabled:( theSel = ((modPanel.getCurrentObject()).GetNodeListSelection()); theSel.count == 1 and theSel[1] != (modPanel.getCurrentObject()).nodeList.count)
				menuitem mnu_moveSelectedToBottom "To Bottom" enabled:(((modPanel.getCurrentObject()).GetNodeListSelection()).count > 0)
			)
			separator sep_300
			subMenu "SORT List"
			(
				menuitem mnu_sortAscending "ASCENDING Alphabetical Order"
				menuitem mnu_sortDescending "DESCENDING Alphabetical Order"
				separator sep_400
				menuitem mnu_sortCreationOrder "CREATION ORDER In The Scene"
				menuitem mnu_sortByType "By Object TYPE"
			)
			separator sep_500
			menuitem mnu_reverseOrder "REVERSE Current List Order"
			
			
			fn sortListA v1 v2 valArray: =
			(
				v1i = valArray[v1]
				v2i = valArray[v2]
				if v1i.name > v2i.name then 1 else -1
			)
			fn sortListD v1 v2 valArray: =
			(
				v1i = valArray[v1]
				v2i = valArray[v2]
				if v1i.name < v2i.name then 1 else -1
			)
			
			on mnu_highlightAll picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theCount = theFrostBaseObject.nodeList.count
				theFrostBaseObject.SetNodeListSelection (#{1..theCount} as array) 
			)
			on mnu_highlightInvert picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theCount = theFrostBaseObject.nodeList.count
				theFrostBaseObject.SetNodeListSelection ((#{1..theCount}-(theFrostBaseObject.GetNodeListSelection() as bitarray)) as array) 
			)		
			on mnu_highlightSimilar picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theSelected = (theFrostBaseObject.GetNodeListSelection())[1]
				local theClass = classof theFrostBaseObject.nodeList[theSelected]
				theFrostBaseObject.SetNodeListSelection (for i = 1 to theFrostBaseObject.nodeList.count where (classof theFrostBaseObject.nodeList[i]) == theClass collect i)
			)
			
			on mnu_selectHighlighted picked do
			(
				local theSelection = (modPanel.getCurrentObject()).GetNodeListSelection()
				local NodeList = #()
				for i in theSelection where isValidNode (modPanel.getCurrentObject()).nodeList[i] do 
					append NodeList (modPanel.getCurrentObject()).nodeList[i]
				try(select NodeList)catch()
			)
			
			on mnu_moveSelectedToTop picked do
			(
				with undo "Frost Move Top"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = theFrostBaseObject.GetNodeListSelection()
					local NodeList = #()
					local theNewSelection = #()
					for i in theSelection do 
					(
						append NodeList theFrostBaseObject.nodeList[i]
						append theNewSelection NodeList.count
					)
					for i = 1 to theFrostBaseObject.nodeList.count where findItem theSelection i == 0 do append NodeList theFrostBaseObject.nodeList[i]
					theFrostBaseObject.nodeList = #()
					theFrostBaseObject.NodeList = NodeList
					theFrostBaseObject.SetNodeListSelection theNewSelection
				)
			)
			
			on mnu_moveSelectedUp picked do
			(
				with undo "Frost Move Up"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = (modPanel.getCurrentObject()).GetNodeListSelection()
					if theSelection.count == 1 then
					(
						local nodeList = for o in (modPanel.getCurrentObject()).NodeList collect o
						local tempEntry = nodeList[theSelection[1]]
						nodeList[theSelection[1]] = nodeList[theSelection[1]-1]
						nodeList[theSelection[1]-1] = tempEntry
						(modPanel.getCurrentObject()).NodeList = #()
						(modPanel.getCurrentObject()).NodeList = NodeList
						(modPanel.getCurrentObject()).SetNodeListSelection #(theSelection[1]-1)
					)
				)
			)
			on mnu_moveSelectedDown picked do
			(
				with undo "Frost Move Down"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = (modPanel.getCurrentObject()).GetNodeListSelection()
					if theSelection.count == 1 then
					(
						local nodeList = for o in (modPanel.getCurrentObject()).NodeList collect o
						local tempEntry = nodeList[theSelection[1]]
						nodeList[theSelection[1]] = nodeList[theSelection[1]+1]
						nodeList[theSelection[1]+1] = tempEntry
						(modPanel.getCurrentObject()).NodeList = #()
						(modPanel.getCurrentObject()).NodeList = NodeList
						(modPanel.getCurrentObject()).SetNodeListSelection #(theSelection[1]+1)
					)
				)
			)
			
			on mnu_moveSelectedToBottom picked do
			(
				with undo "Frost Move Bottom"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = theFrostBaseObject.GetNodeListSelection()
					local NodeList = #()
					local theNewSelection = #()
					for i = 1 to theFrostBaseObject.nodeList.count where findItem theSelection i == 0 do append NodeList theFrostBaseObject.nodeList[i]
					for i in theSelection do 
					(
						append NodeList theFrostBaseObject.nodeList[i]
						append theNewSelection NodeList.count
					)
					theFrostBaseObject.nodeList = #()
					theFrostBaseObject.NodeList = NodeList
					theFrostBaseObject.SetNodeListSelection theNewSelection
				)
			)
			
			on mnu_sortAscending picked do
			(
				with undo "Frost Sort Ascending" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.nodeList where o != undefined collect o)
					local theIndexList = for i = 1 to NodeList.count collect i
					qsort theIndexList sortListA valArray:NodeList
					theFrostBaseObject.nodeList = #()
					theFrostBaseObject.NodeList = for i in theIndexList collect NodeList[i]
				)
			)
			on mnu_sortDescending picked do
			(
				with undo "Frost Sort Descending" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.nodeList where o != undefined collect o)
					local theIndexList = for i = 1 to NodeList.count collect i
					qsort theIndexList sortListD valArray:NodeList
					theFrostBaseObject.nodeList = #()
					theFrostBaseObject.NodeList = for i in theIndexList collect NodeList[i]
				)
			)	
			on mnu_sortCreationOrder picked do
			(
				with undo "Frost Creation Order" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.nodeList where o != undefined collect o)
					local newArray = for o in objects where findItem NodeList o > 0 collect o
					theFrostBaseObject.nodeList = #()
					theFrostBaseObject.NodeList = newArray
				)
			)	
			on mnu_reverseOrder picked do
			(
				with undo "Frost Reverse List" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.nodeList where o != undefined collect o)
					local newArray = for i = NodeList.count to 1 by -1 collect NodeList[i]
					theFrostBaseObject.nodeList = #()
					theFrostBaseObject.NodeList = newArray
				)
			)
			on mnu_sortByType picked do
			(
				with undo "Frost Sort Type" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.nodeList where o != undefined collect o)
					local theClasses = #()
					for o in NodeList where findItem theClasses ((classof o) as string) == 0 do append theClasses ((classof o) as string)
					sort theClasses 
					local newArray = #()
					for i in theClasses do
						for o in NodeList where (classof o) as string == i do append newArray o
					theFrostBaseObject.nodeList = #()
					theFrostBaseObject.nodeList = newArray
				)
			)
			
			on mnu_removeDeletedObjects picked do
			(
				with undo "Frost Remove Deleted" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local newNodeList = (for o in theFrostBaseObject.nodeList where o != undefined collect o)
					if newNodeList.count != theFrostBaseObject.nodeList.count do
					(
						theFrostBaseObject.nodeList = #()
						theFrostBaseObject.nodeList = newNodeList
					)
				)
			)
		)
		if modPanel.getCurrentObject() == undefined do return()
		popUpMenu Frost_NodeListOptions_RCMenu pos:mouse.screenpos
	),

	fn on_NodeList_doubleClicked frostNode =
	(
		local theSelection = (modPanel.getCurrentObject()).GetNodeListSelection()
		local NodeList = #()
		for i in theSelection where isValidNode (modPanel.getCurrentObject()).nodeList[i] do
			append NodeList (modPanel.getCurrentObject()).nodeList[i]
		try(select NodeList)catch()
	),

	fn on_NodeList_rightClicked frostNode =
	(
		rcmenu Frost_NodeListOptions_RCMenu
		(
			menuitem mnu_selectHighlighted "SELECT In the Scene"
			separator sep_100
			submenu "MOVE In The List"
			(
				menuitem mnu_moveSelectedToTop "To TOP" enabled:(((modPanel.getCurrentObject()).GetNodeListSelection()).count > 0)
				menuitem mnu_moveSelectedUp "One Object UP" enabled:( theSel = ((modPanel.getCurrentObject()).GetNodeListSelection()); theSel.count == 1 and theSel[1] != 1)
				menuitem mnu_moveSelectedDown "One Object DOWN" enabled:( theSel = ((modPanel.getCurrentObject()).GetNodeListSelection()); theSel.count == 1 and theSel[1] != (modPanel.getCurrentObject()).nodeList.count)
				menuitem mnu_moveSelectedToBottom "To BOTTOM" enabled:(((modPanel.getCurrentObject()).GetNodeListSelection()).count > 0)
			)
			separator sep_200
			menuitem mnu_removeHighlighted "REMOVE From The List"			
			
			on mnu_selectHighlighted picked do
			(
				local theSelection = (modPanel.getCurrentObject()).GetNodeListSelection()
				local NodeList = #()
				for i in theSelection where isValidNode (modPanel.getCurrentObject()).nodeList[i] do 
					append NodeList (modPanel.getCurrentObject()).nodeList[i]
				try(select NodeList)catch()
			)
			
			on mnu_moveSelectedToTop picked do
			(
				with undo "Frost Move Top"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = theFrostBaseObject.GetNodeListSelection()
					local NodeList = #()
					local theNewSelection = #()
					for i in theSelection do 
					(
						append NodeList theFrostBaseObject.nodeList[i]
						append theNewSelection NodeList.count
					)
					for i = 1 to theFrostBaseObject.nodeList.count where findItem theSelection i == 0 do append NodeList theFrostBaseObject.nodeList[i]
					theFrostBaseObject.nodeList = #()
					theFrostBaseObject.NodeList = NodeList
					theFrostBaseObject.SetNodeListSelection theNewSelection
				)
			)
			
			on mnu_moveSelectedUp picked do
			(
				with undo "Frost Move Up"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = (modPanel.getCurrentObject()).GetNodeListSelection()
					if theSelection.count == 1 then
					(
						local nodeList = for o in (modPanel.getCurrentObject()).NodeList collect o
						local tempEntry = nodeList[theSelection[1]]
						nodeList[theSelection[1]] = nodeList[theSelection[1]-1]
						nodeList[theSelection[1]-1] = tempEntry
						(modPanel.getCurrentObject()).NodeList = #()
						(modPanel.getCurrentObject()).NodeList = NodeList
						(modPanel.getCurrentObject()).SetNodeListSelection #(theSelection[1]-1)
					)
				)
			)
			on mnu_moveSelectedDown picked do
			(
				with undo "Frost Move Down"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = (modPanel.getCurrentObject()).GetNodeListSelection()
					if theSelection.count == 1 then
					(
						local nodeList = for o in (modPanel.getCurrentObject()).NodeList collect o
						local tempEntry = nodeList[theSelection[1]]
						nodeList[theSelection[1]] = nodeList[theSelection[1]+1]
						nodeList[theSelection[1]+1] = tempEntry
						(modPanel.getCurrentObject()).NodeList = #()
						(modPanel.getCurrentObject()).NodeList = NodeList
						(modPanel.getCurrentObject()).SetNodeListSelection #(theSelection[1]+1)
					)
				)
			)
			
			on mnu_moveSelectedToBottom picked do
			(
				with undo "Frost Move Bottom"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = theFrostBaseObject.GetNodeListSelection()
					local NodeList = #()
					local theNewSelection = #()
					for i = 1 to theFrostBaseObject.nodeList.count where findItem theSelection i == 0 do append NodeList theFrostBaseObject.nodeList[i]
					for i in theSelection do 
					(
						append NodeList theFrostBaseObject.nodeList[i]
						append theNewSelection NodeList.count
					)
					theFrostBaseObject.nodeList = #()
					theFrostBaseObject.NodeList = NodeList
					theFrostBaseObject.SetNodeListSelection theNewSelection
				)
			)
			
			on mnu_removeHighlighted picked do
			(
				with undo "Frost Node Remove"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = theFrostBaseObject.GetNodeListSelection()
					for i = theFrostBaseObject.nodeList.count to 1 by -1 where findItem theSelection i != 0 do deleteItem theFrostBaseObject.nodeList i
				)				
			)
		)
		popUpMenu Frost_NodeListOptions_RCMenu pos:mouse.screenpos
	),

	fn on_PFEventListOptions_pressed frostNode =
	(
		rcmenu Frost_PFEventListOptions_RCMenu
		(
			menuitem mnu_removeDeletedEvents "REMOVE DELETED Events"
			
			on mnu_removeDeletedEvents picked do
			(
				with undo "Frost Remove Deleted" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local newEventList = (for o in theFrostBaseObject.pfEventList where o != undefined collect o)
					if newEventList.count != theFrostBaseObject.pfEventList.count do
					(
						theFrostBaseObject.pfEventList = #()
						theFrostBaseObject.pfEventList = newEventList
					)
				)
			)
		)
		if modPanel.getCurrentObject() == undefined do return()
		popUpMenu Frost_PFEventListOptions_RCMenu pos:mouse.screenpos
	),

	fn on_PFEventList_rightClicked frostNode =
	(
	),

	fn on_TPGroupListOptions_pressed frostNode =
	(
		rcmenu Frost_TPGroupListOptions_RCMenu
		(
			menuitem mnu_removeDeletedEvents "REMOVE DELETED Groups"

			on mnu_removeDeletedEvents picked do
			(
				with undo "Frost Remove Deleted" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local newGroupList = (for o in theFrostBaseObject.tpGroupList where o != undefined collect o)
					if newGroupList.count != theFrostBaseObject.tpGroupList.count do
					(
						theFrostBaseObject.tpGroupList = #()
						theFrostBaseObject.tpGroupList = newGroupList
					)
				)
			)
		)
		if modPanel.getCurrentObject() == undefined do return()
		popUpMenu Frost_TPGroupListOptions_RCMenu pos:mouse.screenpos
	),

	fn on_TPGroupList_rightClicked frostNode =
	(
	),

	fn openEditor =
	(
		--disabled
	),

	fn on_addFile_pressed frostNode = (
		local fileToAdd = getOpenFileName caption:"Select the Particle File Sequence" types:"All Particle Files|*.prt;*.sprt;*.bin;*.rpc;*.csv;*.las;*.e57;*.pts;*.ptx;*.xyz;*.ptg;*.ply|Thinkbox Particle Files (*.prt, *.sprt)|*.prt;*.sprt|RealFlow Particle Files (*.bin, *.rpc)|*.bin;*.rpc|CSV Files (*.csv)|*.csv|LAS Files (*.las)|*.las|E57 Files (*.e57)|*.e57|PTS Files (*.pts)|*.pts|PTX Files (*.ptx)|*.ptx|XYZ Files (*.xyz)|*.xyz|PTG Files (*.ptg)|*.ptg|PLY Files (*.ply)|*.ply|All Files (*.*)|*.*|"

		-- If the file type is .PTS, .PTX, or .XYZ, then we must convert it to a .PRT
		-- before we can add it to the file list.
		if fileToAdd != undefined do 
		(
			local ext = toLower( getFilenameType fileToAdd )
			-- case sensitivity ?
			if (ext == ".pts") or (ext == ".ptx") or (ext == ".xyz") or (ext == ".ply") then
			(
				local convertedFileToAdd = FrostUtils.ShowConvertToPRTFileDialog fileToAdd
				if convertedFileToAdd != undefined and convertedFileToAdd != "" then
				(
					fileToAdd = convertedFileToAdd
				)
				else
				(
					fileToAdd = undefined
				)
			)
		)

		if fileToAdd != undefined do
		(
			if findItem frostNode.fileList fileToAdd == 0 then --if the file isn't already in fileList
			(
				append frostNode.fileList fileToAdd
			) else (
				messageBox("This file already exists in the list.\n") title:"Frost"
			)
		)
	),
	
	fn on_removeFile_pressed frostNode = (
		local toDelete = frostNode.GetFileListSelection()
		if toDelete.count > 0 then
		(
			txt = "Are you sure you want to REMOVE the "
			txt += if toDelete.count == 1 then "File Sequence\n" else (toDelete.count as string + " File Sequences\n" )
			theCnt = toDelete.count
			if theCnt >= 50 do theCnt = 50
			for i = 1 to theCnt do txt += frostNode.fileList[toDelete[i]]+"\n"
			if theCnt < toDelete.count do txt += "...and "+ (toDelete.count - theCnt) as string +" more?\n"
			if (querybox txt title:"Frost : Remove File Sequence") do
			(
				with undo "Frost Remove Files" on 
				(
					for i = toDelete.count to 1 by -1 do 
					(
						local theFile = frostNode.fileList[toDelete[i]]
						deleteItem frostNode.fileList toDelete[i]
					)
				)
			)
		)
	),

	fn on_editFile_pressed frostNode = (
		rcmenu Frost_EditFile_RCMenu
		(
			fn canPastePaths = (
				try
				(
					local clipboardClass = dotNetClass  "System.Windows.Forms.Clipboard"
					local theFilesString = clipboardClass.GetText()
					local thePaths = filterString theFilesString "\n"
					(for f in thePaths where pathConfig.isLegalPath f and getFileNamePath f != "" and findItem (modPanel.getCurrentObject()).fileList f == 0 and (matchPattern (getFileNameType f) pattern:".prt" or matchPattern (getFileNameType f) pattern:".csv" or matchPattern (getFileNameType f) pattern:".bin" ) collect f).count >0
				)
				catch(false)
			)
			menuitem mnu_copyAllToClipboard "Copy ALL Filenames To Windows Clipboard" 
			menuitem mnu_copySelectedToClipboard "Copy SELECTED Filenames To Windows Clipboard" 
			separator sep_20 filter:canPastePaths
			menuitem mnu_pasteFromClipboard "Paste Filenames From Windows Clipboard" filter:canPastePaths
			separator sep_30
			menuitem mnu_convertToPRT "Convert Particle File To PRT..."
			
			on mnu_copyAllToClipboard picked do
			(
				if (modPanel.getCurrentObject()).fileList.count == 1 then 
				(
					theText =  (modPanel.getCurrentObject()).fileList[1]
				)
				else 
				(
					theText = "--FROST FILE LIST: [" + (modPanel.getCurrentObject()).name + "]\n"
					for i in (modPanel.getCurrentObject()).fileList do theText += i + "\n"
				)
				clipboardClass = dotNetClass  "System.Windows.Forms.Clipboard"
				clipboardClass.SetText theText
			)

			on mnu_copySelectedToClipboard picked do
			(
				local toCopy = (modPanel.getCurrentObject()).GetFileListSelection()
				if toCopy.count == 1 then 
				(
					theText =  (modPanel.getCurrentObject()).fileList[toCopy[1]]
				)
				else
				(
					theText = "--FROST FILE LIST SELECTION: [" + (modPanel.getCurrentObject()).name + "]\n"
					for i in toCopy do theText += (modPanel.getCurrentObject()).fileList[i] + "\n"
				)
				clipboardClass = dotNetClass  "System.Windows.Forms.Clipboard"
				clipboardClass.SetText theText
			)

			on mnu_pasteFromClipboard picked do
			(
				clipboardClass = dotNetClass  "System.Windows.Forms.Clipboard"
				local theFilesString = clipboardClass.GetText()
				local theSS = theFilesString as StringStream
				theFileList = (modPanel.getCurrentObject()).fileList
				local changesMade = false
				while not eof theSS do
				(
					local theLine = readLine theSS
					if pathConfig.isLegalPath theLine and getFileNamePath theLine != "" do
					(
						if findItem theFileList theLine == 0 do
						(
							append theFileList theLine
							changesMade = true
						)
					)
				)
				if changesMade do
				(
					(modPanel.getCurrentObject()).fileList = theFileList 
				)
			)
			
			on mnu_convertToPRT picked do
			(
				FrostUtils.ShowConvertToPRTFileDialog ""
			)
		)
		if (modPanel.getCurrentObject()) == undefined do return()
		popUpMenu Frost_EditFile_RCMenu pos:mouse.screenpos
	),

	fn on_fileList_doubleClicked frostNode itm = (
		try (shellLaunch "explorer.exe" (getFileNamePath frostNode.fileList[itm]))catch()
	),

	fn on_FileList_rightClicked frostNode =
	(
		
	),

	fn createPresetsRCMenu type:#radius menuHead:"" =
	(
		case type of
		(
			#fileCustomScale : (
					presetName = "fileCustomScale"
					theParameter = "fileCustomScale"
				)
			#viewportLoadPercent : (
					presetName = "viewportLoadPercent"
					theParameter = "viewportLoadPercent"
				)
			#zhuBridsonBlendRadiusScale : (
					presetName = "zhuBridsonBlendRadiusScale"
					theParameter = "zhuBridsonBlendRadiusScale"
				)
			#zhuBridsonLowDensityTrimmingThreshold : (
					presetName = "zhuBridsonLowDensityTrimmingThreshold"
					theParameter = "zhuBridsonLowDensityTrimmingThreshold"
				)
			#zhuBridsonLowDensityTrimmingStrength : (
					presetName = "zhuBridsonLowDensityTrimmingStrength"
					theParameter = "zhuBridsonLowDensityTrimmingStrength"
				)
			#metaballRadiusScale : (
					presetName = "metaballRadiusScale"
					theParameter = "metaballRadiusScale"
				)
			#metaballIsosurfaceLevel : (
					presetName = "metaballIsosurfaceLevel"
					theParameter = "metaballIsosurfaceLevel"
				)
			#anisotropicRadiusScale : (
					presetName = "anisotropicRadiusScale"
					theParameter = "anisotropicRadiusScale"
				)
			#anisotropicWindowScale : (
					presetName = "anisotropicWindowScale"
					theParameter = "anisotropicWindowScale"
				)
			#anisotropicIsosurfaceLevel : (
					presetName = "anisotropicIsosurfaceLevel"
					theParameter = "anisotropicIsosurfaceLevel"
				)
			#anisotropicMaxAnisotropy : (
					presetName = "anisotropicMaxAnisotropy"
					theParameter = "anisotropicMaxAnisotropy"
				)
			#anisotropicMinNeighborCount : (
					presetName = "anisotropicMinNeighborCount"
					theParameter = "anisotropicMinNeighborCount"
				)
			#anisotropicPositionSmoothingWeight : (
					presetName = "anisotropicPositionSmoothingWeight"
					theParameter = "anisotropicPositionSmoothingWeight"
				)
			#anisotropicPositionSmoothingWindowScale : (
					presetName = "anisotropicPositionSmoothingWindowScale"
					theParameter = "anisotropicPositionSmoothingWindowScale"
				)
			#renderMeshingResolution : (
					presetName = "renderMeshingResolution"
					theParameter = "renderMeshingResolution"
				)
			#renderVertRefinementIterations : (
					presetName = "renderVertRefinementIterations"
					theParameter = "renderVertRefinementIterations"
				)
			#viewportMeshingResolution : (
					presetName = "viewportMeshingResolution"
					theParameter = "viewportMeshingResolution"
				)
			#viewportVertRefinementIterations : (
					presetName = "viewportVertRefinementIterations"
					theParameter = "viewportVertRefinementIterations"
				)
			#renderVoxelLength : (
					presetName = "renderVoxelLength"
					theParameter = "renderVoxelLength"
				)
			#viewportVoxelLength : (
					presetName = "viewportVoxelLength"
					theParameter = "viewportVoxelLength"
				)
			#radius : (
					presetName = "radius"
					theParameter = "radius"
				)
			#radiusScale : (
					presetName = "radiusScale"
					theParameter = "radiusScale"
				)
			#radiusRandomVariation : (
					presetName = "radiusRandomVariation"
					theParameter = "radiusRandomVariation"
				)
			#radiusRandomSeed : (
					presetName = "radiusRandomSeed"
					theParameter = "radiusRandomSeed"
				)
			#geometrySelectionSeed : (
					presetName = "geometrySelectionSeed"
					theParameter = "geometrySelectionSeed"
				)
			#geometrySampleTimeSeed : (
					presetName = "geometrySampleTimeSeed"
					theParameter = "geometrySampleTimeSeed"
				)
			#geometrySampleTimeMaxRandomOffset : (
					presetName = "geometrySampleTimeMaxRandomOffset"
					theParameter = "geometrySampleTimeMaxRandomOffset"
				)
			#geometryOrientationDivergence : (
					presetName = "geometryOrientationDivergence"
					theParameter = "geometryOrientationDivergence"
				)
		)
		local presetsList = #()
		makeDir Frost_PresetsDirectory all:true
		local theKeys = for i in ( getIniSetting ( FrostUtils.GetValuePresetsFilename() ) presetName  ) collect (execute i)
		sort theKeys 
		if theKeys.count == 0 then 
		(
			theKeys = case type of
			(
				#fileCustomScale : #(10,39.37,100)
				#viewportLoadPercent : #(0.1,1.0,100.0)
				#radius : #(1.0,2.0,3.0,5.0,10.0)
				#radiusScale : #(1.0)
				#radiusRandomVariation : #(10.0,40.0,50.0,80.0,95.0)
				#radiusRandomSeed: #(1,4379,12345,36096)
				#renderMeshingResolution: #(1.0,2.0,3.0,4.0,5.0,6.0,8.0,10.0)
				#renderVertRefinementIterations : #(1,3,10,20)
				#viewportMeshingResolution: #(0.5,1.0,1.5,2.0,2.5,3.0,4.0,5.0)
				#viewportVertRefinementIterations: #(0,1,3,10)
				#renderVoxelLength: #(0.5,1,1.5,2,2.5,3,5,10)
				#viewportVoxelLength: #(1,1.5,2,2.5,3,5,10,20)
				#metaballRadiusScale: #(1.1,1.7,2.5)
				#metaballIsosurfaceLevel : #(0.1,0.3,0.5,0.7,1.0)
				#zhuBridsonBlendRadiusScale : #(1.1,1.5,1.7,2.0,2.5)
				#zhuBridsonLowDensityTrimmingThreshold: #(0.01,0.1,0.5,1.0,2.0,5.0,10.0)
				#zhuBridsonLowDensityTrimmingStrength: #(5.0,10.0,15.0,20.0,30.0)
				#anisotropicRadiusScale : #(1.0,2.0,2.5,4.0)
				#anisotropicWindowScale : #(1.0,2.0)
				#anisotropicIsosurfaceLevel : #(0.1,0.3,0.5,0.7,1.0)
				#anisotropicMaxAnisotropy : #(1.0,2.0,3.0,4.0,5.0)
				#anisotropicMinNeighborCount : #(6,9,16,25,36,49,64)
				#anisotropicPositionSmoothingWeight : #(0.8,0.9,0.95,1.0)
				#anisotropicPositionSmoothingWindowScale : #(1.0,2.0)
				#geometrySelectionSeed : #(1,10898,12345,74030)
				#geometrySampleTimeSeed : #(1,9732,12345,86949)
				#geometrySampleTimeMaxRandomOffset: #(0,10,20,50,100)
				#geometryOrientationDivergence : #(0.0,15.0,30.0,180.0)
			)
			for i in theKeys do
				setIniSetting ( FrostUtils.GetValuePresetsFilename() ) presetName (i as string) (i as string)
		)
		for k in theKeys do
			if findItem presetsList theValue == 0 do append presetsList k

		theValue = execute ("(modPanel.getCurrentObject())." + theParameter)
		global Frost_Presets_RCMenu
		local txt = "rcmenu Frost_Presets_RCMenu\n(\n"
		
		txt += menuHead
		
		if findItem presetsList theValue == 0 do 
		(
			txt += "menuItem mnu_AddPreset \"Add "+ theValue as string+"\"\n" 
			txt += "on mnu_AddPreset picked do setIniSetting (FrostUtils.GetValuePresetsFilename()) \""+ presetName + "\" \""+ theValue as string +"\" \""+ theValue as string +"\" \n"
			txt += "separator spr_10\n" 
		)	
		
		cnt = 0
		for i in presetsList do
		(
			cnt += 1
			txt += "menuItem mnu_preset"+ cnt as string +" \"" + i as string + "\" \n" 
			txt += "on mnu_preset" + cnt as string + " picked do (modPanel.getCurrentObject())."+ theParameter +" = "+ i as string +"\n"
		)
		
		if findItem presetsList theValue != 0 do 
		(
			txt += "separator spr_20\n" 
			txt += "menuItem mnu_RemovePreset \"Remove "+ theValue as string+"\"\n" 
			txt += "on mnu_RemovePreset picked do delIniSetting (FrostUtils.GetValuePresetsFilename()) \""+ presetName +"\" \""+ theValue as string +"\" \n"
		)	
		txt += ")\n"
		execute txt
	),

	fn on_FileCustomScaleOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#fileCustomScale
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),

	fn on_ViewportLoadPercentOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#viewportLoadPercent
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),

	fn on_ZhuBridsonBlendRadiusScaleOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#zhuBridsonBlendRadiusScale
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_ZhuBridsonLowDensityTrimmingThresholdOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#zhuBridsonLowDensityTrimmingThreshold
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_ZhuBridsonLowDensityTrimmingStrengthOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#zhuBridsonLowDensityTrimmingStrength
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),

	fn on_MetaballRadiusScaleOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#metaballRadiusScale
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_MetaballIsosurfaceLevelOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#metaballIsosurfaceLevel
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),

	fn on_AnisotropicRadiusScaleOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#anisotropicRadiusScale
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_AnisotropicWindowScaleOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#anisotropicWindowScale
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_AnisotropicIsosurfaceLevelOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#anisotropicIsosurfaceLevel
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_AnisotropicMaxAnisotropyOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#anisotropicMaxAnisotropy
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_AnisotropicMinNeighborCountOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#anisotropicMinNeighborCount
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_AnisotropicPositionSmoothingWindowScaleOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#anisotropicPositionSmoothingWindowScale
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_AnisotropicPositionSmoothingWeightOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#anisotropicPositionSmoothingWeight
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),

	fn on_RenderMeshingResolutionOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#renderMeshingResolution
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_RenderVertRefinementIterationsOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#renderVertRefinementIterations
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_ViewportMeshingResolutionOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#viewportMeshingResolution
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_ViewportVertRefinementIterationsOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#viewportVertRefinementIterations
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_RenderVoxelLengthOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#renderVoxelLength
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_ViewportVoxelLengthOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#viewportVoxelLength
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),

	fn on_RadiusOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#radius
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_RadiusScaleOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#radiusScale
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_RadiusRandomVariationOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#radiusRandomVariation
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_RadiusRandomSeedOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#radiusRandomSeed
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),

	fn on_GeometryListOptions_pressed frostNode =
	(
		rcmenu Frost_GeometryListOptions_RCMenu
		(
			menuitem mnu_selectHighlighted "SELECT Highlighted In the Scene" enabled:(((modPanel.getCurrentObject()).GetGeometryListSelection()).count > 0)
			separator sep_100
			menuitem mnu_removeDeletedObjects "REMOVE DELETE Objects"
			separator sep_150
			submenu "HIGHLIGHT Objects In The List"
			(
				menuItem mnu_highlightAll "ALL"
				menuItem mnu_highlightInvert "INVERT"
				separator sep_160
				menuItem mnu_highlightSimilar "SIMILAR" enabled:(((modPanel.getCurrentObject()).GetGeometryListSelection()).count == 1)
			)			
			separator sep_200
			subMenu "MOVE Highlighted Objects"
			(
				menuitem mnu_moveSelectedToTop "To TOP" enabled:(((modPanel.getCurrentObject()).GetGeometryListSelection()).count > 0)
				menuitem mnu_moveSelectedUp "One Object UP" enabled:( theSel = ((modPanel.getCurrentObject()).GetGeometryListSelection()); theSel.count == 1 and theSel[1] != 1)
				menuitem mnu_moveSelectedDown "One Object DOWN" enabled:( theSel = ((modPanel.getCurrentObject()).GetGeometryListSelection()); theSel.count == 1 and theSel[1] != (modPanel.getCurrentObject()).GeometryList.count)
				menuitem mnu_moveSelectedToBottom "To BOTTOM" enabled:(((modPanel.getCurrentObject()).GetGeometryListSelection()).count > 0)
			)
			separator sep_300
			subMenu "SORT List"
			(
				menuitem mnu_sortAscending "ASCENDING Alphabetical Order"
				menuitem mnu_sortDescending "DESCENDING Alphabetical Order"
				separator sep_400
				menuitem mnu_sortCreationOrder "CREATION ORDER In The Scene"
				menuitem mnu_sortByType "By Object TYPE"
			)
			separator sep_500
			menuitem mnu_reverseOrder "REVERSE Current List Order"

			fn sortListA v1 v2 valArray: =
			(
				v1i = valArray[v1]
				v2i = valArray[v2]
				if v1i.name > v2i.name then 1 else -1
			)
			fn sortListD v1 v2 valArray: =
			(
				v1i = valArray[v1]
				v2i = valArray[v2]
				if v1i.name < v2i.name then 1 else -1
			)
			
			on mnu_highlightAll picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theCount = theFrostBaseObject.geometryList.count
				theFrostBaseObject.SetGeometryListSelection (#{1..theCount} as array) 
			)
			on mnu_highlightInvert picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theCount = theFrostBaseObject.geometryList.count
				theFrostBaseObject.SetGeometryListSelection ((#{1..theCount}-(theFrostBaseObject.GetGeometryListSelection() as bitarray)) as array) 
			)		
			on mnu_highlightSimilar picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theSelected = (theFrostBaseObject.GetGeometryListSelection())[1]
				local theClass = classof theFrostBaseObject.geometryList[theSelected]
				theFrostBaseObject.SetGeometryListSelection (for i = 1 to theFrostBaseObject.geometryList.count where (classof theFrostBaseObject.geometryList[i]) == theClass collect i)
			)			
			
			on mnu_selectHighlighted picked do
			(
				local theSelection = (modPanel.getCurrentObject()).GetGeometryListSelection()
				local NodeList = #()
				for i in theSelection where isValidNode (modPanel.getCurrentObject()).geometryList[i] do 
					append NodeList (modPanel.getCurrentObject()).geometryList[i]
				try(select NodeList)catch()
			)
			
			on mnu_moveSelectedToTop picked do
			(
				with undo "Frost Geo Move Top"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = theFrostBaseObject.GetGeometryListSelection()
					local NodeList = #()
					local theNewSelection = #()
					for i in theSelection do 
					(
						append NodeList theFrostBaseObject.geometryList[i]
						append theNewSelection NodeList.count
					)
					for i = 1 to theFrostBaseObject.geometryList.count where findItem theSelection i == 0 do append NodeList theFrostBaseObject.geometryList[i]
					theFrostBaseObject.geometryList = #()
					theFrostBaseObject.geometryList = NodeList
					theFrostBaseObject.SetGeometryListSelection theNewSelection
				)
			)
			
			on mnu_moveSelectedUp picked do
			(
				with undo "Frost Geo Move Up"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = theFrostBaseObject.GetGeometryListSelection()
					if theSelection.count == 1 then
					(
						local nodeList = for o in theFrostBaseObject.geometryList collect o
						local tempEntry = nodeList[theSelection[1]]
						nodeList[theSelection[1]] = nodeList[theSelection[1]-1]
						nodeList[theSelection[1]-1] = tempEntry
						theFrostBaseObject.geometryList = #()
						theFrostBaseObject.geometryList = NodeList
						theFrostBaseObject.SetGeometryListSelection #(theSelection[1]-1)
					)
				)
			)
			on mnu_moveSelectedDown picked do
			(
				with undo "Frost Geo Move Down"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = theFrostBaseObject.GetGeometryListSelection()
					if theSelection.count == 1 then
					(
						local nodeList = for o in theFrostBaseObject.geometryList collect o
						local tempEntry = nodeList[theSelection[1]]
						nodeList[theSelection[1]] = nodeList[theSelection[1]+1]
						nodeList[theSelection[1]+1] = tempEntry
						theFrostBaseObject.geometryList = #()
						theFrostBaseObject.geometryList = NodeList
						theFrostBaseObject.SetGeometryListSelection #(theSelection[1]+1)
					)
				)
			)
			
			on mnu_moveSelectedToBottom picked do
			(
				with undo "Frost Geo Move Bottom"  on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theSelection = theFrostBaseObject.GetGeometryListSelection()
					local NodeList = #()
					local theNewSelection = #()
					for i = 1 to theFrostBaseObject.geometryList.count where findItem theSelection i == 0 do append NodeList theFrostBaseObject.geometryList[i]
					for i in theSelection do
					(
						append NodeList theFrostBaseObject.geometryList[i]
						append theNewSelection NodeList.count
					)
					theFrostBaseObject.geometryList = #()
					theFrostBaseObject.geometryList = NodeList
					theFrostBaseObject.SetGeometryListSelection theNewSelection
				)
			)
			
			on mnu_sortAscending picked do
			(
				with undo "Frost Geo Sort Ascending" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.geometryList where o != undefined collect o)
					local theIndexList = for i = 1 to NodeList.count collect i
					qsort theIndexList sortListA valArray:NodeList
					theFrostBaseObject.geometryList = #()
					theFrostBaseObject.geometryList = for i in theIndexList collect NodeList[i]
				)
			)
			on mnu_sortDescending picked do
			(
				with undo "Frost Geo Sort Descending" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.geometryList where o != undefined collect o)
					local theIndexList = for i = 1 to NodeList.count collect i
					qsort theIndexList sortListD valArray:NodeList
					theFrostBaseObject.geometryList = #()
					theFrostBaseObject.geometryList = for i in theIndexList collect NodeList[i]
				)
			)
			on mnu_sortCreationOrder picked do
			(
				with undo "Frost Geo Creation Order" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.geometryList where o != undefined collect o)
					local newArray = for o in objects where findItem NodeList o > 0 collect o
					theFrostBaseObject.geometryList = #()
					theFrostBaseObject.geometryList = newArray
				)
			)
			on mnu_reverseOrder picked do
			(
				with undo "Frost Rev. Geo List" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.geometryList where o != undefined collect o)
					local newArray = for i = NodeList.count to 1 by -1 collect NodeList[i]
					theFrostBaseObject.geometryList = #()
					theFrostBaseObject.geometryList = newArray
				)
			)
			on mnu_sortByType picked do
			(
				with undo "Frost Sort Type" on
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local NodeList = (for o in theFrostBaseObject.geometryList where o != undefined collect o)
					local theClasses = #()
					for o in NodeList where findItem theClasses ((classof o) as string) == 0 do append theClasses ((classof o) as string)
					sort theClasses 
					local newArray = #()
					for i in theClasses do
						for o in NodeList where (classof o) as string == i do append newArray o
					theFrostBaseObject.geometryList = #()
					theFrostBaseObject.geometryList = newArray
				)
			)
			
			on mnu_removeDeletedObjects picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local newGeometryList = (for o in theFrostBaseObject.geometryList where o != undefined collect o)
				if newGeometryList.count != theFrostBaseObject.geometryList.count do
				(
					theFrostBaseObject.geometryList = #()
					theFrostBaseObject.geometryList = newGeometryList
				)
			)
		)
		if modPanel.getCurrentObject() == undefined do return()
		popUpMenu Frost_GeometryListOptions_RCMenu pos:mouse.screenpos
	),
	fn on_GeometryList_doubleClicked frostNode =
	(
		local theSelection = (modPanel.getCurrentObject()).GetGeometryListSelection()
		local NodeList = #()
		for i in theSelection where isValidNode (modPanel.getCurrentObject()).geometryList[i] do
			append NodeList (modPanel.getCurrentObject()).geometryList[i]
		try(select NodeList)catch()
	),
	fn on_GeometryList_rightClicked frostNode =
	(
	),
	fn on_GeometrySelectionSeedOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#geometrySelectionSeed
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_GeometrySampleTimeSeedOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#geometrySampleTimeSeed
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_GeometrySampleTimeMaxRandomOffsetOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#geometrySampleTimeMaxRandomOffset
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),
	fn on_GeometryOrientationDivergenceOptions_pressed frostNode =
	(
		if modPanel.getCurrentObject() == undefined do return()
		FrostUi.createPresetsRCMenu type:#geometryOrientationDivergence
		popUpMenu ::Frost_Presets_RCMenu position:mouse.screenPos
	),

	fn getSubMaterial theMaterial theMaterialID =
	(
		if classof theMaterial == Multimaterial then
		(
			localMaterialID = 1 + ((mod (theMaterialID - 1) theMaterial.numsubs) as Integer)
			i = findItem theMaterial.materialIDList localMaterialID
			if i > 0 then
				return getSubMaterial theMaterial.materialList[i] theMaterialID
			else
				return undefined
		)
		else
		(
			return theMaterial
		)
	),
	fn clearGeometryMaterialIDMapping frostNode =
	(
		frostNode.geometryMaterialIDNodeList.count = 0
		frostNode.geometryMaterialIDInList.count = 0
		frostNode.geometryMaterialIDOutList.count = 0
	),
	fn addGeometryMaterialIDMapping frostNode geometryNode inMaterialID outMaterialID =
	(
		index = frostNode.geometryMaterialIDNodeList.count + 1
		frostNode.geometryMaterialIDNodeList[index] = geometryNode
		frostNode.geometryMaterialIDInList[index] = inMaterialID
		frostNode.geometryMaterialIDOutList[index] = outMaterialID
	),
	fn appendFrostMaterial frostNode outMaterialList outMaterialIDList inMaterial geometryNode inMaterialID undefinedOutMaterialID =
	(
		outMaterialID = undefined

		if inMaterial == undefined then
		(
			outMaterialID = undefinedOutMaterialID
		)
		else
		(
			i = findItem outMaterialList inMaterial
			if i > 0 do
			(
				outMaterialID = outMaterialIDList[i]
			)

			if outMaterialID == undefined do
			(
				lastOutMaterialID = if outMaterialIDList.count > 0 then outMaterialIDList[outMaterialIDList.count] else 0
				outMaterialID = 1 + lastOutMaterialID
				if outMaterialID == undefinedOutMaterialID do ( outMaterialID = 1 + outMaterialID )
				if outMaterialID >= 65537 do ( throw "Too many materials to combine" )
				append outMaterialList inMaterial
				append outMaterialIDList outMaterialID
			)
		)

		FrostUI.addGeometryMaterialIDMapping frostNode geometryNode inMaterialID outMaterialID
		return lastOutIndex
	),
	fn on_CreateMaterial_pressed frostNode =
	(
		try 
		(
			FrostUI.clearGeometryMaterialIDMapping frostNode
			undefinedMaterialID = frostNode.undefinedMaterialID
			materialList = #()
			materialIDList = #()

			for geometryNumber = 1 to frostNode.geometryList.count do
			(
				theNode = frostNode.geometryList[geometryNumber]
				if not isValidNode theNode do continue
				theMaterial = theNode.material
				if theMaterial == undefined do continue
				if classof theMaterial == Multimaterial then
				(
					for i = 1 to theMaterial.materialList.count do
					(
						theSubMaterial = theMaterial.materialList[i]
						theSubMaterialID = theMaterial.materialIDList[i]
						theSubMaterial = FrostUI.getSubMaterial theSubMaterial theSubMaterialID
						lastMaterialID = FrostUI.appendFrostMaterial frostNode materialList materialIDList theSubMaterial theNode theSubMaterialID undefinedMaterialID
					)
				)
				else
				(
					lastMaterialID = FrostUI.appendFrostMaterial frostNode materialList materialIDList theMaterial theNode 1 undefinedMaterialID
				)
			)

			if materialIDList.count < 65536 do
			(
				append materialIDList undefinedMaterialID
				append materialList undefined
			)

			frostMaterial = Multimaterial numsubs:materialList.count
			frostMaterial.materialList = materialList
			frostMaterial.materialIDList = materialIDList

			frostNode.material = frostMaterial
		)
		catch
		(
			messageBox(getCurrentException()) title:"Frost: Create Material Error"
		)
	),
	fn getFloatFromDialog prompt default =
	(
		rollout Frost_RadiusScaleAnimationTools_FloatPrompt "Frost: Scale with Age"
		(
			label lbl_1
			label lbl_caption "Frames:" align:#right across:2
			spinner spn_value range:[0,1000000,30] type:#float scale:1 align:#left
			label lbl_2
			button btn_ok "OK" width:75 align:#right offset:[75,0] across:2
			button btn_cancel "Cancel" width:75 align:#left offset:[75,0]

			on Frost_RadiusScaleAnimationTools_FloatPrompt open do
			(
				spn_value.value = Frost_RadiusScaleAnimationTools_Value
				lbl_caption.text = Frost_RadiusScaleAnimationTools_Prompt
			)

			on btn_ok pressed do
			(
				Frost_RadiusScaleAnimationTools_Value = spn_value.value
				destroyDialog Frost_RadiusScaleAnimationTools_FloatPrompt
			)

			on btn_cancel pressed do
			(
				Frost_RadiusScaleAnimationTools_Value = undefined
				destroyDialog Frost_RadiusScaleAnimationTools_FloatPrompt
			)
		)

		Frost_RadiusScaleAnimationTools_Value = default
		Frost_RadiusScaleAnimationTools_Prompt = prompt
		createDialog Frost_RadiusScaleAnimationTools_FloatPrompt 320 92 modal:true
		return Frost_RadiusScaleAnimationTools_Value
	),
	fn createRampController startTime startValue endTime endValue =
	(
		local theCtrl = bezier_float()

		local theKey = addNewKey theCtrl startTime
		theKey.inTangentType = theKey.outTangentType = #linear
		theKey.value = startValue

		theKey = addNewKey theCtrl endTime
		theKey.inTangentType = theKey.outTangentType = #linear
		theKey.value = endValue

		setBeforeORT theCtrl #constant
		setAfterORT theCtrl #constant
		
		return theCtrl
	),
	fn on_RadiusScaleAnimationTools_pressed frostNode =
	(
		rcmenu Frost_RadiusScaleAnimationTools_RCMenu
		(
			menuitem mnu_openTrackView "Open In TrackView" enabled:((modPanel.getCurrentObject()).Radius_Scale.isAnimated)
			separator sep_10 
			menuitem mnu_growOverFrames "Grow over N Frames..."
			menuitem mnu_growOverLifepan "Grow over LifeSpan"
			separator sep_20 
			menuitem mnu_shrinkOverFrames "Shrink over N Frames..."
			menuitem mnu_shrinkOverLifespan "Shrink over LifeSpan"
			separator sep_30 
			menuitem mnu_noScale "Delete All Scale Keys" enabled:((modPanel.getCurrentObject()).Radius_Scale.isAnimated)

			on mnu_noScale picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theCtrl = try(theFrostBaseObject.radiusScale.controller)catch(undefined)
				if theCtrl != undefined do
					try(deleteKeys theCtrl #allKeys)catch()
				theFrostBaseObject.radiusScale = 1
			)
			on mnu_growOverFrames picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local frameCount = FrostUI.getFloatFromDialog "Grow over frames:" 1s
				if frameCount != undefined do
				(
					local theCtrl = try(theFrostBaseObject.radiusScale.controller)catch(undefined)
					if theCtrl != undefined do
						try(deleteKeys theCtrl #allKeys)catch()
					theFrostBaseObject.radiusScale.controller = FrostUI.createRampController 0 0 frameCount 1
					theFrostBaseObject.radiusAnimationMode = 1
					theFrostBaseObject.enableRadiusScale = true
				)
			)
			on mnu_growOverLifepan picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theCtrl = try(theFrostBaseObject.radiusScale.controller)catch(undefined)
				if theCtrl != undefined do
					try(deleteKeys theCtrl #allKeys)catch()
				theFrostBaseObject.radiusScale.controller = FrostUI.createRampController 0 0 100 1
				theFrostBaseObject.radiusAnimationMode = 2
				theFrostBaseObject.enableRadiusScale = true
			)
			on mnu_shrinkOverFrames picked do
			(
				local frameCount = FrostUI.getFloatFromDialog "Shrink over frames:" 1s
				if frameCount != undefined do
				(
					local theFrostBaseObject = modPanel.getCurrentObject()
					local theCtrl = try(theFrostBaseObject.radiusScale.controller)catch(undefined)
					if theCtrl != undefined do
						try(deleteKeys theCtrl #allKeys)catch()
					theFrostBaseObject.radiusScale.controller = FrostUI.createRampController 0 1 frameCount 0
					theFrostBaseObject.radiusAnimationMode = 1
					theFrostBaseObject.enableRadiusScale = true
				)
			)
			on mnu_shrinkOverLifespan picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theCtrl = try(theFrostBaseObject.radiusScale.controller)catch(undefined)
				if theCtrl != undefined do
					try(deleteKeys theCtrl #allKeys)catch()
				theFrostBaseObject.radiusScale.controller = FrostUI.createRampController 0 1 100 0
				theFrostBaseObject.radiusAnimationMode = 2
				theFrostBaseObject.enableRadiusScale = true
			)
			on mnu_openTrackView picked do
			(
				local theFrostBaseObject = modPanel.getCurrentObject()
				local theNode = (refs.dependentNodes theFrostBaseObject)[1]
				if not theNode.isSelected do select theNode
				local theTVName = "["+theNode.name + "] Animated Tracks"
				trackView.open theTVName
				trackviews.current.zoomOnTrack theFrostBaseObject 30
				trackviews.setFilter theTVName 	#selectedObjects
				trackviews.setFilter theTVName 	#animatedTracks
			)
		)
		if modPanel.getCurrentObject() == undefined do return()
		popUpMenu Frost_RadiusScaleAnimationTools_RCMenu pos:mouse.screenpos
	)
)

try
(
	FrostUtils.PopupLogWindowOnError = true
	theVal = execute ( getIniSetting (FrostUtils.GetSettingsFilename()) "Logging" "PopupLogWindowOnError" )
	if theVal != OK do 
	(
		FrostUtils.PopupLogWindowOnError = theVal
	)
)
catch()
