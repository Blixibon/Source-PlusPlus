// Created by Crowbar 0.58

$ModelName "Weapons/v_crowbar.mdl"

$BodyGroup "studio"
{
	studio "v_crowbar.smd"
}
$BodyGroup "hands"
{
	studio "v_crowbar_RefHEVHands.smd"
}

$Include "v_crowbar_lod.qci"

$SurfaceProp "default"

$Contents "solid"

$IllumPosition -8.647 -25.37 -15.377

$CDMaterials "models\Weapons\V_hand\"
$CDMaterials "models\blueflytrap\crowbar\"

$Attachment "muzzle" "cbar" 11 5 0 rotate 0 90 90

$Include "v_crowbar_box.qci"

$Include "v_crowbar_bone.qci"

$Include "v_crowbar_animation.qci"
