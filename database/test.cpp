/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Fixed.cpp                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: kaltevog <kaltevog@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2024/08/05 23:16:01 by kaltevog      #+#    #+#                 */
/*   Updated: 2024/08/05 23:16:01 by kaltevog      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "Fixed.hpp"
#include <cmath>

const int Fixed::_fractional_bits = 8;

Fixed::Fixed() : _fixed_point_number(0)
{
	std::cout << "Default constructor called\n";
}

Fixed::Fixed(const int number)
{
	_fixed_point_number = number << _fractional_bits;
	std::cout << "Int constructor called\n";
}

Fixed::Fixed(const float number)
{

	_fixed_point_number = static_cast<int>(number * (1 << _fractional_bits));
	std::cout << "Float constructor called" << std::endl;
}

Fixed::Fixed(const Fixed &other)
{
	std::cout << "Copy constructor called\n";
	*this = other;
}

Fixed &Fixed::operator=(const Fixed &other)
{
	std::cout << "Copy assignment constructor called\n";
	setRawBits(other.getRawBits());
	return (*this);
}

Fixed::~Fixed()
{
	std::cout << "Deconstructor called\n";
}

int Fixed::getRawBits(void) const
{
	return (_fixed_point_number);
}

void Fixed::setRawBits(int const raw)
{
	_fixed_point_number = raw;
}

float Fixed::toFloat(void) const
{
	return (static_cast<float>(_fixed_point_number) / (1 << _fractional_bits));
}

int Fixed::toInt(void) const
{
	return (_fixed_point_number >> _fractional_bits);
}

std::ostream &operator<<(std::ostream &os, Fixed const &fixed)
{
    os << fixed.toFloat();
    return os;
 }