// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A resource for rendering a UMaterial in Slate
 */
class FSlateMaterialResource : public FSlateShaderResource
{
public:
	FSlateMaterialResource(const UMaterialInterface& InMaterialResource, const FVector2D& InImageSize, FSlateShaderResource* InTextureMask = nullptr );
	~FSlateMaterialResource();

	virtual uint32 GetWidth() const override { return Width; }
	virtual uint32 GetHeight() const override { return Height; }
	virtual ESlateShaderResource::Type GetType() const override { return ESlateShaderResource::Material; }

	void UpdateMaterial(const UMaterialInterface& InMaterialResource, const FVector2D& InImageSize, FSlateShaderResource* InTextureMask );
	void ResetMaterial();

	/** @return The material render proxy */
	FMaterialRenderProxy* GetRenderProxy() const { return MaterialObject->GetRenderProxy(false, false); }

	/** @return the material object */
	const UMaterialInterface* GetMaterialObject() const { return MaterialObject; }

	FSlateShaderResource* GetTextureMaskResource() const { return TextureMaskResource; }
public:
	const class UMaterialInterface* MaterialObject;
	/** Slate proxy used for batching the material */
	FSlateShaderResourceProxy* SlateProxy;

	FSlateShaderResource* TextureMaskResource;
	uint32 Width;
	uint32 Height;
};

