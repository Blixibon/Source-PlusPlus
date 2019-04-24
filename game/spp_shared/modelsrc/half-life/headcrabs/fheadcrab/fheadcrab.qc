// Created by Crowbar 0.58

$ModelName "half-life\hcf_fheadcrab.mdl"

$BodyGroup "studio"
{
	studio "fheadcrab_reference"
}

$SurfaceProp "alienflesh"

$Contents "solid"

$IllumPosition -0.333 -1.766 8.164

$EyePosition 0 0 20

$CDMaterials "models\half-life\fheadcrab"

$pushd ".."

$Include "..\headcrab_box.qci"

$Include "..\headcrab_bone.qci"

$popd

$CollisionJoints "headcrab_physics.smd"
{
	$mass 30
	$inertia 5
	$damping 0.01
	$rotdamping 1.5
	$rootbone "bip01"


	$jointconstrain "Bip01 Spine" x limit 0 0 0
	$jointconstrain "Bip01 Spine" y limit 0 0 0
	$jointconstrain "Bip01 Spine" z limit 0 0 0

	$jointconstrain "Bone05" x limit -60 60 0.2
	$jointconstrain "Bone05" y limit -30 30 0.2
	$jointconstrain "Bone05" z limit -70 40 0.2

	$jointconstrain "Bone06" x limit -20 70 0.2
	$jointconstrain "Bone06" y limit -30 30 0.2
	$jointconstrain "Bone06" z limit -10 45 0.2

	$jointconstrain "Bone03" x limit -60 60 0.2
	$jointconstrain "Bone03" y limit -30 30 0.2
	$jointconstrain "Bone03" z limit -40 70 0.2

	$jointconstrain "Bip01 R Leg" x limit -20 20 0
	$jointconstrain "Bip01 R Leg" y limit -25 15 0
	$jointconstrain "Bip01 R Leg" z limit -30 40 0.2

	$jointconstrain "Bip01 R Leg1" x limit 0 0 0
	$jointconstrain "Bip01 R Leg1" y limit 0 0 0
	$jointconstrain "Bip01 R Leg1" z limit -90 20 0.2

	$jointconstrain "Bip01 R Foot" x limit 0 0 0
	$jointconstrain "Bip01 R Foot" y limit 0 0 0
	$jointconstrain "Bip01 R Foot" z limit -50 15 0.2

	$jointconstrain "Bip01 L Leg" x limit -20 20 0
	$jointconstrain "Bip01 L Leg" y limit -15 25 0
	$jointconstrain "Bip01 L Leg" z limit -30 40 0.2

	$jointconstrain "Bip01 L Leg1" x limit 0 0 0
	$jointconstrain "Bip01 L Leg1" y limit 0 0 0
	$jointconstrain "Bip01 L Leg1" z limit -90 20 0.2

	$jointconstrain "Bip01 L Foot" x limit 0 0 0
	$jointconstrain "Bip01 L Foot" y limit 0 0 0
	$jointconstrain "Bip01 L Foot" z limit -50 15 0.2

	$jointconstrain "Bone08" x limit -20 70 0.2
	$jointconstrain "Bone08" y limit -30 30 0.2
	$jointconstrain "Bone08" z limit -45 10 0.2
}

$Sequence "idle1" {
	"fheadcrab_anims\idle1.smd"
	activity "ACT_IDLE" 15
	fadein 0.2
	fadeout 0.2
	fps 20
	loop
}

$Sequence "idle2" {
	"fheadcrab_anims\idle2.smd"
	activity "ACT_IDLE" 1
	fadein 0.2
	fadeout 0.2
	fps 20
}

$Sequence "idle3" {
	"fheadcrab_anims\idle3.smd"
	activity "ACT_IDLE" 1
	fadein 0.2
	fadeout 0.2
	fps 20
}

