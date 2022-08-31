/**
 * \file Animation2DClipDefinition.cpp
 * \brief Contains a set of timelines that can be applied to
 * an Animation2DInstance, to pose its skeleton into a current state.
 * This is read-only data. To apply a Clip at runtime, you must
 * instantiate an Animation2D::ClipInstance.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#include "Animation2DClipDefinition.h"
#include "Animation2DDataDefinition.h"
#include "Animation2DReadWriteUtil.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionPrereqs.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Animation2D::BoneKeyFrames, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Animation2D::PathKeyFrames, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Animation2D::SlotKeyFrames, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, HashTable<HString, HashTable<HString, Vector<Animation2D::KeyFrameDeform, 2>, 2, DefaultHashTableKeyTraits<HString>>, 2, DefaultHashTableKeyTraits<HString>>, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, HashTable<HString, Vector<Animation2D::KeyFrameDeform, 2>, 2, DefaultHashTableKeyTraits<HString>>, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, SharedPtr<Animation2D::Clip>, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Vector<Animation2D::KeyFrameDeform, 2>, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Vector<Animation2D::KeyFrameIk, 2>, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Vector<Animation2D::KeyFrameTransform, 2>, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(SharedPtr<Animation2D::Clip>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::DrawOrderOffset, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrame2D, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameAttachment, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameColor, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameDeform, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameDrawOrder, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameEvent, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameIk, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFramePathMix, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFramePathPosition, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFramePathSpacing, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameRotation, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameScale, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameTransform, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::KeyFrameTwoColor, 2>)
SEOUL_BEGIN_ENUM(Animation2D::CurveType)
	SEOUL_ENUM_N("linear", Animation2D::CurveType::kLinear)
	SEOUL_ENUM_N("stepped", Animation2D::CurveType::kStepped)
	SEOUL_ENUM_N("bezier", Animation2D::CurveType::kBezier)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("time", m_fTime)
	SEOUL_PROPERTY_PAIR_N("curve", GetCurveType, SetCurveType)
		SEOUL_ATTRIBUTE(CustomSerializeProperty, "CustomDeserializeCurve")

	SEOUL_METHOD(CustomDeserializeCurve)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::BoneKeyFrames)
	SEOUL_PROPERTY_N("rotate", m_vRotation)
	SEOUL_PROPERTY_N("scale", m_vScale)
	SEOUL_PROPERTY_N("shear", m_vShear)
	SEOUL_PROPERTY_N("translate", m_vTranslation)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::Clip, TypeFlags::kDisableCopy)
	SEOUL_PROPERTY_N("bones", m_tBones)
	SEOUL_PROPERTY_N("deform", m_tDeforms)
	SEOUL_PROPERTY_N("drawOrder", m_vDrawOrder)
	SEOUL_PROPERTY_N("events", m_vEvents)
	SEOUL_PROPERTY_N("ik", m_tIk)
	SEOUL_PROPERTY_N("paths", m_tPaths)
	SEOUL_PROPERTY_N("slots", m_tSlots)
	SEOUL_PROPERTY_N("transform", m_tTransforms)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::DrawOrderOffset)
	SEOUL_PROPERTY_N("slot", m_Slot)
	SEOUL_PROPERTY_N("offset", m_iOffset)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrame2D)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("x", m_fX)
	SEOUL_PROPERTY_N("y", m_fY)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameAttachment)
	SEOUL_PROPERTY_N("time", m_fTime)
	SEOUL_PROPERTY_N("name", m_Id)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameColor)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("color", m_Color)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameDeform)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_ATTRIBUTE(CustomSerializeType, Animation2D::KeyFrameDeform::CustomDeserializeType)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameDrawOrder)
	SEOUL_PROPERTY_N("time", m_fTime)
	SEOUL_PROPERTY_N("offsets", m_vOffsets)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameEvent)
	SEOUL_ATTRIBUTE(CustomSerializeType, Animation2D::KeyFrameEvent::CustomDeserializeType)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameIk)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("mix", m_fMix)
	SEOUL_PROPERTY_N("softness", m_fSoftness)
	SEOUL_PROPERTY_N("bendPositive", m_bBendPositive)
	SEOUL_PROPERTY_N("compress", m_bCompress)
	SEOUL_PROPERTY_N("stretch", m_bStretch)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFramePathMix)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("rotateMix", m_fRotationMix)
	SEOUL_PROPERTY_N("translateMix", m_fPositionMix)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFramePathPosition)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("position", m_fPosition)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFramePathSpacing)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("spacing", m_fSpacing)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameRotation)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("angle", m_fAngleInDegrees)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameScale)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("x", m_fX)
	SEOUL_PROPERTY_N("y", m_fY)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameTransform)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("translateMix", m_fPositionMix)
	SEOUL_PROPERTY_N("rotateMix", m_fRotationMix)
	SEOUL_PROPERTY_N("scaleMix", m_fScaleMix)
	SEOUL_PROPERTY_N("shearMix", m_fShearMix)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::KeyFrameTwoColor)
	SEOUL_PARENT(Animation2D::BaseKeyFrame)
	SEOUL_PROPERTY_N("light", m_Color)
	SEOUL_PROPERTY_N("dark", m_SecondaryColor)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::PathKeyFrames)
	SEOUL_PROPERTY_N("mix", m_vMix)
	SEOUL_PROPERTY_N("position", m_vPosition)
	SEOUL_PROPERTY_N("spacing", m_vSpacing)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::SlotKeyFrames)
	SEOUL_PROPERTY_N("attachment", m_vAttachment)
	SEOUL_PROPERTY_N("color", m_vColor)
	SEOUL_PROPERTY_N("twoColor", m_vTwoColor)
SEOUL_END_TYPE()

namespace Animation2D
{

// Keys used for direct serialization of KeyFrameEvent.
static const HString kFloat("float");
static const HString kInt("int");
static const HString kName("name");
static const HString kOffset("offset");
static const HString kString("string");
static const HString kTime("time");
static const HString kVertices("vertices");

static const HString kaControlPointNames[] =
{
	HString("curve"),
	HString("c2"),
	HString("c3"),
	HString("c4"),
};

Bool KeyFrameDeform::CustomDeserializeType(
	Reflection::SerializeContext& rContext,
	const DataStore& dataStore,
	const DataNode& table,
	const Reflection::WeakAny& objectThis,
	Bool bSkipPostSerialize)
{
	// Perform standard deserialization to grab base values.
	if (!Reflection::Type::TryDeserialize(
		rContext,
		dataStore,
		table,
		objectThis,
		bSkipPostSerialize,
		true))
	{
		return false;
	}

	// Grab the instance pointer we're populating.
	auto pThis = objectThis.Cast<Animation2D::KeyFrameDeform*>();

	// Offset is optional and defaults to 0.
	Int32 iOffset = 0;
	DataNode node;
	(void)dataStore.GetValueFromTable(table, kOffset, node);
	(void)dataStore.AsInt32(node, iOffset);

	// Sanity check.
	if (iOffset < 0)
	{
		SEOUL_WARN("%s: vertices out of range.",
			rContext.GetKey().GetFilePath().CStr());
		return false;
	}

	// Get data we need to fill in the vertices completely - the input data is
	// deltas, we want to specify it as full state.
	HString skinId = rContext.Top(3u).m_Name;
	HString slotId = rContext.Top(2u).m_Name;
	HString attachmentId = rContext.Top(1u).m_Name;

	// Grab the base vertices - if none defined, we can't continue. Targeting error.
	auto pData = rContext.GetUserData().Cast<Animation2D::DataDefinition*>();

	// Vertices always starts off as a copy of the base vertices.
	if (!pData->CopyBaseVertices(skinId, slotId, attachmentId, pThis->m_vVertices))
	{
		SEOUL_WARN("%s: base vertices not defined, can't process deforms.",
			rContext.GetKey().GetFilePath().CStr());
		return false;
	}

	// If the base data has no vertices, we're done.
	if (pThis->m_vVertices.IsEmpty())
	{
		return true;
	}

	// Get the input vertices - these are optional and may have a size of 0.
	DataNode value;
	(void)dataStore.GetValueFromTable(table, kVertices, value);

	// Get the input count. This is in floats, not Vector2Ds.
	UInt32 uCount = 0u;
	(void)dataStore.GetArrayCount(value, uCount);

	// Early out if count is 0.
	if (0u == uCount)
	{
		return true;
	}

	// Check patch length.
	if (iOffset + (Int32)uCount > (Int32)pThis->m_vVertices.GetSize())
	{
		SEOUL_WARN("%s: vertices patch is too large.",
			rContext.GetKey().GetFilePath().CStr());
		return false;
	}

	// Now iterate and patch.
	Float f = 0.0f;
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		// Get each input float as a value.
		if (!dataStore.GetValueFromArray(value, i, node) || !dataStore.AsFloat32(node, f))
		{
			SEOUL_WARN("%s: vertices array invalid, invalid element '%u'.",
				rContext.GetKey().GetFilePath().CStr(),
				i);
			return false;
		}

		// Compute output offset.
		Int32 const iOut = (iOffset + i);
		pThis->m_vVertices[iOut] += f;
	}

	return true;
}

Bool KeyFrameEvent::CustomDeserializeType(
	Reflection::SerializeContext& rContext,
	const DataStore& dataStore,
	const DataNode& table,
	const Reflection::WeakAny& objectThis,
	Bool bSkipPostSerialize)
{
	auto p = objectThis.Cast<Animation2D::KeyFrameEvent*>();

	// Get values - time and name are both optional.
	DataNode value;
	(void)dataStore.GetValueFromTable(table, kTime, value); (void)dataStore.AsFloat32(value, p->m_fTime);
	(void)dataStore.GetValueFromTable(table, kName, value); (void)dataStore.AsString(value, p->m_Id);

	// The remaining values, we need to lookup from the default
	// event data if they were not defined on the timeline.
	DataNode floatValue;
	Bool const bFloat = dataStore.GetValueFromTable(table, kFloat, floatValue); (void)dataStore.AsFloat32(floatValue, p->m_f);
	DataNode intValue;
	Bool const bInt = dataStore.GetValueFromTable(table, kInt, intValue); (void)dataStore.AsInt32(intValue, p->m_i);
	DataNode stringValue;
	Bool const bString = dataStore.GetValueFromTable(table, kString, stringValue); (void)dataStore.AsString(stringValue, p->m_s);

	// Need to get the default data and use it.
	if (!bFloat || !bInt || !bString)
	{
		auto pData = rContext.GetUserData().Cast<Animation2D::DataDefinition*>();
		auto pDefault = pData->GetEvents().Find(p->m_Id);

		// Apply defaults if we need them.
		if (nullptr != pDefault)
		{
			if (!bFloat) { p->m_f = pDefault->m_f; }
			if (!bInt) { p->m_i = pDefault->m_i; }
			if (!bString) { p->m_s = pDefault->m_s; }
		}
	}

	return true;
}

/** Generates a piecewise-linear approximation of a bezier curve, from 4 control points. */
static void PopulateCurve(const FixedArray<Float, 4u>& aControlPoints, BezierCurve& ra)
{
	Float32 const fCX0 = aControlPoints[0];
	Float32 const fCY0 = aControlPoints[1];
	Float32 const fCX1 = aControlPoints[2];
	Float32 const fCY1 = aControlPoints[3];

	Float32 const fTempX = (-fCX0 * 2.0f + fCX1) * 0.03f;
	Float32 const fTempY = (-fCY0 * 2.0f + fCY1) * 0.03f;

	Float32 fXd3 = ((fCX0 - fCX1) * 3.0f + 1.0f) * 0.006f;
	Float32 fYd3 = ((fCY0 - fCY1) * 3.0f + 1.0f) * 0.006f;

	Float32 fXd2 = fTempX * 2.0f + fXd3;
	Float32 fYd2 = fTempY * 2.0f + fYd3;

	Float32 fXd1 = fCX0 * 0.3f + fTempX + fXd3 * 0.16666667f;
	Float32 fYd1 = fCY0 * 0.3f + fTempY + fYd3 * 0.16666667f;

	Float32 fX = fXd1;
	Float32 fY = fYd1;

	for (UInt32 i = 0u; i < ra.GetSize(); i += 2u)
	{
		ra[i+0] = fX;
		ra[i+1] = fY;
		fXd1 += fXd2;
		fYd1 += fYd2;
		fXd2 += fXd3;
		fYd2 += fYd3;
		fX += fXd1;
		fY += fYd1;
	}
}

