// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"
#include "PropertyHelper.h"
#include "PropertyTag.h"

/*-----------------------------------------------------------------------------
	UArrayProperty.
-----------------------------------------------------------------------------*/


void UArrayProperty::LinkInternal(FArchive& Ar)
{
	FLinkerLoad* MyLinker = GetLinker();
	if( MyLinker )
	{
		MyLinker->Preload(this);
	}
	Ar.Preload(Inner);
	Inner->Link(Ar);
	Super::LinkInternal(Ar);
}
bool UArrayProperty::Identical( const void* A, const void* B, uint32 PortFlags ) const
{
	checkSlow(Inner);

	FScriptArrayHelper ArrayHelperA(this, A);

	const int32 ArrayNum = ArrayHelperA.Num();
	if ( B == NULL )
	{
		return ArrayNum == 0;
	}

	FScriptArrayHelper ArrayHelperB(this, B);
	if ( ArrayNum != ArrayHelperB.Num() )
	{
		return false;
	}

	for ( int32 ArrayIndex = 0; ArrayIndex < ArrayNum; ArrayIndex++ )
	{
		if ( !Inner->Identical( ArrayHelperA.GetRawPtr(ArrayIndex), ArrayHelperB.GetRawPtr(ArrayIndex), PortFlags) )
		{
			return false;
		}
	}

	return true;
}
void UArrayProperty::SerializeItem( FArchive& Ar, void* Value, void const* Defaults ) const
{
	checkSlow(Inner);

	// Ensure that the Inner itself has been loaded before calling SerializeItem() on it
	Ar.Preload(Inner);

	FScriptArrayHelper ArrayHelper(this, Value);
	int32		n		= ArrayHelper.Num();
	Ar << n;
	if( Ar.IsLoading() )
	{
		ArrayHelper.EmptyAndAddValues(n);
	}
	ArrayHelper.CountBytes( Ar );

	// Serialize a PropertyTag for the inner property of this array, allows us to validate the inner struct to see if it has changed
	FPropertyTag InnerTag(Ar, Inner, 0, (uint8*)Value, (uint8*)Defaults);
	if (Ar.UE4Ver() >= VER_UE4_INNER_ARRAY_TAG_INFO && InnerTag.Type == NAME_StructProperty)
	{
		if (Ar.IsSaving())
		{
			Ar << InnerTag;
		}
		else if (Ar.IsLoading())
		{
			Ar << InnerTag;

			auto CanSerializeFromStructWithDifferentName = [](const FArchive& InAr, const FPropertyTag& PropertyTag, const UStructProperty* StructProperty)
			{
				return PropertyTag.StructGuid.IsValid() && StructProperty && StructProperty->Struct && (PropertyTag.StructGuid == StructProperty->Struct->GetCustomGuid() && (StructProperty->Struct->StructFlags & STRUCT_SerializeFromMismatchedTag));
			};

			// Check if the Inner property can successfully serialize, the type may have changed
			UStructProperty* StructProperty = CastChecked<UStructProperty>(Inner);
			if (InnerTag.StructName != StructProperty->Struct->GetFName()
				&& !CanSerializeFromStructWithDifferentName(Ar, InnerTag, StructProperty))
			{
				UE_LOG(LogClass, Warning, TEXT("Property %s of %s has a struct type mismatch (tag %s != prop %s) in package:  %s. If that struct got renamed, add an entry to ActiveStructRedirects."),
					*InnerTag.Name.ToString(), *GetName(), *InnerTag.StructName.ToString(), *CastChecked<UStructProperty>(Inner)->Struct->GetName(), *Ar.GetArchiveName());

#if WITH_EDITOR
				// Ensure the structure is initialized
				for (int32 i = 0; i < n; i++)
				{
					StructProperty->Struct->InitializeDefaultValue(ArrayHelper.GetRawPtr(i));
				}
#endif // WITH_EDITOR

				// Skip the property
				const int64 StartOfProperty = Ar.Tell();
				const int64 RemainingSize = InnerTag.Size - (Ar.Tell() - StartOfProperty);
				uint8 B;
				for (int64 i = 0; i < RemainingSize; i++)
				{
					Ar << B;
				}
				return;
			}
		}
	}

	// need to know how much data this call to SerializeItem consumes, so mark where we are
	int32 DataOffset = Ar.Tell();

	for( int32 i=0; i<n; i++ )
	{
		Inner->SerializeItem( Ar, ArrayHelper.GetRawPtr(i) );
	}

	if (Ar.UE4Ver() >= VER_UE4_INNER_ARRAY_TAG_INFO && Ar.IsSaving() && InnerTag.Type == NAME_StructProperty)
	{
		// set the tag's size
		InnerTag.Size = Ar.Tell() - DataOffset;

		if (InnerTag.Size > 0)
		{
			// mark our current location
			DataOffset = Ar.Tell();

			// go back and re-serialize the size now that we know it
			Ar.Seek(InnerTag.SizeOffset);
			Ar << InnerTag.Size;

			// return to the current location
			Ar.Seek(DataOffset);
		}
	}
}