$Sequence "walk" {
	"fheadcrab_anims\walk.smd"
	activity "ACT_WALK" 1
	fadein 0.2
	fadeout 0.2
	fps 40
	loop
	walkframe 34 LX
}

$Sequence "run" {
	"fheadcrab_anims\run.smd"
	activity "ACT_RUN" 1
	fadein 0.2
	fadeout 0.2
	fps 50
	loop
	walkframe 28 LX
}

$Sequence "angry" {
	"fheadcrab_anims\angry.smd"
	fadein 0.2
	fadeout 0.2
	fps 30
	loop
}

$Sequence "flinch" {
	"fheadcrab_anims\flinch.smd"
	activity "ACT_SMALL_FLINCH" 1
	fadein 0.2
	fadeout 0.2
	fps 30
}

$Sequence "dieback" {
	"fheadcrab_anims\dieback.smd"
	activity "ACT_DIESIMPLE" 1
	fadein 0.2
	fadeout 0.2
	fps 30
}

$Sequence "180_left" {
	"fheadcrab_anims\180_left.smd"
	activity "ACT_TURN_LEFT" 1
	fadein 0.2
	fadeout 0.2
	fps 15
	loop
}

$Sequence "180_right" {
	"fheadcrab_anims\180_right.smd"
	activity "ACT_TURN_RIGHT" 1
	fadein 0.2
	fadeout 0.2
	fps 15
	loop
}

$Sequence "jump" {
	"fheadcrab_anims\jump.smd"
	activity "ACT_RANGE_ATTACK1" 2
	{ event 2 0 "" }
	fadein 0.2
	fadeout 0.2
	fps 30
}

$Sequence "jump_variation1" {
	"fheadcrab_anims\jump_variation1.smd"
	activity "ACT_RANGE_ATTACK1" 1
	{ event 2 0 "" }
	fadein 0.2
	fadeout 0.2
	fps 30
}

$Sequence "jump_variation2" {
	"fheadcrab_anims\jump_variation2.smd"
	activity "ACT_RANGE_ATTACK1" 1
	{ event 2 0 "" }
	fadein 0.2
	fadeout 0.2
	fps 30
}

$Sequence "yaw_adjustment" {
	"fheadcrab_anims\yaw_adjustment.smd"
	activity "ACT_MELEE_ATTACK2" 1
	fadein 0.2
	fadeout 0.2
	fps 30
	loop
}

$Sequence "headcrabbed#1" {
	"fheadcrab_anims\headcrabbed#1.smd"
	{ event 1000 1 "" }
	{ event 1003 110 "splat" }
	{ event 1 109 "" }
	fadein 0.2
	fadeout 0.2
	fps 20
}

$Sequence "headcrabbed#2" {
	"fheadcrab_anims\headcrabbed#2.smd"
	{ event 1000 1 "" }
	{ event 1 165 "" }
	fadein 0.2
	fadeout 0.2
	fps 20
}

$Sequence "headidle" {
	"fheadcrab_anims\headidle.smd"
	fadein 0.2
	fadeout 0.2
	fps 18
	loop
}

$Sequence "crashidle" {
	"fheadcrab_anims\crashidle.smd"
	fadein 0.2
	fadeout 0.2
	fps 34
	loop
}

$Sequence "crash" {
	"fheadcrab_anims\crash.smd"
	{ event 1000 1 "" }
	fadein 0.2
	fadeout 0.2
	fps 20
}

$Sequence "hstruggleidle" {
	"fheadcrab_anims\hstruggleidle.smd"
	fadein 0.2
	fadeout 0.2
	fps 18
	loop
}

$Sequence "hstruggle" {
	"fheadcrab_anims\hstruggle.smd"
	{ event 1000 1 "" }
	fadein 0.2
	fadeout 0.2
	fps 28
}

$Sequence "ragdoll" {
	"fheadcrab_anims\ragdoll.smd"
	activity "ACT_DIERAGDOLL" 1
	fadein 0.2
	fadeout 0.2
	fps 30
}
