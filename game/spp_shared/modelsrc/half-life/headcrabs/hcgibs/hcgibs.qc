// Created by Crowbar 0.58

$ModelName "gibs/hcgibs.mdl"

$BodyGroup "parts"
{
	studio "hcgibs_gib_alien_01"
	studio "hcgibs_gib_alien_02"
	studio "hcgibs_gib_alien_03"
	studio "hcgibs_gib_alien_04"
	studio "hcgibs_gib_alien_05"
	studio "hcgibs_gib_alien_06"
}

$cdmaterials "models/half-life/hcgibs"

$TextureGroup "skinfamilies"
{
	{ "a_gib"  }
	{ "a_gib2" }
	{ "a_gib3" }
	{ "a_gib4" }
	{ "a_gib5" }
	{ "a_gib6" }
	{ "a_gib7" }
	{ "a_gib8" }
}


$CBox 0 0 0 0 0 0

$BBox 0 0 0 0 0 0

$HBox 0 "gib" -4.82 -3.9 -0.38 4.82 3.85 3.49

$Include "agibs_collision.qci"

$Sequence "idle1" {
	"hcgibs_anims\idle1"
	fps 30
}