bool UArrayProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData ) const
{
	UE_LOG( LogProperty, Fatal, TEXT( "Deprecated code path" ) );
	return 1;
}

void UArrayProperty::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	Ar << Inner;
	checkSlow(Inner || HasAnyFlags(RF_ClassDefaultObject) || IsPendingKill());
}
void UArrayProperty::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UArrayProperty* This = CastChecked<UArrayProperty>(InThis);
	Collector.AddReferencedObject( This->Inner, This );
	Super::AddReferencedObjects( This, Collector );
}

FString UArrayProperty::GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& InnerTypeText, const FString& InInnerExtendedTypeText) const
{
	if (ExtendedTypeText != NULL)
	{
		FString InnerExtendedTypeText = InInnerExtendedTypeText;
		if (InnerExtendedTypeText.Len() && InnerExtendedTypeText.Right(1) == TEXT(">"))
		{
			// if our internal property type is a template class, add a space between the closing brackets b/c VS.NET cannot parse this correctly
			InnerExtendedTypeText += TEXT(" ");
		}
		else if (!InnerExtendedTypeText.Len() && InnerTypeText.Len() && InnerTypeText.Right(1) == TEXT(">"))
		{
			// if our internal property type is a template class, add a space between the closing brackets b/c VS.NET cannot parse this correctly
			InnerExtendedTypeText += TEXT(" ");
		}
		*ExtendedTypeText = FString::Printf(TEXT("<%s%s>"), *InnerTypeText, *InnerExtendedTypeText);
	}
	return TEXT("TArray");
}

FString UArrayProperty::GetCPPType( FString* ExtendedTypeText/*=NULL*/, uint32 CPPExportFlags/*=0*/ ) const
{
	checkSlow(Inner);
	FString InnerExtendedTypeText;
	FString InnerTypeText;
	if ( ExtendedTypeText != NULL )
	{
		InnerTypeText = Inner->GetCPPType(&InnerExtendedTypeText, CPPExportFlags & ~CPPF_ArgumentOrReturnValue); // we won't consider array inners to be "arguments or return values"
	}
	return GetCPPTypeCustom(ExtendedTypeText, CPPExportFlags, InnerTypeText, InnerExtendedTypeText);
}

