-- Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
-- SPDX-License-Identifier: Apache-2.0
global Frost_PresetsOptions 
global Frost_PresetsOptions_Dialog
global Frost_PresetsOptions_Filename = ""

rollout Frost_PresetsOptions_Dialog "Frost Preset Options"
(
	edittext edt_newPreset "Name:"  text:"Frost Preset"
	multilistbox lbx_propertiesToSave "Save The Following Properties:" items:#() height:45
	button btn_selectAll "All" across:3 width:99 height:18 offset:[-1,-4] --across:3 --align:#left offset:[-5,-3]
	button btn_selectInvert "Invert" width:99 height:18 offset:[0,-4] --align:#center offset:[0,-3]
	button btn_selectDefault "Default" width:99 height:18 offset:[1,-4] --align:#right offset:[5,-3]

	button btn_savePreset "SAVE PRESET" height:25 width:148 across:2 offset:[-1,0]
	button btn_cancel "CANCEL" height:25 width:148 offset:[1,0]

	on edt_newPreset entered val do
	(
		edt_newPreset.text = ( FrostUtils.SanitizePresetName val )
	)

	on btn_selectAll pressed do lbx_propertiesToSave.selection = #{1..lbx_propertiesToSave.items.count}
	on btn_selectInvert pressed do lbx_propertiesToSave.selection = -lbx_propertiesToSave.selection
	on btn_selectDefault pressed do lbx_propertiesToSave.selection = (for i = 1 to Frost_PresetsOptions.count where Frost_PresetsOptions[i].save collect i) as bitarray

	on Frost_PresetsOptions_Dialog open do
	(
		Frost_PresetsOptions_Filename = ""
		lbx_propertiesToSave.items = for i in Frost_PresetsOptions collect i.label
		lbx_propertiesToSave.selection = (for i = 1 to Frost_PresetsOptions.count where Frost_PresetsOptions[i].save collect i) as bitarray
	)

	on btn_savePreset pressed do
	(
		edt_newPreset.text = FrostUtils.SanitizePresetName edt_newPreset.text
		local thePresetName = edt_newPreset.text
		local thePresetFile = FrostUi.getPresetFilename thePresetName
		if doesFileExist thePresetFile do
		(
			local result = queryBox (thePresetName + " already exists.\n\nDo you want to replace it?") title:"Confirm Save As"
			if result == false do return()
		)
		Frost_PresetsOptions_Filename = thePresetName
		for i = 1 to Frost_PresetsOptions.count do
		(
			Frost_PresetsOptions[i].save = lbx_propertiesToSave.selection[i]
		)
		destroyDialog Frost_PresetsOptions_Dialog
	)

	on btn_cancel pressed do
	(
		Frost_PresetsOptions_Filename = ""
		for i = 1 to Frost_PresetsOptions.count do
		(
			Frost_PresetsOptions[i].save = lbx_propertiesToSave.selection[i]
		)
		destroyDialog Frost_PresetsOptions_Dialog
	)
)
