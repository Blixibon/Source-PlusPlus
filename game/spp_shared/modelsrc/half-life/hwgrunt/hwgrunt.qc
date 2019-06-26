// Created by Crowbar 0.58

$ModelName "half-life/hwgrunt.mdl"

$BodyGroup "body"
{
	studio "hwgrunt_mouth.smd"
}
$BodyGroup "weapons"
{
	studio "hwgrunt_weapon_minigun.smd"
	studio "hwgrunt_weapon_beretta.smd"
	studio "hwgrunt_weapon_deagle.smd"
	studio "hwgrunt_weapon_python.smd"
	blank
}


$EyePosition 0 0 70

$CDMaterials "models/half-life/hwgrunt"

$attachment "muzzle" "b_fucked" 35 0 0 rotate 0 -0 0
$Attachment "gun" "b_fucked" 2 -3 -3
$Attachment "eyes" "Bip01 Head" 2.9 6 -1 rotate 0 0 0
$attachment "muzzle1" "Bip01 R Hand" 12.00 -2.00 0.50 rotate -0 0 90
$attachment "muzzle2" "Bip01 R Hand" 14.00 -1.60 0.50 rotate -0 0 90
$attachment "muzzle3" "Bip01 R Hand" 16.00 -3.00 0.50 rotate -0 0 90

$Include "hwgrunt_animation.qci"
$Include "hwgrunt_box.qci"