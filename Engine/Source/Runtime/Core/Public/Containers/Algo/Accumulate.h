// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

// TPlus<T> specifically takes const T& and returns T.
// TPlus<> (empty angle brackets) is late-binding, taking whatever is passed and returning the correct result type for (A+B)
template<typename T = void>
struct TPlus
{
	FORCEINLINE T operator()(const T& A, const T& B) { return A + B; }
};
template<>
struct TPlus<void>
{
	template<typename U, typename V>
	FORCEINLINE auto operator()(U&& A, V&& B) -> decltype(A + B) { return A + B; }
};


namespace Algo
{
	/**
	* Sums a range by successively applying Op.
	*
	* @param  Input  Any iterable type
	* @param  Init  Initial value for the summation
	* @param  Op  Summing Operation (the default is TPlus<>)
	*
	* @return the result of summing all the elements of Input
	*/
	template <typename T, typename A, typename OpT>
	FORCEINLINE T Accumulate(const A& Input, T init, OpT Op)
	{
		T result = init;
		for (auto&& i : Input)
		{
			result = Op(result, i);
		}
		return result;
	}

	/**
	 * Sums a range.
	 *
	 * @param  Input  Any iterable type
	 * @param  Init  Initial value for the summation
	 *
	 * @return the result of summing all the elements of Input
	 */
	template <typename T, typename A>
	FORCEINLINE T Accumulate(const A& Input, T init)
	{
		return Accumulate(Input, init, TPlus<>());
	}

	/**
	* Sums a range by applying MapOp to each element, and then summing the results.
	*
	* @param  Input  Any iterable type
	* @param  Init  Initial value for the summation
	* @param  MapOp  Mapping Operation
	* @param  Op  Summing Operation (the default is TPlus<>)
	*
	* @return the result of mapping and then summing all the elements of Input
	*/
	template <typename T, typename A, typename MapT, typename OpT>
	FORCEINLINE T TransformAccumulate(const A& Input, MapT MapOp, T init, OpT Op)
	{
		T result = init;
		for (auto&& i : Input)
		{
			result = Op(result, MapOp(i));
		}
		return result;
	}

	/**
	* Sums a range by applying MapOp to each element, and then summing the results.
	*
	* @param  Input  Any iterable type
	* @param  Init  Initial value for the summation
	* @param  MapOp  Mapping Operation
	*
	* @return the result of mapping and then summing all the elements of Input
	*/
	template <typename T, typename A, typename MapT>
	FORCEINLINE T TransformAccumulate(const A& Input, MapT MapOp, T init)
	{
		return TransformAccumulate(Input, MapOp, init, TPlus<>());
	}
}
