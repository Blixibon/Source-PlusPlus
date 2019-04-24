// Created by Crowbar 0.56

$ModelName "half-life\barney.mdl"

$BodyGroup "body"
{
	studio "barney_body_armour_ref.smd"
}
$BodyGroup "head"
{
	studio "barney_head_helmet_ref.smd"
}
$BodyGroup "gun"
{
	studio "barney_beretta_holstered_ref.smd"
	studio "barney_beretta_unholstered_ref.smd"
	blank
}

$Include "barney_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition -0.894 -2.124 37.974

$EyePosition 0 0 63

$CDMaterials "models\half-life\barney\"

$Attachment "gun" "Bip01 R Hand" 15 3.5 5 rotate 0 0 0
$Attachment "neck" "Bip01 Head" 0 0 0 rotate 0 90 90
$Attachment "forward" "Bip01 Spine" 0 0 0 rotate 0 90 90
$attachment "gutblood" "Bip01 Spine" 4.00 7.00 0.00 rotate -0 90 90
$attachment "chest" "Bip01 Spine1" 4.00 6.80 0.00 rotate -0 90 90
$attachment "eyes" "Bip01 Head" 4.50 5.50 -0.00 rotate -0 90 90
$attachment "mouth" "Mouth" 3.50 0.50 0.00 rotate -0 0 0

$Include "barney_box.qci"

$Include "barney_bone.qci"

$Include "barney_animation.qci"

$Include "barney_collision.qci"
