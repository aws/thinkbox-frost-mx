-- Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
-- SPDX-License-Identifier: Apache-2.0
macroScript CreateFrost category:"Frost" buttontext:"Frost" tooltip:"FROST Particle Mesher - Select Source(s) and Click to Create, or Hold SHIFT to Create Manually in the Viewport" icon:#("Frost",1)
(
	on isChecked return  (try(mcrUtils.IsCreating Frost)catch(false)) 
	on isEnabled return Frost != undefined	
	on execute do 
	(
		local theObjects = (for o in selection where (findItem shape.classes (classof o) > 0) OR  (classof o == SphereGizmo) OR (findItem GeometryClass.classes (classof o) > 0 AND (classof o) != TargetObject AND (classof o) != ParticleGroup) collect o) 
		if keyboard.shiftPressed or theObjects.count == 0 then
		(
			StartObjectCreation Frost
		)
		else
		(
			local oldTaskMode = getCommandPanelTaskMode()
			try
			(
				theCenter = selection.center
				theColor = color 0 0 0
				for o in theObjects do theColor += o.wirecolor
				theColor /= theObjects.count
				local newFrost = Frost()
				select newFrost
				setCommandPanelTaskMode #modify
				newFrost.pos = theCenter
				newFrost.wirecolor = theColor
				newFrost.iconsize = 50.0
				newFrost.nodeList = theObjects 
			)
			catch
			(
				messagebox "One or more selected objects were not valid particle sources.\nPlease make sure all selected objects are valid!" title:"Failed to Add to Frost"
			)
			setCommandPanelTaskMode oldTaskMode
		)
	)
)

macroScript AutoFrost tooltip:"Select a Frost object and toggle to add newly created objects to it automatically..." category:"Frost" icon:#("Frost",2)
(
global FrostSphereGizmoAutoAdd_System, FrostSphereGizmoAutoAdd_Function
fn FrostSphereGizmoAutoAdd_Function val =
(
	try
	(
		if isValidNode ::FrostSphereGizmoAutoAdd_System do(
			if finditem #(ParticleGroup) (classof val) == 0 do(
				--if a PRT Loader is about to be created from the current Frost system, this would lead to a dependency loop and should be avoided:
				if classof val == PRT_Volume and selection.count == 1 and selection[1] == ::FrostSphereGizmoAutoAdd_System do return false
				append ::FrostSphereGizmoAutoAdd_System.nodeList val
				if superclassof val == Shape do 
				(
					val.optimize = false
					val.steps = 50
				)
			)
			FrostUtils.LogProgress("+AUTO-FROST: Added " + (val.name as String) + " to " + (::FrostSphereGizmoAutoAdd_System.name as String))
		)
	)catch(FrostUtils.LogProgress("--AUTO-FROST: Failed To Add " + (val.name as String) + "..."))
)

on isChecked return isValidNode ::FrostSphereGizmoAutoAdd_System 
on isEnabled return Frost != undefined	
on execute do
(
	callbacks.removeScripts id:#FrostSphereGizmoAutoAdd
	if isValidNode ::FrostSphereGizmoAutoAdd_System then
	(
		::FrostSphereGizmoAutoAdd_System = undefined
		FrostUtils.LogProgress "--AUTO-FROST: Turned Off!"
	)
	else
	(
		if (selection.count == 1 and classof $.baseobject == Frost) then
			::FrostSphereGizmoAutoAdd_System = $
		else
		(
			theFrosts = for o in objects where classof o.baseobject == Frost collect o
			if theFrosts.count > 0 do 
				::FrostSphereGizmoAutoAdd_System = theFrosts[theFrosts.count]
		)
		if isValidNode ::FrostSphereGizmoAutoAdd_System then
		(
			callbacks.addScript #nodeCreated "::FrostSphereGizmoAutoAdd_Function (callbacks.notificationParam())" id:#FrostSphereGizmoAutoAdd
			FrostUtils.LogProgress("AUTO-FROST: Enabled For " + (::FrostSphereGizmoAutoAdd_System.name as String))
			::FrostSphereGizmoAutoAdd_System.updateOnParticleChange = true
			::FrostSphereGizmoAutoAdd_System.useRadiusChannel = true
		)
		else
			FrostUtils.LogError "--Failed To Enable Auto-Frost.  Please create a Frost object first."
	)
)

)--end script

macroScript FrostGeo buttontext:"Geometry" tooltip:"Switch selected Frost objects to Geometry Meshing Mode" category:"Frost" icon:#("Frost",3)
(
	for o in selection where classof o.baseobject == Frost do o.meshingMethod = 1
)
macroScript FrostUOS buttontext:"Union Of Spheres" tooltip:"Switch selected Frost objects to Union Of Spheres Meshing Mode" category:"Frost" icon:#("Frost",4)
(
	for o in selection where classof o.baseobject == Frost do o.meshingMethod = 2
)
macroScript FrostMB buttontext:"Metaballs" tooltip:"Switch selected Frost objects to Metaballs Meshing Mode" category:"Frost" icon:#("Frost",5)
(
	for o in selection where classof o.baseobject == Frost do o.meshingMethod = 3
)
macroScript FrostZB buttontext:"Zhu/Bridson" tooltip:"Switch selected Frost objects to Zhu/Bridson Meshing Mode" category:"Frost" icon:#("Frost",6)
(
	for o in selection where classof o.baseobject == Frost do o.meshingMethod = 4
)
macroScript FrostVertexCloud buttontext:"VertexCloud" tooltip:"Switch selected Frost objects to Vertex Cloud Meshing Mode" category:"Frost" icon:#("Frost",7)
(
	for o in selection where classof o.baseobject == Frost do o.meshingMethod = 5
)
macroScript FrostAniso buttontext:"Anisotropic" tooltip:"Switch selected Frost objects to Anisotropic Meshing Mode" category:"Frost" icon:#("Frost",8)
(
	for o in selection where classof o.baseobject == Frost do o.meshingMethod = 6
)

macroScript FrostLogToggle buttontext:"Frost Log" tooltip:"Toggle Frost Log Window On And Off" category:"Frost" icon:#("Frost",9)
(
	on isChecked return try(FrostUtils.LogWindowVisible)catch()
	on execute do try(FrostUtils.LogWindowVisible = not FrostUtils.LogWindowVisible)catch()
)
macroScript FrostOpenLogOnError buttontext:"Frost Auto Log" tooltip:"Open Frost Log Window On Errors" category:"Frost" icon:#("Frost",10)
(
	on isChecked return try(FrostUtils.PopupLogWindowOnError)catch(false)
	on execute do try(FrostUtils.PopupLogWindowOnError = not FrostUtils.PopupLogWindowOnError)catch()
)

