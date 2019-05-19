// Created by Crowbar 0.58

$ModelName "opfor\zombie_soldier.mdl"

$BodyGroup "studio"
{
	studio "zombie_soldier.smd"
}

$Include "zombie_soldier_lod.qci"

$SurfaceProp "zombieflesh"

$Contents "solid"

$IllumPosition 9.537 2.206 37.714

$EyePosition 0 0 55

$CDMaterials "models\opfor\zombie_soldier\"

$Attachment "possession_cam" "Bip01" 0 0 30 rotate 0 0 0

$Include "zombie_soldier_box.qci"

$Include "zombie_soldier_bone.qci"

$Include "zombie_soldier_animation.qci"

$Include "zombie_soldier_collision.qci"
