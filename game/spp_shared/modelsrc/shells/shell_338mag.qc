// Created by Crowbar 0.58

$ModelName "shells\shell_338mag.mdl"

$BodyGroup "studio"
{
	studio "shell_338mag_338mag_reference.smd"
}

$Include "shell_338mag_lod.qci"

$SurfaceProp "shell"

$Contents "solid"

$IllumPosition 0 -1.254 0

$CDMaterials "models\shells\338mag\"

$Include "shell_338mag_box.qci"

$Include "shell_338mag_bone.qci"

$Include "shell_338mag_animation.qci"

$Include "shell_338mag_collision.qci"

$keyvalues
{
	prop_data
	{
		base			Metal.Small 
		physicsmode 3
	}
}