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

$Opaque

// This list shows the VMT file names used in the SMD files.
// "barney_hands_wht.vmt"
// "barney_tie.vmt"
// "barney_body.vmt"
// "barney_legs.vmt"
// "barney_head_helmet_wht.vmt"
// "barney_helmet_Chrome.vmt"
// "barney_helmet.vmt"
// "barney_id_helmet_wht.vmt"
// "wp_beretta.vmt"
// "wp_beretta_Chrome.vmt"

$Attachment "gun" "Bip01 R Hand" 15 3.5 5 rotate 0 0 0
$Attachment "eyes" "Bip01 Head" 0 0 0 rotate 0 90 90
$Attachment "forward" "Bip01 Spine" 0 0 0 rotate 0 90 90

$Include "barney_box.qci"

$Include "barney_bone.qci"

$Include "barney_animation.qci"

$Include "barney_collision.qci"
