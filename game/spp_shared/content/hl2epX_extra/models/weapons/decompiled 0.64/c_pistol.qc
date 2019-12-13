// Created by Crowbar 0.64

$ModelName "weapons/c_pistol.mdl"

$BodyGroup "studio"
{
	studio "v_pistol_Pistol_reference.smd"
}

$Include "v_pistol_lod.qci"

$SurfaceProp "default"

$Contents "solid"

$IllumPosition -2.641 -8.852 -9.508

$CDMaterials "models\weapons\v_hand\"
$CDMaterials "models\Weapons\v_pistol\"

$Attachment "muzzle" "usp" -6.7 0.6 -0.03 rotate 0 180 90
$Attachment "1" "usp" -1 0.75 -0.05 rotate 25 90 25

$Include "v_pistol_box.qci"

$Include "v_pistol_bone.qci"

$Include "v_pistol_animation.qci"
