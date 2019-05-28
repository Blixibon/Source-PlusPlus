// Created by Crowbar 0.58

$ModelName "weapons/cstrike/c_pist_deagle.mdl"

$BodyGroup "studio"
{
	studio "c_pist_deagle_deagle_mirrored_ref.smd"
}

$Include "c_pist_deagle_lod.qci"

$SurfaceProp "default"

$Contents "solid"

$IllumPosition -2.537 -17.877 -8.112

$CDMaterials "models\weapons\v_models\pist_deagle\"

$Attachment "1" "v_weapon.Deagle_Parent" 0 4 -7.87 rotate 90 0 0
$Attachment "2" "v_weapon.Deagle_Parent" 0 3.87 -0.67 rotate 0 160 0

$Include "c_pist_deagle_box.qci"

$Include "c_pist_deagle_bone.qci"

$Include "c_pist_deagle_animation.qci"