FString UArrayProperty::GetCPPTypeForwardDeclaration() const
{
	checkSlow(Inner);
	return Inner->GetCPPTypeForwardDeclaration();
}
FString UArrayProperty::GetCPPMacroType( FString& ExtendedTypeText ) const
{
	checkSlow(Inner);
	ExtendedTypeText = Inner->GetCPPType();
	return TEXT("TARRAY");
}
void UArrayProperty::ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const
{
	checkSlow(Inner);

	if (0 != (PortFlags & PPF_ExportCpp))
	{
		FString ExtendedTypeText;
		FString TypeText = GetCPPType(&ExtendedTypeText, EPropertyExportCPPFlags::CPPF_BlueprintCppBackend);
		ValueStr += FString::Printf(TEXT("%s%s()"), *TypeText, *ExtendedTypeText);
		return;
	}

	FScriptArrayHelper ArrayHelper(this, PropertyValue);
	FScriptArrayHelper DefaultArrayHelper(this, DefaultValue);

	uint8* StructDefaults = NULL;
	UStructProperty* StructProperty = dynamic_cast<UStructProperty*>(Inner);
	if ( StructProperty != NULL )
	{
		checkSlow(StructProperty->Struct);
		StructDefaults = (uint8*)FMemory::Malloc(StructProperty->Struct->GetStructureSize());
		StructProperty->InitializeValue(StructDefaults);
	}

	const bool bReadableForm = (0 != (PPF_BlueprintDebugView & PortFlags));

	int32 Count = 0;
	for( int32 i=0; i<ArrayHelper.Num(); i++ )
	{
		++Count;
		if(!bReadableForm)
		{
			if ( Count == 1 )
			{
				ValueStr += TCHAR('(');
			}
			else
			{
				ValueStr += TCHAR(',');
			}
		}
		else
		{
			if(Count > 1)
			{
				ValueStr += TCHAR('\n');
			}
			ValueStr += FString::Printf(TEXT("[%i] "), i);
		}

		uint8* PropData = ArrayHelper.GetRawPtr(i);

		// Always use struct defaults if the inner is a struct, for symmetry with the import of array inner struct defaults
		uint8* PropDefault = ( StructProperty != NULL ) ? StructDefaults :
			( ( DefaultValue && DefaultArrayHelper.Num() > i ) ? DefaultArrayHelper.GetRawPtr(i) : NULL );

		Inner->ExportTextItem( ValueStr, PropData, PropDefault, Parent, PortFlags|PPF_Delimited, ExportRootScope );
	}

	if ((Count > 0) && !bReadableForm)
	{
		ValueStr += TEXT(")");
	}
	if (StructDefaults)
	{
		StructProperty->DestroyValue(StructDefaults);
		FMemory::Free(StructDefaults);
	}
}

const TCHAR* UArrayProperty::ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText ) const
{
	checkSlow(Inner);

	FScriptArrayHelper ArrayHelper(this, Data);

	// If we export an empty array we export an empty string, so ensure that if we're passed an empty string
	// we interpret it as an empty array.
	if (*Buffer == TCHAR('\0') || *Buffer == TCHAR(')') || *Buffer == TCHAR(','))
	{
		ArrayHelper.EmptyValues();
		return NULL;
	}

	if ( *Buffer++ != TCHAR('(') )
	{
		return NULL;
	}

	ArrayHelper.EmptyValues();

	SkipWhitespace(Buffer);

	int32 Index = 0;

	ArrayHelper.ExpandForIndex(0);
	while ((Buffer != NULL) && (*Buffer != TCHAR(')')))
	{
		SkipWhitespace(Buffer);

		if (*Buffer != TCHAR(','))
		{
			// Parse the item
			Buffer = Inner->ImportText(Buffer, ArrayHelper.GetRawPtr(Index), PortFlags | PPF_Delimited, Parent, ErrorText);

			if(!Buffer)
			{
				return NULL;
			}

			SkipWhitespace(Buffer);
		}


		if (*Buffer == TCHAR(','))
		{
			Buffer++;
			Index++;
			ArrayHelper.ExpandForIndex(Index);
		}
		else
		{
			break;
		}
	}

	// Make sure we ended on a )
	CA_SUPPRESS(6011)
	if (*Buffer++ != TCHAR(')'))
	{
		return NULL;
	}

	return Buffer;
}

