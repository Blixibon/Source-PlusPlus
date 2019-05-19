// Created by Crowbar 0.58

$ModelName "half-life\zombie.mdl"

$BodyGroup "studio"
{
	studio "zombie_blue_zom.smd"
}

$Include "zombie_lod.qci"

$SurfaceProp "zombieflesh"

$Contents "solid"

$IllumPosition -0.04 18.749 33.409

$EyePosition 0 0 55

$CDMaterials "models\half-life\zombie\"

$Attachment "possession_cam" "Bip01" 0 0 30 rotate 0 0 0

$Include "zombie_box.qci"

$Include "zombie_bone.qci"

$Include "zombie_animation.qci"

$Include "zombie_collision.qci"
