// Created by Crowbar 0.63

$ModelName "weapons/tfa_bms/w_shotgun_bms.mdl"

$BodyGroup "Body"
{
	studio "w_shotgun_bms_reference.smd"
}

$Include "w_shotgun_bms_lod.qci"

$SurfaceProp "weapon"

$Contents "solid"

$IllumPosition 1.017 -14.678 0.62

$CDMaterials "bms\models\world_models\"

$Attachment "muzzle" "muzzle" 0 0 0 rotate 0 0 90

$Include "w_shotgun_bms_box.qci"

$Include "w_shotgun_bms_bone.qci"

$Include "w_shotgun_bms_animation.qci"

$Include "w_shotgun_bms_collision.qci"

$KeyValues
{
	prop_data 
	{
		"base" "metal.small"
	}
}
