// Created by Crowbar 0.63

$ModelName "weapons/tfa_bms/w_glock_bms.mdl"

$BodyGroup "Body"
{
	studio "w_glock_bms_w_glock_reference.smd"
}

$Include "w_glock_bms_lod.qci"

$SurfaceProp "weapon"

$Contents "solid"

$IllumPosition 0.396 -0.531 -2.777

$CDMaterials "bms\models\weapons\v_glock\"

$Attachment "muzzle" "muzzle" 0 0 0 rotate 0 0 0
$Attachment "2" "muzzle" -5 0 0 rotate -90 0 0
$Attachment "3" "muzzle" 1 0 0 rotate 0 0 0

$Include "w_glock_bms_box.qci"

$Include "w_glock_bms_bone.qci"

$Include "w_glock_bms_animation.qci"

$Include "w_glock_bms_collision.qci"

$KeyValues
{
	prop_data 
	{
		"base" "metal.small"
	}
}
