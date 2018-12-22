// Created by Crowbar 0.56

$ModelName "gascit4.mdl"

$BodyGroup "gas mask citizen"
{
	studio "gascit4_male_02_reference3.smd"
}

$Include "gascit4_lod.qci"

$SurfaceProp "flesh"

$Contents "solid"

$IllumPosition -0.637 0 35.65

$EyePosition 0 0 70

$CDMaterials "models\Citizen_17\"
$CDMaterials "models\Humans\Male\Group01\"
$CDMaterials "models\Humans\Male\"

$Attachment "eyes" "ValveBiped.Bip01_Head1" 3.45 -3.5 0.05 rotate 0 -80.1 -90
$Attachment "mouth" "ValveBiped.Bip01_Head1" 0.8 -5.5 0.1 rotate 0 -80 -90
$Attachment "chest" "ValveBiped.Bip01_Spine2" 5 4 0 rotate 0 90 90
$Attachment "forward" "ValveBiped.forward" 0 0 0 rotate 0 0 0
$Attachment "anim_attachment_RH" "ValveBiped.Anim_Attachment_RH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_LH" "ValveBiped.Anim_Attachment_LH" 0 0 0 rotate -90 -90 0
$Attachment "anim_attachment_head" "ValveBiped.Bip01_Head1" 0 0 0 rotate -90 -90 0

// $Include "gascit4_box.qci"

$Include "gascit4_bone.qci"

$Include "gascit4_animation.qci"

$Include "gascit4_collision.qci"
