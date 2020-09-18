//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef BONE_MERGE_CACHE_H
#define BONE_MERGE_CACHE_H
#ifdef _WIN32
#pragma once
#endif


class C_BaseAnimating;
class CStudioHdr;
class CBoneBitList;

#include "mathlib/vector.h"


class CBoneMergeCache
{
public:
	enum MergedBoneFlags_e
	{
		BONE_FLAG_OFFSET_MATRIX = (1 << 0),
	};

	struct boneextradata_t
	{
		int	m_nMatrixIndex;
		unsigned int m_iFlags = 0;
	};

	CBoneMergeCache();
	
	void Init( C_BaseAnimating *pOwner );

	// Updates the lookups that let it merge bones quickly.
	virtual void UpdateCache();
	
	// This copies the transform from all bones in the followed entity that have 
	// names that match our bones.
	virtual void MergeMatchingBones(int boneMask, CBoneBitList &boneComputed);

	// copy bones instead of matrices
	void CopyParentToChild( const Vector parentPos[], const Quaternion parentQ[], Vector childPos[], Quaternion childQ[], int boneMask );
	void CopyChildToParent( const Vector childPos[], const Quaternion childQ[], Vector parentPos[], Quaternion parentQ[], int boneMask );

	// Returns true if the specified bone is one that gets merged in MergeMatchingBones.
	int IsBoneMerged( int iBone ) const;

	// Gets the origin for the first merge bone on the parent.
	bool GetAimEntOrigin( Vector *pAbsOrigin, QAngle *pAbsAngles );

	bool GetRootBone( matrix3x4_t &rootBone );

protected:
	virtual int	GetParentBone(CStudioHdr *pHdr, const char *pszName, boneextradata_t &extraData);
	void		GetFollowBone(int iMergeBone, matrix3x4a_t& matBoneOut);

	// This is the entity that we're keeping the cache updated for.
	C_BaseAnimating *m_pOwner;

	// All the cache data is based off these. When they change, the cache data is regenerated.
	// These are either all valid pointers or all NULL.
	C_BaseAnimating *m_pFollow;
	CStudioHdr		*m_pFollowHdr;
	const studiohdr_t	*m_pFollowRenderHdr;
	CStudioHdr		*m_pOwnerHdr;

	// This is the mask we need to use to set up bones on the followed entity to do the bone merge
	int				m_nFollowBoneSetupMask;

	// Cache data.
	class CMergedBone
	{
	public:
		unsigned short m_iMyBone;
		unsigned short m_iParentBone;
		boneextradata_t m_ExtraData;
	};

	CUtlVector<CMergedBone> m_MergedBones;
	CUtlVector<unsigned char> m_BoneMergeBits;	// One bit for each bone. The bit is set if the bone gets merged.
	CUtlVector<matrix3x4a_t, CUtlMemoryAligned<matrix3x4a_t, 16>> m_MergeOffsetMatrices;
};


inline int CBoneMergeCache::IsBoneMerged( int iBone ) const
{
	if ( m_pOwnerHdr )
		return m_BoneMergeBits[iBone >> 3] & ( 1 << ( iBone & 7 ) );
	else
		return 0;
}


#endif // BONE_MERGE_CACHE_H
