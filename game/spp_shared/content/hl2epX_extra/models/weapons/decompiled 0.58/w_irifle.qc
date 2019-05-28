// Created by Crowbar 0.58

$ModelName "weapons/w_irifle.mdl"

$BodyGroup "studio"
{
	studio "w_irifle_reference.smd"
}
$BodyGroup "magazine"
{
	studio "w_irifle_magazine.smd"
	blank
}

$Include "w_irifle_lod.qci"

$SurfaceProp "weapon"

$Contents "solid"

$IllumPosition -0.448 -4.137 1.987

$MostlyOpaque

$CDMaterials "models\weapons\w_Irifle\"

$Attachment "muzzle" "ValveBiped.muzzle" -8.5 0 0 rotate 0 0 -90
$Attachment "1" "ValveBiped.w_shotgun" 1 -7 3 rotate 0 0 0

$Include "w_irifle_box.qci"

$Include "w_irifle_bone.qci"

$Include "w_irifle_animation.qci"

$Include "w_irifle_collision.qci"