Bool BaseKeyFrame::CustomDeserializeCurve(
	Reflection::SerializeContext* pContext,
	DataStore const* pDataStore,
	const DataNode& value,
	const DataNode& parentValue)
{
	// New as of c63bc7b88f7202a0c7718b6a724040a4e314c28f of spine runtime,
	// May 2019, 3.8 release. If "curve" property is just a number, then
	// it is the first control point, and the remaining control points will
	// be on optional arguments of the parent named "c2", "c3", and "c4".
	Float fC0 = 0.0f;
	if (value.IsArray() || pDataStore->AsFloat32(value, fC0))
	{
		FixedArray<Float, 4u> aControlPoints;

		if (value.IsArray())
		{
			if (!Reflection::Type::TryDeserialize(
				*pContext,
				*pDataStore,
				value,
				&aControlPoints))
			{
				return false;
			}
		}
		else
		{
			aControlPoints[0] = fC0;
			// Defaults.
			aControlPoints[1] = 0.0f;
			aControlPoints[2] = 1.0f;
			aControlPoints[3] = 1.0f;
			// Get values - failure of the get is expected, but
			// failure on the AsFloat32 is not.
			{
				DataNode extra;
				for (auto i = 1; i < 4; ++i)
				{
					if (pDataStore->GetValueFromTable(parentValue, kaControlPointNames[i], extra))
					{
						if (!pDataStore->AsFloat32(extra, aControlPoints[i]))
						{
							return false;
						}
					}
				}
			}
		}

		auto pData = pContext->GetUserData().Cast<Animation2D::DataDefinition*>();
		auto& v = pData->GetCurves();
		m_uCurveDataOffset = (UInt32)v.GetSize();
		m_uCurveType = (UInt32)CurveType::kBezier;
		v.PushBack(BezierCurve());
		PopulateCurve(aControlPoints, v.Back());
		return true;
	}
	else
	{
		Byte const* s = nullptr;
		UInt32 u = 0u;
		HString name;
		if (!pDataStore->AsString(value, s, u) ||
			!HString::Get(name, s, u))
		{
			return false;
		}

		CurveType eValue;
		if (!EnumOf<CurveType>().TryGetValue(name, eValue))
		{
			return false;
		}

		m_uCurveDataOffset = 0u;
		m_uCurveType = (UInt32)eValue;
		return true;
	}
}