void UArrayProperty::AddCppProperty( UProperty* Property )
{
	check(!Inner);
	check(Property);

	Inner = Property;
}

void UArrayProperty::CopyValuesInternal( void* Dest, void const* Src, int32 Count  ) const
{
	check(Count==1); // this was never supported, apparently
	FScriptArrayHelper SrcArrayHelper(this, Src);
	FScriptArrayHelper DestArrayHelper(this, Dest);

	int32 Num = SrcArrayHelper.Num();
	if ( !(Inner->PropertyFlags & CPF_IsPlainOldData) )
	{
		DestArrayHelper.EmptyAndAddValues(Num);
	}
	else
	{
		DestArrayHelper.EmptyAndAddUninitializedValues(Num);
	}
	if (Num)
	{
		int32 Size = Inner->ElementSize;
		uint8* SrcData = (uint8*)SrcArrayHelper.GetRawPtr();
		uint8* DestData = (uint8*)DestArrayHelper.GetRawPtr();
		if( !(Inner->PropertyFlags & CPF_IsPlainOldData) )
		{
			for( int32 i=0; i<Num; i++ )
			{
				Inner->CopyCompleteValue( DestData + i * Size, SrcData + i * Size );
			}
		}
		else
		{
			FMemory::Memcpy( DestData, SrcData, Num*Size );
		}
	}
}
void UArrayProperty::ClearValueInternal( void* Data ) const
{
	FScriptArrayHelper ArrayHelper(this, Data);
	ArrayHelper.EmptyValues();
}
void UArrayProperty::DestroyValueInternal( void* Dest ) const
{
	FScriptArrayHelper ArrayHelper(this, Dest);
	ArrayHelper.EmptyValues();

	//@todo UE4 potential double destroy later from this...would be ok for a script array, but still
	((FScriptArray*)Dest)->~FScriptArray();
}
bool UArrayProperty::PassCPPArgsByRef() const
{
	return true;
}

/**
 * Creates new copies of components
 * 
 * @param	Data				pointer to the address of the instanced object referenced by this UComponentProperty
 * @param	DefaultData			pointer to the address of the default value of the instanced object referenced by this UComponentProperty
 * @param	Owner				the object that contains this property's data
 * @param	InstanceGraph		contains the mappings of instanced objects and components to their templates
 */
void UArrayProperty::InstanceSubobjects( void* Data, void const* DefaultData, UObject* Owner, FObjectInstancingGraph* InstanceGraph )
{
	if( Data && Inner->ContainsInstancedObjectProperty())
	{
		FScriptArrayHelper ArrayHelper(this, Data);
		FScriptArrayHelper DefaultArrayHelper(this, DefaultData);

		for( int32 ElementIndex = 0; ElementIndex < ArrayHelper.Num(); ElementIndex++ )
		{
			uint8* DefaultValue = (DefaultData && ElementIndex < DefaultArrayHelper.Num()) ? DefaultArrayHelper.GetRawPtr(ElementIndex) : NULL;
			Inner->InstanceSubobjects( ArrayHelper.GetRawPtr(ElementIndex), DefaultValue, Owner, InstanceGraph );
		}
	}
}

bool UArrayProperty::SameType(const UProperty* Other) const
{
	return Super::SameType(Other) && Inner && Inner->SameType(((UArrayProperty*)Other)->Inner);
}

IMPLEMENT_CORE_INTRINSIC_CLASS(UArrayProperty, UProperty,
	{
		Class->EmitObjectReference(STRUCT_OFFSET(UArrayProperty, Inner), TEXT("Inner"));

		// Ensure that TArray and FScriptArray are interchangeable, as FScriptArray will be used to access a native array property
		// from script that is declared as a TArray in C++.
		static_assert(sizeof(FScriptArray) == sizeof(TArray<uint8>), "FScriptArray and TArray<uint8> must be interchangable.");
	}
);
