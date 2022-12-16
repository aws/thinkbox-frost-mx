-- Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
-- SPDX-License-Identifier: Apache-2.0
(
	local CurrentVersion = "20160908"
	makeDir (GetDir #plugcfg + "\\Frost\\") all:true
	local isMenuRequested = not matchPattern (getIniSetting (GetDir #plugcfg + "\\Frost\\FrostPreferences.ini" ) "MenuSystem" "CreateMenu") pattern:"false"
	local theMainMenu = menuMan.getMainMenuBar()
	local theFrostMenu = menuMan.findMenu "Frost"
	local oldVersion = (getIniSetting (GetDir #plugcfg + "\\Frost\\FrostPreferences.ini" ) "MenuSystem" "LastVersion")
	if oldVersion == "" do oldVersion = "20100506"
	local needUpdate = oldVersion != CurrentVersion
	if theFrostMenu != undefined and (not isMenuRequested or needUpdate) do
	(
		if needUpdate then
			format "--Old Frost Menu Version % Found and Removed.\n" oldVersion
		else	
			format "--Frost Menu Found and Removed at User Request via the Frost Preferences\n"
		menuMan.unRegisterMenu theFrostMenu
		menuMan.updateMenuBar() 
		theFrostMenu = undefined
	)
	if theFrostMenu == undefined and isMenuRequested do
	(
		if needUpdate then
			format "--Creating Updated Frost Menu Version %\n" CurrentVersion
		else
			format "--Frost Menu Not Found - Creating...\n"
		
		theFrostMenu = menuMan.createMenu "Frost"

		theMenuDefinitionArray = #(
			#(#menuitem,"CreateFrost","Frost","CREATE FROST - Select Source(s) and Click to Create, or Hold SHIFT to Create Manually in the Viewport"),
			#(#separator),
			#(#menuitem,"AutoFrost","Frost","AUTO-FROST - Select a Frost Object and Toggle to Add Newly Created Objects Automatically"),
			#(#separator),
			#(#menuitem,"FrostLogToggle","Frost","FROST LOG - Toggle the Log Window On and Off"),
			#(#menuitem,"FrostOpenLogOnError","Frost","LOG ON ERROR - Open the Frost Log Window on Errors"),
			#(#separator),
			#(#submenustart,"SWITCH Selected Frost Objects To.."),
			#(#menuitem,"FrostGeo","Frost","GEOMETRY Meshing Mode"),
			#(#menuitem,"FrostUOS","Frost","UNION OF SPHERES Meshing Mode"),
			#(#menuitem,"FrostMB","Frost","METABALLS Meshing Mode"),
			#(#menuitem,"FrostZB","Frost","ZHU/BRIDSON Meshing Mode"),
			#(#menuitem,"FrostAniso","Frost","ANISOTROPIC Meshing Mode"),
			#(#menuitem,"FrostVertexCloud","Frost","VERTEX CLOUD Meshing Mode"),
			#(#submenuend)
		)
		local theSubMenu = undefined
		for aDef in theMenuDefinitionArray do
		(
			case aDef[1] of
			(
				#menuitem:
				(
					try(
						theAction = menuMan.createActionItem aDef[2] aDef[3]
						theAction.setTitle aDef[4]
						theAction.setUseCustomTitle true
						if theSubMenu == undefined then
							theFrostMenu.addItem theAction -1
						else
							theSubMenu.addItem theAction -1
					)catch()
				)
				#separator: 
				(
					try(
						theAction = menuMan.createSeparatorItem()
						if theSubMenu == undefined then
							theFrostMenu.addItem theAction -1
						else
							theSubMenu.addItem theAction -1
					)catch()
				)
				#submenustart:
				(
					try(
						theSubMenu = menuMan.createMenu aDef[2]
					)catch()
				)
				#submenuend: 
				(
					try(
						theAction = menuMan.createSubMenuItem (theSubMenu.getTitle()) theSubMenu
						theFrostMenu.addItem theAction -1	
					)catch()
					theSubMenu = undefined
				)
			)--end case
		)--end aDef loop
		theSubMenu = menuMan.createSubMenuItem "Frost" theFrostMenu
		theMainMenu.addItem theSubMenu -1
		menuMan.updateMenuBar() 
		setIniSetting (GetDir #plugcfg + "\\Frost\\FrostPreferences.ini" ) "MenuSystem" "LastVersion" CurrentVersion
	)--end if
)--end script