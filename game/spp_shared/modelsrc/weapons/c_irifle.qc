// Created by Crowbar 0.58

$ModelName "weapons/c_irifle.mdl"

$BodyGroup "studio"
{
	studio "c_irifle_v_irifle_IRifle_reference.smd"
}
$BodyGroup "core_in_barrel"
{
	studio "c_irifle_v_irifle_IRifle_reference_core1.smd"
	blank
}
$BodyGroup "core_in_mag"
{
	studio "c_irifle_v_irifle_IRifle_reference_core2.smd"
	blank
}
$BodyGroup "core_in_mag_clone"
{
	studio "c_irifle_v_irifle_IRifle_reference_core2_2.smd"
	blank
}

$Include "c_irifle_lod.qci"

$SurfaceProp "default"

$Contents "solid"

$IllumPosition -1.736 -11.656 -9.388

$CDMaterials "models\Weapons\V_hand\"
$CDMaterials "models\Weapons\V_irifle\"

$TextureGroup "skinfamilies"
{
	{ "V_IRifle"          "V_IRifle_core"          }
	{ "V_IRifle_charging" "v_irifle_core_charging" }
}

$Attachment "muzzle" "ar2_weapon" 25 -1 0 rotate 0 0 90
$Attachment "1" "ar2_weapon" 11.3 -1 0 rotate 65 180 -40
$Attachment "secondary" "ar2_weapon" 22 2.5 -0.2 rotate 0 0 90
$Attachment "punch" "ar2_weapon" 11.9 -1.2 0.25 rotate 0 0 90

$Include "c_irifle_box.qci"

$Include "c_irifle_bone.qci"

$Include "c_irifle_animation.qci"
