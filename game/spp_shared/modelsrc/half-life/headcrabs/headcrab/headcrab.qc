// Created by Crowbar 0.58

$ModelName "half-life\hcf_headcrab.mdl"

$BodyGroup "studio"
{
	studio "headcrab_reference.smd"
}

$SurfaceProp "alienflesh"

$Contents "solid"

$IllumPosition -0.333 -1.766 8.164

$EyePosition 0 0 20

$CDMaterials "models\half-life\headcrab"

$MostlyOpaque

$TextureGroup "skinfamilies"
{
	{ "lowerside"  "bottom"  "front"  "butt"  "transparent section"  }
	{ "lowerside2" "bottom2" "front2" "butt2" "transparent section2" }
	{ "lowerside3" "bottom3" "front3" "butt3" "transparent section3" }
	{ "lowerside4" "bottom4" "front4" "butt4" "transparent section4" }
	{ "lowerside5" "bottom5" "front5" "butt5" "transparent section5" }
	{ "lowerside6" "bottom6" "front6" "butt6" "transparent section6" }
}


$Attachment "0" "Bone06" 0 1 -1
$Attachment "1" "Bone06" -0.5 -5 -4.75
$Attachment "2" "Bone08" 0 1 -1
$Attachment "3" "Bone08" 0.5 -5 -4.75

$pushd ".."

$Include "..\headcrab_box.qci"

$Include "..\headcrab_bone.qci"

$Include "..\headcrab_animation.qci"

$Include "..\headcrab_collision.qci"

$popd