Clip::Clip()
{
}

Clip::~Clip()
{
}

Bool Clip::Load(ReadWriteUtil& r)
{
	Bool bReturn = true;
	bReturn = bReturn && r.Read(m_tBones);
	bReturn = bReturn && r.Read(m_tDeforms);
	bReturn = bReturn && r.Read(m_vDrawOrder);
	bReturn = bReturn && r.Read(m_vEvents);
	bReturn = bReturn && r.Read(m_tIk);
	bReturn = bReturn && r.Read(m_tPaths);
	bReturn = bReturn && r.Read(m_tSlots);
	bReturn = bReturn && r.Read(m_tTransforms);
	return bReturn;
}

Bool Clip::Save(ReadWriteUtil& r) const
{
	Bool bReturn = true;
	bReturn = bReturn && r.Write(m_tBones);
	bReturn = bReturn && r.Write(m_tDeforms);
	bReturn = bReturn && r.Write(m_vDrawOrder);
	bReturn = bReturn && r.Write(m_vEvents);
	bReturn = bReturn && r.Write(m_tIk);
	bReturn = bReturn && r.Write(m_tPaths);
	bReturn = bReturn && r.Write(m_tSlots);
	bReturn = bReturn && r.Write(m_tTransforms);
	return bReturn;
}

Bool Clip::operator==(const Clip& b) const
{
	Bool bReturn = true;
	bReturn = bReturn && (m_tBones == b.m_tBones);
	bReturn = bReturn && (m_tDeforms == b.m_tDeforms);
	bReturn = bReturn && (m_vDrawOrder == b.m_vDrawOrder);
	bReturn = bReturn && (m_vEvents == b.m_vEvents);
	bReturn = bReturn && (m_tIk == b.m_tIk);
	bReturn = bReturn && (m_tPaths == b.m_tPaths);
	bReturn = bReturn && (m_tSlots == b.m_tSlots);
	bReturn = bReturn && (m_tTransforms == b.m_tTransforms);
	return bReturn;
}

} // namespace Animation2D

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D
