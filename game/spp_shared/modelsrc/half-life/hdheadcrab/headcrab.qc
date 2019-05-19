// Created by Crowbar 0.58

$ModelName "half-life\headcrab.mdl"

$BodyGroup "studio"
{
	studio "headcrab_reference.smd"
}

$Include "headcrab_lod.qci"

$SurfaceProp "alienflesh"

$Contents "solid"

$IllumPosition 1.765 0.001 7.865

$EyePosition 0 0 20

$CDMaterials "models\half-life\headcrab\"

$Attachment "possession_cam" "Bip01" 1 0 4 rotate 0 0 0

$Include "headcrab_box.qci"

$Include "headcrab_bone.qci"

$Include "headcrab_animation.qci"

$Include "headcrab_collision.qci"
