// Created by Crowbar 0.56

$ModelName "opfor\otis.mdl"

$BodyGroup "body"
{
	studio "otis_body_ref.smd"
}
$BodyGroup "gun"
{
	studio "otis_gun_holstered_ref.smd"
	studio "otis_gun_unholstered_ref.smd"
	blank
	studio "otis_donut_ref.smd"
}
$BodyGroup "heads"
{
	studio "otis_head_1_ref.smd"
	studio "otis_head_2_ref.smd"
}

$Include "otis_lod.qci"

$Opaque

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition -0.706 -0.835 36.868

$EyePosition 0 0 63

$CDMaterials "models\opfor\otis\"

$Attachment "gun" "Bip01 R Hand" 11 2.8 4 rotate 0 0 0
$Attachment "forward" "Bip01 Spine" 0 0 0 rotate 0 90 90
$attachment "chest" "Bip01 Spine1" 4.00 6.80 0.00 rotate -0 90 90
$attachment "eyes" "Bip01 Head" 4.50 5.50 -0.00 rotate -0 90 90
$attachment "mouth" "bone05" 3.50 0.50 0.00 rotate -0 0 0

$Include "otis_box.qci"

$Include "otis_bone.qci"

$Include "otis_animation.qci"

$Include "otis_collision.qci"
