-- Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
-- SPDX-License-Identifier: Apache-2.0
plugin simpleManipulator FrostROIManip
	name:"FrostROIManip"
	invisible:true
(

	local selectedColor = colorMan.getColor #manipulatorsSelected
	local activeColor = colorMan.getColor #manipulatorsActive
	local redColor = color 0.75 0 0
	local greenColor =  color 0 0.75 0
	local blueColor =  color 0 0 0.75

	on canManipulate target return (classof target == Frost)

	on updateGizmos do
	(
		this.clearGizmos()
		local theNode = (refs.dependentNodes target)[1]

		if not (theNode.enableRenderROI or theNode.enableViewportROI) do
		(
			return()
		)

		local theTM = inverse theNode.objectTransform

		local bboxMin = [target.ROICenterX-target.ROISizeX/2, target.ROICenterY - target.ROISizeY/2, target.ROICenterZ - target.ROISizeZ/2 ]
		local bboxMax = [target.ROICenterX+target.ROISizeX/2, target.ROICenterY + target.ROISizeY/2, target.ROICenterZ + target.ROISizeZ/2 ]


		local size = length ( bboxMax  - bboxMin)/50
		local minsize = 0.1
		local maxsize = 20.0
		if size < minsize do size = minsize
		if size > maxsize do size = maxsize

		giz0 = manip.MakeSphere ([bboxMin.x,target.ROICenterY,target.ROICenterZ]*theTM) size 16
		giz1 = manip.MakeSphere ([bboxMax.x,target.ROICenterY,target.ROICenterZ]*theTM) size 16

		giz2 = manip.MakeSphere ([target.ROICenterX,bboxMin.y,target.ROICenterZ]*theTM) size 16
		giz3 = manip.MakeSphere ([target.ROICenterX,bboxMax.y,target.ROICenterZ]*theTM) size 16

		giz4 = manip.MakeSphere ([target.ROICenterX,target.ROICenterY,bboxMin.z]*theTM) size 16
		giz5 = manip.MakeSphere ([target.ROICenterX,target.ROICenterY,bboxMax.z]*theTM) size 16

		this.addGizmoMesh giz0 0 redColor selectedColor
		this.addGizmoMesh giz1 0 redColor selectedColor
		this.addGizmoMesh giz2 0 greenColor selectedColor
		this.addGizmoMesh giz3 0 greenColor selectedColor
		this.addGizmoMesh giz4 0 blueColor selectedColor
		this.addGizmoMesh giz5 0 blueColor selectedColor

		this.addGizmoMarker #asterisk  ([target.ROICenterX,target.ROICenterY,target.ROICenterZ]*theTM)  0 activeColor selectedColor

		this.addGizmoMarker #xMarker  ([bboxMax.x+size,bboxMax.y+size,bboxMax.z+size]*theTM)  0 activeColor selectedColor

		--move whole box one size along -X, gizmo #8
		gizShapeXmin = manip.makeGizmoShape()
		gizShapeXmin.addPoint ([bboxMin.x,bboxMin.y,bboxMin.z]*theTM)
		gizShapeXmin.addPoint ([bboxMin.x-size*5,target.ROICenterY,target.ROICenterZ]*theTM)
		gizShapeXmin.addPoint ([bboxMin.x,bboxMin.y,bboxMax.z]*theTM)
		gizShapeXmin.addPoint ([bboxMin.x-size*5,target.ROICenterY,target.ROICenterZ]*theTM)
		gizShapeXmin.addPoint ([bboxMin.x,bboxMax.y,bboxMax.z]*theTM)
		gizShapeXmin.addPoint ([bboxMin.x-size*5,target.ROICenterY,target.ROICenterZ]*theTM)
		gizShapeXmin.addPoint ([bboxMin.x,bboxMax.y,bboxMin.z]*theTM)
		this.addGizmoShape gizShapeXmin 0 redColor redColor

		--move whole box one size along +X, gizmo #9
		gizShapeXmax = manip.makeGizmoShape()
		gizShapeXmax.addPoint ([bboxMax.x,bboxMin.y,bboxMin.z]*theTM)
		gizShapeXmax.addPoint ([bboxMax.x+size*5,target.ROICenterY,target.ROICenterZ]*theTM)
		gizShapeXmax.addPoint ([bboxMax.x,bboxMin.y,bboxMax.z]*theTM)
		gizShapeXmax.addPoint ([bboxMax.x+size*5,target.ROICenterY,target.ROICenterZ]*theTM)
		gizShapeXmax.addPoint ([bboxMax.x,bboxMax.y,bboxMax.z]*theTM)
		gizShapeXmax.addPoint ([bboxMax.x+size*5,target.ROICenterY,target.ROICenterZ]*theTM)
		gizShapeXmax.addPoint ([bboxMax.x,bboxMax.y,bboxMin.z]*theTM)
		this.addGizmoShape gizShapeXmax 0 redColor redColor

		--move whole box one size along -Y, gizmo #10
		gizShapeYmin = manip.makeGizmoShape()
		gizShapeYmin.addPoint ([bboxMin.x,bboxMin.y,bboxMin.z]*theTM)
		gizShapeYmin.addPoint ([target.ROICenterX,bboxMin.y-size*5,target.ROICenterZ]*theTM)
		gizShapeYmin.addPoint ([bboxMin.x,bboxMin.y,bboxMax.z]*theTM)
		gizShapeYmin.addPoint ([target.ROICenterX,bboxMin.y-size*5,target.ROICenterZ]*theTM)
		gizShapeYmin.addPoint ([bboxMax.x,bboxMin.y,bboxMax.z]*theTM)
		gizShapeYmin.addPoint ([target.ROICenterX,bboxMin.y-size*5,target.ROICenterZ]*theTM)
		gizShapeYmin.addPoint ([bboxMax.x,bboxMin.y,bboxMin.z]*theTM)
		this.addGizmoShape gizShapeYmin 0 greenColor greenColor

		--move whole box one size along +Y, gizmo #11
		gizShapeYmax = manip.makeGizmoShape()
		gizShapeYmax.addPoint ([bboxMin.x,bboxMax.y,bboxMin.z]*theTM)
		gizShapeYmax.addPoint ([target.ROICenterX,bboxMax.y+size*5,target.ROICenterZ]*theTM)
		gizShapeYmax.addPoint ([bboxMin.x,bboxMax.y,bboxMax.z]*theTM)
		gizShapeYmax.addPoint ([target.ROICenterX,bboxMax.y+size*5,target.ROICenterZ]*theTM)
		gizShapeYmax.addPoint ([bboxMax.x,bboxMax.y,bboxMax.z]*theTM)
		gizShapeYmax.addPoint ([target.ROICenterX,bboxMax.y+size*5,target.ROICenterZ]*theTM)
		gizShapeYmax.addPoint ([bboxMax.x,bboxMax.y,bboxMin.z]*theTM)
		this.addGizmoShape gizShapeYmax 0 greenColor greenColor

		--move whole box one size along -Z, gizmo #12
		gizShapeZmin = manip.makeGizmoShape()
		gizShapeZmin.addPoint ([bboxMin.x,bboxMin.y,bboxMin.z]*theTM)
		gizShapeZmin.addPoint ([target.ROICenterX,target.ROICenterY,bboxMin.z-size*5]*theTM)
		gizShapeZmin.addPoint ([bboxMin.x,bboxMax.y,bboxMin.z]*theTM)
		gizShapeZmin.addPoint ([target.ROICenterX,target.ROICenterY,bboxMin.z-size*5]*theTM)
		gizShapeZmin.addPoint ([bboxMax.x,bboxMin.y,bboxMin.z]*theTM)
		gizShapeZmin.addPoint ([target.ROICenterX,target.ROICenterY,bboxMin.z-size*5]*theTM)
		gizShapeZmin.addPoint ([bboxMax.x,bboxMax.y,bboxMin.z]*theTM)
		this.addGizmoShape gizShapeZmin 0 blueColor blueColor

		--move whole box one size along +Z gizmo #13
		gizShapeZmax = manip.makeGizmoShape()
		gizShapeZmax.addPoint ([bboxMin.x,bboxMin.y,bboxMax.z]*theTM)
		gizShapeZmax.addPoint ([target.ROICenterX,target.ROICenterY,bboxMax.z+size*5]*theTM)
		gizShapeZmax.addPoint ([bboxMin.x,bboxMax.y,bboxMax.z]*theTM)
		gizShapeZmax.addPoint ([target.ROICenterX,target.ROICenterY,bboxMax.z+size*5]*theTM)
		gizShapeZmax.addPoint ([bboxMax.x,bboxMax.y,bboxMax.z]*theTM)
		gizShapeZmax.addPoint ([target.ROICenterX,target.ROICenterY,bboxMax.z+size*5]*theTM)
		gizShapeZmax.addPoint ([bboxMax.x,bboxMin.y,bboxMax.z]*theTM)
		this.addGizmoShape gizShapeZmax 0 blueColor blueColor


		gizShape = manip.makeGizmoShape()
		gizShape.addPoint ([bboxMin.x,bboxMin.y,bboxMin.z]*theTM)
		gizShape.addPoint ([bboxMax.x,bboxMin.y,bboxMin.z]*theTM)
		gizShape.addPoint ([bboxMax.x,bboxMax.y,bboxMin.z]*theTM)
		gizShape.addPoint ([bboxMin.x,bboxMax.y,bboxMin.z]*theTM)
		gizShape.addPoint ([bboxMin.x,bboxMin.y,bboxMin.z]*theTM)
		gizShape.addPoint ([bboxMin.x,bboxMin.y,bboxMax.z]*theTM)
		gizShape.addPoint ([bboxMax.x,bboxMin.y,bboxMax.z]*theTM)
		gizShape.addPoint ([bboxMax.x,bboxMax.y,bboxMax.z]*theTM)
		gizShape.addPoint ([bboxMin.x,bboxMax.y,bboxMax.z]*theTM)
		gizShape.addPoint ([bboxMin.x,bboxMin.y,bboxMax.z]*theTM)
		gizShape.addPoint ([bboxMin.x,bboxMin.y,bboxMin.z]*theTM)
		gizShape.addPoint ([bboxMax.x,bboxMin.y,bboxMin.z]*theTM)
		gizShape.addPoint ([bboxMax.x,bboxMin.y,bboxMax.z]*theTM)
		gizShape.addPoint ([bboxMax.x,bboxMax.y,bboxMax.z]*theTM)
		gizShape.addPoint ([bboxMax.x,bboxMax.y,bboxMin.z]*theTM)
		gizShape.addPoint ([bboxMin.x,bboxMax.y,bboxMin.z]*theTM)
		gizShape.addPoint ([bboxMin.x,bboxMax.y,bboxMax.z]*theTM)

		this.addGizmoShape gizShape gizmoDontHitTest activeColor activeColor

		return node.name + " Center:" + [target.ROICenterX,target.ROICenterY,target.ROICenterZ] as string  + " Size:" + [target.ROISizeX,target.ROISizeY,target.ROISizeZ] as string
	)

	on mouseDown m which do
	(
			case which of
			(
				default: ()
				7 : (
						target.ROISizeX = target.ROISizeY = target.ROISizeZ = target.iconSize
						target.roiCenterX = target.roiCenterY = target.roiCenterZ = 0
					)
				8 : (
						target.roiCenterX -= target.ROISizeX
					)
				9 : (
						target.roiCenterX += target.ROISizeX
					)
				10 : (
						target.roiCenterY -= target.ROISizeY
					)
				11 : (
						target.roiCenterY += target.ROISizeY
					)
				12 : (
						target.roiCenterZ -= target.ROISizeZ
					)
				13 : (
						target.roiCenterZ += target.ROISizeZ
					)
			)
	)

	on mouseMove m which do
	(
		if which > 6 do return false

		local theNode = (refs.dependentNodes target)[1]
		local theTM = theNode.objectTransform
		local thePoint = [target.roiCenterX,target.roiCenterY,target.roiCenterZ]
		thePoint *= (inverse theTM)

		local planes = #(
			manip.makePlaneFromNormal x_axis thePoint,
			manip.makePlaneFromNormal y_axis thePoint,
			manip.makePlaneFromNormal z_axis thePoint)

		local viewRay = this.getLocalViewRay m

		local pl = planes[1]
		if which < 6 then
		(
			local axis = which / 2
			local axis1 = ((mod (axis + 1) 3) as Integer) + 1
			local axis2 = ((mod (axis + 2) 3) as Integer) + 1
			pl = planes[axis1].mostOrthogonal viewRay planes[axis2]
		)
		else
		(
			local pl1 = planes[2].mostOrthogonal viewRay planes[3]
			pl = pl1.mostOrthogonal viewRay planes[1]
		)

		local projectedPoint = [0,0,0]
		local res = pl.intersect viewRay &projectedPoint
		projectedPoint *= theTM
		if res do
		(
			case which of
			(
				default: ()
				0: (
						if keyboard.controlPressed then
						(
							target.ROISizeX = (target.roiCenterX-projectedPoint.x )*2
							if target.ROISizeX  < 0.0 do target.ROISizeX = 1.0
							if keyboard.shiftPressed  do
								target.ROISizeZ = target.ROISizeY = target.ROISizeX
						)
						else
						(
							if keyboard.shiftPressed  then
							(
								target.roiCenterX = projectedPoint.x + target.ROISizeX/2
							)
							else
							(
								local oldSide = target.roiCenterX+(target.ROISizeX*0.5)
								target.ROISizeX = (oldSide - projectedPoint.x)
								if target.ROISizeX  < 0.0 do target.ROISizeX = 1.0
								target.roiCenterX = oldSide - (target.ROISizeX*0.5)
							)
						)
				)
				1: (
						if keyboard.controlPressed then
						(
							target.ROISizeX = (projectedPoint.x-target.roiCenterX )*2
							if target.ROISizeX  < 0.0 do target.ROISizeX = 1.0
							if keyboard.shiftPressed  do
								target.ROISizeZ = target.ROISizeY = target.ROISizeX
						)
						else
						(
							if keyboard.shiftPressed  then
							(
								target.roiCenterX = projectedPoint.x - target.ROISizeX/2
							)
							else
							(
								local oldSide = target.roiCenterX-(target.ROISizeX*0.5)
								target.ROISizeX = (projectedPoint.x-oldSide)
								if target.ROISizeX  < 0.0 do target.ROISizeX = 1.0
								target.roiCenterX = oldSide + (target.ROISizeX*0.5)
							)
						)
				)
				2: (

						if keyboard.controlPressed then
						(
							target.ROISizeY = (target.roiCenterY-projectedPoint.y )*2
							if target.ROISizeY  < 0.0 do target.ROISizeY = 1.0
							if keyboard.shiftPressed  do
								target.ROISizeZ  = target.ROISizeX = target.ROISizeY
						)
						else
						(
							if keyboard.shiftPressed  then
							(
								target.roiCenterY = projectedPoint.y + target.ROISizeY/2
							)
							else
							(
								local oldSide = target.roiCenterY+(target.ROISizeY*0.5)
								target.ROISizeY = (oldSide - projectedPoint.y)
								if target.ROISizeY  < 0.0 do target.ROISizeY = 1.0
								target.roiCenterY = oldSide - (target.ROISizeY*0.5)
							)
						)
				)
				3: (
						if keyboard.controlPressed then
						(
							target.ROISizeY = (projectedPoint.y-target.roiCenterY )*2
							if target.ROISizeY  < 0.0 do target.ROISizeY = 1.0
							if keyboard.shiftPressed  do
								target.ROISizeZ = target.ROISizeX = target.ROISizeY
						)
						else
						(
							if keyboard.shiftPressed  then
							(
								target.roiCenterY = projectedPoint.y - target.ROISizeY/2
							)
							else
							(
								local oldSide = target.roiCenterY-(target.ROISizeY*0.5)
								target.ROISizeY = (projectedPoint.y-oldSide)
								if target.ROISizeY  < 0.0 do target.ROISizeY = 1.0
								target.roiCenterY = oldSide + (target.ROISizeY*0.5)
							)
						)
				)
				4: (
						if keyboard.controlPressed then
						(
							target.ROISizeZ = (target.roiCenterY-projectedPoint.z )*2
							if target.ROISizeZ  < 0.0 do target.ROISizeZ = 1.0
							if keyboard.shiftPressed  do
								target.ROISizeX = target.ROISizeY = target.ROISizeZ
						)
						else
						(
							if keyboard.shiftPressed  then
							(
								target.roiCenterZ = projectedPoint.z + target.ROISizeZ/2
							)
							else
							(
								local oldSide = target.roiCenterZ+(target.ROISizeZ*0.5)
								target.ROISizeZ = (oldSide - projectedPoint.z)
								if target.ROISizeZ  < 0.0 do target.ROISizeZ = 1.0
								target.roiCenterZ = oldSide - (target.ROISizeZ*0.5)
							)
						)
				)
				5: (
						if keyboard.controlPressed then
						(
							target.ROISizeZ = (projectedPoint.z-target.roiCenterZ )*2
							if target.ROISizeZ  < 0.0 do target.ROISizeZ = 1.0
							if keyboard.shiftPressed  do
								target.ROISizeX = target.ROISizeY =target.ROISizeZ
						)
						else
						(
							if keyboard.shiftPressed  then
							(
								target.roiCenterZ = projectedPoint.z - target.ROISizeZ/2
							)
							else
							(
								local oldSide = target.roiCenterZ-(target.ROISizeZ*0.5)
								target.ROISizeZ = (projectedPoint.z-oldSide)
								if target.ROISizeZ  < 0.0 do target.ROISizeZ = 1.0
								target.roiCenterZ = oldSide + (target.ROISizeZ*0.5)
							)
						)
				)
				6: (
					target.roiCenterX = projectedPoint.x
					target.roiCenterY = projectedPoint.y
					target.roiCenterZ = projectedPoint.z
				)
			)--end case
		)--end if res
	)
)
