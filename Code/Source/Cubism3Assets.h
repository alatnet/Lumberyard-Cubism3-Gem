#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace Cubism3 {
	class MocAsset {
	public:
		AZ_TYPE_INFO(MocAsset, "{7DB33C8B-8498-404C-A301-B0269AE60388}");
		static const char* GetFileFilter() {
			return "*.moc3";
		}
	};

	class Cubism3Asset {
	public:
		AZ_TYPE_INFO(Cubism3Asset, "{A679F1C0-60A1-48FB-8107-A68195D76CF2}");
		static const char* GetFileFilter() {
			return "*.model3.json";
		}
	};

	class MotionAsset {
	public:
		AZ_TYPE_INFO(MotionAsset, "{DC1BA430-5D5E-4A09-BA5F-1FB05180C6A1}");
		static const char* GetFileFilter() {
			return "*.motion3.json";
		}
	};
}

namespace AZ {
	AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<Cubism3::MocAsset>, "{159BD2E0-F8CD-4270-9C1F-7D12556D7E90}");
	AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<Cubism3::Cubism3Asset>, "{BAF20174-A79F-40E1-A93B-A5417FB3ACBC}");
	AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<Cubism3::MotionAsset>, "{9DB82D18-56C2-43C0-B73E-5FF95B8AA532}");
}