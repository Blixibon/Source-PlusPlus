// Created by Crowbar 0.56

$ModelName "weapons/w_pistol.mdl"

$BodyGroup "w_pistol"
{
	studio "w_pistol_reference.smd"
}
$BodyGroup "slide"
{
	studio "w_pistol_slide.smd"
	studio "w_pistol_slideback.smd"
}
$BodyGroup "magazine"
{
	studio "w_pistol_magazine.smd"
	blank
}

$Include "w_pistol_lod.qci"

$SurfaceProp "weapon"

$Contents "solid"

$IllumPosition 0.458 0.02 0.439

$MostlyOpaque

$CDMaterials "models\weapons\w_pistol\"

$Attachment "muzzle" "ValveBiped.Weapon_bone" 6.31 -0.68 0.42 rotate 4.49 -1.17 -98.82
$Attachment "1" "ValveBiped.Weapon_bone" 1.06 0.18 0.72 rotate 8.79 89.53 -85.45

// $Include "w_pistol_box.qci"

$Include "w_pistol_bone.qci"

$Include "w_pistol_animation.qci"

$Include "w_pistol_collision.qci"
