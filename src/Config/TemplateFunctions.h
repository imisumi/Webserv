/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   TemplateFunctions.h                                :+:    :+:            */
/*                                                     +:+                    */
/*   By: kwchu <kwchu@student.codam.nl>               +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/09/18 12:44:52 by kwchu         #+#    #+#                 */
/*   Updated: 2024/09/18 12:45:03 by kwchu         ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <vector>

template<typename T>
inline bool	elementIsUniqueInVector(
	const std::vector<T>& vec,
	const T& input)
{
	for (const T& element : vec)
	{
		if (element == input)
			return false;
	}
	return true;
}
